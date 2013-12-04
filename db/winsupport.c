#include "winsupport.h"
#include <fcntl.h>
#include <io.h>

int pthread_cond_init(pthread_cond_t *cv, const void *t)
{
    cv->waiters_count_ = 0;
    cv->was_broadcast_ = 0;
    cv->sema_ = CreateSemaphore(NULL,       // no security
        0,          // initially 0
        0x7fffffff, // max count
        NULL);      // unnamed 
    InitializeCriticalSection(&cv->waiters_count_lock_);
    cv->waiters_done_ = CreateEvent(NULL,  // no security
        FALSE, // auto-reset
        FALSE, // non-signaled initially
        NULL); // unnamed
    return 0;
}


int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *external_mutex)
{
    // Avoid race conditions.
    EnterCriticalSection(&cv->waiters_count_lock_);
    cv->waiters_count_++;
    LeaveCriticalSection(&cv->waiters_count_lock_);

    // This call atomically releases the mutex and waits on the
    // semaphore until <pthread_cond_signal> or <pthread_cond_broadcast>
    // are called by another thread.
    SignalObjectAndWait(*external_mutex, cv->sema_, INFINITE, FALSE);

    // Reacquire lock to avoid race conditions.
    EnterCriticalSection(&cv->waiters_count_lock_);

    // We're no longer waiting...
    cv->waiters_count_--;

    // Check to see if we're the last waiter after <pthread_cond_broadcast>.
    int last_waiter = cv->was_broadcast_ && cv->waiters_count_ == 0;

    LeaveCriticalSection(&cv->waiters_count_lock_);

    // If we're the last waiter thread during this particular broadcast
    // then let all the other threads proceed.
    if (last_waiter)
        // This call atomically signals the <waiters_done_> event and waits until
        // it can acquire the <external_mutex>.  This is required to ensure fairness. 
        SignalObjectAndWait(cv->waiters_done_, *external_mutex, INFINITE, FALSE);
    else
        // Always regain the external mutex since that's the guarantee we
        // give to our callers. 
        WaitForSingleObject(*external_mutex, INFINITE);
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cv)
{
    EnterCriticalSection(&cv->waiters_count_lock_);
    int have_waiters = cv->waiters_count_ > 0;
    LeaveCriticalSection(&cv->waiters_count_lock_);

    // If there aren't any waiters, then this is a no-op.  
    if (have_waiters)
        ReleaseSemaphore(cv->sema_, 1, 0);
    return 0;
}


int pthread_cond_broadcast(pthread_cond_t *cv)
{
    // This is needed to ensure that <waiters_count_> and <was_broadcast_> are
    // consistent relative to each other.
    EnterCriticalSection(&cv->waiters_count_lock_);
    int have_waiters = 0;

    if (cv->waiters_count_ > 0) {
        // We are broadcasting, even if there is just one waiter...
        // Record that we are broadcasting, which helps optimize
        // <pthread_cond_wait> for the non-broadcast case.
        cv->was_broadcast_ = 1;
        have_waiters = 1;
    }

    if (have_waiters) {
        // Wake up all the waiters atomically.
        ReleaseSemaphore(cv->sema_, cv->waiters_count_, 0);

        LeaveCriticalSection(&cv->waiters_count_lock_);

        // Wait for all the awakened threads to acquire the counting
        // semaphore. 
        WaitForSingleObject(cv->waiters_done_, INFINITE);
        // This assignment is okay, even without the <waiters_count_lock_> held 
        // because no other waiter threads can wake up to access it.
        cv->was_broadcast_ = 0;
    }
    else
        LeaveCriticalSection(&cv->waiters_count_lock_);
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cv){
    CloseHandle(cv->sema_);
    CloseHandle(cv->waiters_done_);
    DeleteCriticalSection(&cv->waiters_count_lock_);
    return 0;
}

int pthread_mutex_init(pthread_mutex_t *l, const void * attr)
{
    *l = CreateMutex(NULL, 0, NULL);
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *l)
{
    WaitForSingleObject(*l, INFINITE);
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *l)
{
    ReleaseMutex(*l);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *l)
{
    CloseHandle(*l);
    return 0;
}

int pthread_create(pthread_t * thread, const void * attr, void *(*start_routine)(void *), void * arg)
{
    DWORD id;
    *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, &id);
    return 0;
}

int pthread_join(pthread_t thread, void **retval)
{
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    if (retval)
        retval = 0;
    return 0;
}
int sp_win_crt2windowsDisposition(int flags) {
    int disposition;
    if ((flags & _O_CREAT) == _O_CREAT) {
        if ((flags & _O_EXCL) == _O_EXCL) {
            disposition = CREATE_NEW;
        }
        else if ((flags & _O_TRUNC) == _O_TRUNC) {
            disposition = CREATE_ALWAYS;
        }
        else
            disposition = OPEN_ALWAYS;
    }
    else if ((flags & _O_TRUNC) == _O_TRUNC) {
        disposition = TRUNCATE_EXISTING;
    }
    else
        disposition = OPEN_EXISTING;
    return disposition;
}

int sp_win_crt2windowsMode(int flags) {
    int mode;
    if ((flags & _O_RDWR) == _O_RDWR)
        mode = GENERIC_WRITE | GENERIC_READ;
    else if ((flags & _O_WRONLY) == _O_WRONLY)
        mode = GENERIC_WRITE;
    else
        mode = GENERIC_READ;
    return mode;
}

int sp_win_openfileshare(char* path, int flags, int share)
{
    HANDLE h = CreateFileA(path, sp_win_crt2windowsMode(flags), share, NULL, sp_win_crt2windowsDisposition(flags), FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return -1;
    return _open_osfhandle((intptr_t)h, flags);
}

int sp_win_openfile(char* path, int flags)
{
    return sp_win_openfileshare(path, flags, FILE_SHARE_DELETE);
}

