//
//  PSPLRuntimeThreads.h
//  PSPL
//
//  Created by Jack Andersen on 6/24/13.
//
//

/* A simple, fork-based, common multi-threading API with mutexes */

#ifndef PSPL_PSPLRuntimeThreads_h
#define PSPL_PSPLRuntimeThreads_h

#include <stdint.h>

#if defined(PSPL_THREADING_GCD)
#include <dispatch/dispatch.h>
typedef dispatch_semaphore_t pspl_mutex_t;
#define pspl_mutex_init(mutex) *mutex = dispatch_semaphore_create(1)
#define pspl_mutex_lock(mutex) dispatch_semaphore_wait(*mutex, DISPATCH_TIME_FOREVER)
#define pspl_mutex_trylock(mutex) dispatch_semaphore_wait(*mutex, DISPATCH_TIME_NOW)
#define pspl_mutex_unlock(mutex) dispatch_semaphore_signal(*mutex)
#define pspl_mutex_destroy(mutex) dispatch_release(*mutex)

#elif defined(PSPL_THREADING_PTHREAD)
#include <pthread.h>
typedef pthread_mutex_t pspl_mutex_t;
#define pspl_mutex_init(mutex) pthread_mutex_init(mutex, NULL)
#define pspl_mutex_lock(mutex) pthread_mutex_lock(mutex)
#define pspl_mutex_trylock(mutex) pthread_mutex_trylock(mutex)
#define pspl_mutex_unlock(mutex) pthread_mutex_unlock(mutex)
#define pspl_mutex_destroy(mutex) pthread_mutex_destroy(mutex)

#elif defined(PSPL_THREADING_OGC)
#include <ogc/lwp.h>
#include <ogc/mutex.h>
typedef mutex_t pspl_mutex_t;
#define pspl_mutex_init(mutex) LWP_MutexInit(mutex, 0)
#define pspl_mutex_lock(mutex) LWP_MutexLock(*mutex)
#define pspl_mutex_trylock(mutex) LWP_MutexTryLock(*mutex)
#define pspl_mutex_unlock(mutex) LWP_MutexUnlock(*mutex)
#define pspl_mutex_destroy(mutex) LWP_MutexDestroy(*mutex)

#elif defined(PSPL_THREADING_WIN32)
#include <winbase.h>
typedef HANDLE pspl_mutex_t;
#define pspl_mutex_init(mutex) *mutex = CreateMutex(NULL, FALSE, NULL)
#define pspl_mutex_lock(mutex) WaitForSingleObject(*mutex, INFINITE)
#define pspl_mutex_trylock(mutex) WaitForSingleObject(*mutex, 0)
#define pspl_mutex_unlock(mutex) SignalObjectAndWait(*mutex, NULL, 0, FALSE)
#define pspl_mutex_destroy(mutex) ReleaseMutex(*mutex)

#endif

int pspl_thread_fork(void(*func)(void*), void* usr_ptr);

/* Thread-specific values (set on fork) */
intptr_t pspl_api_load_state();
intptr_t pspl_api_load_subject_index();

#endif

