/**
 * Copyright (C) 2013-2014 Huawei Technologies Finland Oy.
 *
 * This is unpublished work. See README file for more information.
 */

#ifndef LIBSEPPOCOMMON_THREAD
#define LIBSEPPOCOMMON_THREAD

#include "seppo/common/Class.hpp"
#include "seppo/common/Lock.hpp"
#include <pthread.h>
#include <string>
#include <signal.h>

namespace seppo
{
    struct ThreadCToCppWrapper;

    /**
     * @class Thread
     *
     * Thread is a base class for thread classes.
     */
    class Thread
    {
    public:
        CLASS_UNCOPYABLE(Thread)

        void start(bool wait = true);
        void join();
        void sigint();

        static void exit();

        pthread_t id();

        static sigset_t mask(int sigToMask, sigset_t &sigpipeMask); // set mask & return old signal set
        static void restore(sigset_t saved, sigset_t &sigpipeMask, int sigMasked); // restore saved and clear masked signals

    protected:
        virtual void run() = 0;
        Thread();
        virtual ~Thread();

    private:
        pthread_t thread_;
        friend struct ThreadCToCppWrapper;
        std::mutex mtx_;
        volatile bool running_;
    };

    struct ThreadCToCppWrapper
    {
        static void protectedCppRun(Thread* other)
        {
            Lock lock(other->mtx_);
            other->running_ = true;
            other->run();
        }
    };

    class MaskSignal
    {
    public:
        CLASS_UNCOPYABLE(MaskSignal);

        MaskSignal(int sigToMask)
        {
            sig_ = sigToMask;
            saved_ = Thread::mask(sig_, masked_);
        }

        ~MaskSignal()
        {
            Thread::restore(saved_, masked_, sig_);
        }

    private:
        sigset_t saved_;
        sigset_t masked_;
        int sig_;
    };
}

#endif
