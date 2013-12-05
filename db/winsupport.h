#include <Windows.h>

typedef HANDLE pthread_t;
typedef HANDLE pthread_mutex_t;

typedef struct
{
    int waiters_count_;
    // Number of waiting threads.

    CRITICAL_SECTION waiters_count_lock_;
    // Serialize access to <waiters_count_>.

    HANDLE sema_;
    // Semaphore used to queue up threads waiting for the condition to
    // become signaled.

    HANDLE waiters_done_;
    // An auto-reset event used by the broadcast/signal thread to wait
    // for all the waiting thread(s) to wake up and be released from the
    // semaphore.

    size_t was_broadcast_;
    // Keeps track of whether we were broadcasting or signaling.  This
    // allows us to optimize the code if we're just signaling.
} pthread_cond_t;

int pthread_cond_init(pthread_cond_t *cv, const void *attr);
int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *external_mutex);
int pthread_cond_signal(pthread_cond_t *cv);
int pthread_cond_broadcast(pthread_cond_t *cv);
int pthread_cond_destroy(pthread_cond_t *cv);

int pthread_mutex_init(pthread_mutex_t *, const void *attr);
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *);
int pthread_mutex_destroy(pthread_mutex_t *);

int pthread_create(pthread_t * thread, const void * attr, void *(*start_routine)(void *), void * arg);
int pthread_join(pthread_t thread, void **retval);

int sp_win_crt2windowsDisposition(int flags);
int sp_win_crt2windowsMode(int flags);
int sp_win_openfile(char* path, int flags);
int sp_win_openfileshare(char* path, int flags, int share);

char __sync_lock_test_and_set(volatile char * a, char v);
void __sync_lock_release(volatile char * a);

