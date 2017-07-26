/**
 * Copyright (C) 2013-2014 Huawei Technologies Finland Oy.
 *
 * This is unpublished work. See README file for more information.
 */

#include "seppo/common/Thread.hpp"
#include "seppo/common/Log.hpp"
#include "seppo/common/Exception.hpp"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdexcept>
#include <signal.h>

namespace seppo
{
    void* thread_function(void* instance)
    {
        try
        {
            Thread* tmp = (Thread*)instance;
            ThreadCToCppWrapper::protectedCppRun(tmp);
        }
        catch (Exception const& e)
        {
            LOGE("Exception inside thread! %s\n", e.what());
        }
        catch (...)
        {
            LOGE("Exception inside thread!\n");
        }

        return NULL;
    }

    Thread::Thread():
        running_(false)
    {
    }

    Thread::~Thread()
    {
    }

    void Thread::start(bool wait)
    {
        pthread_attr_t type;
        if (pthread_attr_init(&type) != 0)
        {
            throw Exception("pthread_attr_init failed");
        }
        pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);

        int result = pthread_create(&thread_, &type, thread_function, (void*)this);

        pthread_attr_destroy(&type);

        if (result == EAGAIN)
        {
            throw Exception("Thread::start, errno:EAGAIN");
        }
        else if (result == EINVAL)
        {
            throw Exception("Thread::start, errno:EINVAL");
        }
        else if (result == EPERM)
        {
            throw Exception("Thread::start, errno:EPERM");
        }

        while(wait && !running_)
        {
            usleep(1);
        }
    }

    void Thread::join()
    {
        /*
        int result = pthread_join(thread_, NULL);
        if (result == EINVAL)
        {
            throw Exception("Thread::join, errno:EINVAL");
        }
        else if (result == ESRCH)
        {
            throw Exception("Thread::join, errno:ESRCH");
        }
        else if (result == EDEADLK)
        {
            throw Exception("Thread::join, errno:EDEADLK");
        }
        */

        Lock lock(mtx_);
    }

    void Thread::sigint()
    {
        pthread_kill(thread_, SIGINT);
    }

    pthread_t Thread::id()
    {
        return thread_;
    }

    void Thread::exit()
    {
        // exit calling thread
        pthread_exit(NULL);
    }

    sigset_t Thread::mask(int sigToMask, sigset_t &sigpipeMask)
    {
        sigemptyset(&sigpipeMask);
        sigaddset(&sigpipeMask, sigToMask);
        sigset_t saved;

        if(pthread_sigmask(SIG_BLOCK, &sigpipeMask, &saved) == -1){
            LOGE("pthread_sigmask: Can't mask signal %i", sigToMask);
        }

        return saved;
    }

    void Thread::restore(sigset_t saved, sigset_t& /*sigpipeMask*/, int sigMasked)
    {
        sigset_t pending;
        sigpending(&pending);

        if(sigismember(&pending, sigMasked)){

            LOGI("Signal %i pending, ignore it", sigMasked);

            struct sigaction ignore, old;

            ignore.sa_handler = SIG_IGN;
            ignore.sa_flags = 0;
            sigemptyset(&ignore.sa_mask);

            if (sigaction(sigMasked, &ignore, &old) < 0) {
                LOGE("sigaction: Couldn't ignore");
            }

            if (sigaction(sigMasked, &old, NULL) < 0) {
                LOGE("sigaction: Couldn't restore default behavior");
            }
        }

        if(pthread_sigmask(SIG_SETMASK, &saved, 0) == -1){
            LOGE("pthread_sigmask: Can't restore signal mask");
        }
    }
}
