/*
 * This file is the result of combining COPYING and ThreadPool.h from
 * https://github.com/progschj/ThreadPool
 * -wry
 */

/*
Copyright (c) 2012 Jakob Progsch

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

// need this type to "erase" the return type of the packaged task
struct any_packaged_base {
    virtual void execute() = 0;
    virtual ~any_packaged_base() {}
};

template<class R>
struct any_packaged : public any_packaged_base {
    any_packaged(std::packaged_task<R()> &&t)
     : task(std::move(t))
    {
    }
    void execute()
    {
        task();
    }
    std::packaged_task<R()> task;
};

class any_packaged_task {
public:
    template<class R>
    any_packaged_task(std::packaged_task<R()> &&task)
     : ptr(new any_packaged<R>(std::move(task)))
    {
    }
    void operator()()
    {
        ptr->execute();
    }
private:
    std::shared_ptr<any_packaged_base> ptr;
};

class ThreadPool;
 
// our worker thread objects
class Worker {
public:
    Worker(ThreadPool &s) : pool(s) { }
    void operator()();
private:
    ThreadPool &pool;
};

// the actual thread pool
class ThreadPool {
public:
    ThreadPool(size_t);
    template<class T, class F>
    std::future<T> enqueue(F f);
    ~ThreadPool();
private:
    friend class Worker;

    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::deque< any_packaged_task > tasks;
    
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};
 
void Worker::operator()()
{
    while(true)
    {
        std::unique_lock<std::mutex> lock(pool.queue_mutex);
        while(!pool.stop && pool.tasks.empty())
            pool.condition.wait(lock);
        if(pool.stop)
            return;
        any_packaged_task task(pool.tasks.front());
        pool.tasks.pop_front();
        lock.unlock();
        task();
    }
}
 
// the constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads)
    :   stop(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.push_back(std::thread(Worker(*this)));
}

// add new work item to the pool
template<class T, class F>
std::future<T> ThreadPool::enqueue(F f)
{
    std::packaged_task<T()> task(f);
    std::future<T> res= task.get_future();    
    {
        std::unique_lock<std::mutex> lock(queue_mutex);    
        tasks.push_back(any_packaged_task(std::move(task)));
    }
    condition.notify_one();
    return res;
}
 
// the destructor joins all threads
ThreadPool::~ThreadPool()
{
    stop = true;
    condition.notify_all();
    for(size_t i = 0;i<workers.size();++i)
        workers[i].join();
}

#endif
