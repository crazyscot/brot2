noinst_LIBRARIES=libjob.a

libjob_a_SOURCES= \
					IJob.h IJobEngine.h \
					SimpleJobEngine.h SimpleJobEngine.cpp \
					SimpleAsyncJobEngine.h SimpleAsyncJobEngine.cpp \
					MultiThreadJobEngine.h MultiThreadJobEngine.cpp

AM_CPPFLAGS=@libbrot2_CFLAGS@
AM_CFLAGS=@BROT2_CFLAGS@
AM_CXXFLAGS=@BROT2_CXXFLAGS@

CLEANFILES=$(AUTOGEN)

