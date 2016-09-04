/*
 * This file is the result of combining COPYING and (parts of) ThreadPool.h
 * from https://github.com/progschj/ThreadPool
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
/* THIS IS AN ALTERED VERSION OF THE ORIGINAL SOURCE -wry */

#include "libbrot2/ThreadPool.h"
#include "libbrot2/Exception.h"

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
		/* Added exception handler around task() -wry */
		try {
			task();
		} catch (std::exception& e) {
			std::cerr << "FATAL: Uncaught exception in worker: " << e.what() << std::endl;
			exit(5);
		}
    }
}

// the constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads)
    :   stop(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.push_back(std::thread(Worker(*this)));
}


// the destructor joins all threads
ThreadPool::~ThreadPool()
{
    stop = true;
    condition.notify_all();
    for(size_t i = 0;i<workers.size();++i)
        workers[i].join();
}
