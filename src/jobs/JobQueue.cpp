#include "JobQueue.h"

#include "Worker.h"

// TODO on gcc define it as:
//#define COMPILTER_BARRIER asm volatile ("" : : : "memory")
#define COMPILER_BARRIER _ReadWriteBarrier()
// TODO on gcc define it as:
//#define MEMORY_BARRIER __sync_synchronize
#include "Windows.h"
#define MEMORY_BARRIER MemoryBarrier()

using namespace vecs;

// Full disclosure the compiler and memory barriers are a very new concept for me,
// and I've put them where they pretty much solely because that's where molecular matters did
// I've copied the comments explaining why they belong where they do from the blog post as well
// Reference: https://blog.molecular-matters.com/2015/09/25/job-system-2-0-lock-free-work-stealing-part-3-going-lock-free/
// tbh I wish there was more I could do specific to my implementation to demonstrate that I
// understood the article and didn't just copy+paste the code. I only kept the comments too because they're solid
// at explaining (and for more in depth explanations of course just visit the reference), but added my own where
// I felt appropriate.
// He didn't show any header files so at least I can show I understood the concept enough to handle that part,
// plus the barrier define statements (which I had to do separate research on)
void JobQueue::push(Job* job) {
    // store bottom in local variable so we don't have to worry about it changing mid-function call
    long b = bottom;
    long t = top;

    if (b - t > MASK) {
        // We have a situation where the queue's ring has completely circled, and there's no more room for this job
        // We'll instead just complete this job now. This may cause a stack overflow if enough jobs keep pushing more
        // jobs, at which point it'd probably be best to increase the max job count. Hopefully by time this job finishes
        // synchronously, the queue will have been stolen from enough that future pushes will be handled properly
        // Note we store the current job being processed so we can re-set it after worker->work(job) resets it
        Job* currentJob = worker->job;
        worker->work(job);
        worker->job = currentJob;
        return;
    }

    queue[b & MASK] = job;

    // ensure the job is written before b+1 is published to other threads.
    // on x86/64, a compiler barrier is enough.
    COMPILER_BARRIER;

    // writing back to bottom can be done safely because the only function that also writes to bottom
    // is Pop(), which can only be run by the thread running Push(Job*) (so not concurrently)
    // Even if Steal() reads from bottom before we write it back the worst case is Steal() returns
    // no job until the next time its called after this line
    bottom = b + 1;
}

Job* JobQueue::steal() {
    // store top in local variable so we don't have to worry about it changing mid-function call
    long t = top;

    // ensure that top is always read before bottom.
    // loads will not be reordered with other loads on x86, so a compiler barrier is enough.
    COMPILER_BARRIER;

    // store bottom for the same reason as top
    long b = bottom;
    if (t < b) {
        // non-empty queue
        Job* job = queue[t & MASK];

        // the interlocked function serves as a compiler barrier, and guarantees that the read happens before the CAS.
        if (_InterlockedCompareExchange(&top, t + 1, t) != t) {
            // a concurrent steal or pop operation removed an element from the deque in the meantime.
            return nullptr;
        }

        return job;
    } else {
        // empty queue
        return nullptr;
    }
}

Job* JobQueue::pop() {
    long b = bottom - 1;
    bottom = b;

    MEMORY_BARRIER;

    long t = top;
    if (t <= b) {
        // non-empty queue
        Job* job = queue[b & MASK];
        if (t != b) {
            // there's still more than one item left in the queue
            return job;
        }

        // this is the last item in the queue
        if (_InterlockedCompareExchange(&top, t + 1, t) != t) {
            // failed race against steal operation
            job = nullptr;
        }

        bottom = t + 1;
        return job;
    } else {
        // deque was already empty
        bottom = t;
        return nullptr;
    }
}
