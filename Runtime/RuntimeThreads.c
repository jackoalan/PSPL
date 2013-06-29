//
//  RuntimeThreads.c
//  PSPL
//
//  Created by Jack Andersen on 6/24/13.
//
//

#include <PSPL/PSPLRuntimeThreads.h>

#if defined(PSPL_THREADING_GCD)

static const char* api_load_states = "pspl_api_load_state";
static const char* api_load_subject_indices = "pspl_api_load_subject_index";

int pspl_thread_fork(void(*func)(void*), void* usr_ptr) {
    dispatch_queue_t queue = dispatch_queue_create(NULL, NULL);
    dispatch_queue_set_specific(queue, api_load_states, (void*)pspl_api_load_state(), NULL);
    dispatch_queue_set_specific(queue, api_load_subject_indices, (void*)pspl_api_load_subject_index(), NULL);
    dispatch_async_f(queue, usr_ptr, func);
    dispatch_release(queue);
    return 0;
}


/* Thread-specific values (set on fork) */
void pspl_api_set_load_state(intptr_t state) {
    dispatch_queue_set_specific(dispatch_get_main_queue(), api_load_states, (void*)state, NULL);
}
void pspl_api_set_load_subject_index(intptr_t index) {
    dispatch_queue_set_specific(dispatch_get_main_queue(), api_load_subject_indices, (void*)index, NULL);
}

intptr_t pspl_api_load_state() {
    return (intptr_t)dispatch_get_specific(api_load_states);
}
intptr_t pspl_api_load_subject_index() {
    return (intptr_t)dispatch_get_specific(api_load_subject_indices);
}


#elif defined(PSPL_THREADING_PTHREAD)

static pthread_key_t api_load_states = 0;
static pthread_key_t api_load_subject_indices = 0;

struct thread {
    void(*func)(void*);
    void* usr_ptr;
    intptr_t states;
    intptr_t indices;
};
static void* run_thread(void* thread) {
    pthread_setspecific(api_load_states, (void*)((struct thread*)thread)->states);
    pthread_setspecific(api_load_subject_indices, (void*)((struct thread*)thread)->indices);
    ((struct thread*)thread)->func(((struct thread*)thread)->usr_ptr);
    return NULL;
}
int pspl_thread_fork(void(*func)(void*), void* usr_ptr) {
    struct thread th = {
        .func = func,
        .usr_ptr = usr_ptr,
        .states = pspl_api_load_state(),
        .indices = pspl_api_load_subject_index()
    };
    return pthread_create(NULL, NULL, run_thread, &th);
}


/* Thread-specific values (set on fork) */
void pspl_api_set_load_state(intptr_t state) {
    if (!api_load_states)
        pthread_key_create(&api_load_states, NULL);
    pthread_setspecific(api_load_states, (void*)state);
}
void pspl_api_set_load_subject_index(intptr_t index) {
    if (!api_load_subject_indices)
        pthread_key_create(&api_load_subject_indices, NULL);
    pthread_setspecific(api_load_subject_indices, (void*)index);
}

intptr_t pspl_api_load_state() {
    return (intptr_t)pthread_getspecific(api_load_states);
}
intptr_t pspl_api_load_subject_index() {
    return (intptr_t)pthread_getspecific(api_load_subject_indices);
}


#elif defined(PSPL_THREADING_OGC)
#include <ogc/lwp_config.h>

static intptr_t api_load_states[LWP_MAX_THREADS+1];
static intptr_t api_load_subject_indices[LWP_MAX_THREADS+1];

struct thread {
    void(*func)(void*);
    void* usr_ptr;
    unsigned last_thread;
};
#define STACK_SIZE 16384
static void* run_thread(void* thread) {
    api_load_states[LWP_OBJMASKID(LWP_GetSelf())] = api_load_states[((struct thread*)thread)->last_thread];
    api_load_subject_indices[LWP_OBJMASKID(LWP_GetSelf())] = api_load_subject_indices[((struct thread*)thread)->last_thread];
    ((struct thread*)thread)->func(((struct thread*)thread)->usr_ptr);
    u32 level = IRQ_Disable();
    void* hi = SYS_GetArenaHi();
    hi += STACK_SIZE;
    SYS_SetArenaHi(hi);
    IRQ_Restore(level);
    return NULL;
}
int pspl_thread_fork(void(*func)(void*), void* usr_ptr) {
    u32 level = IRQ_Disable();
    if (SYS_GetArenaSize() < STACK_SIZE)
        return -1;
    void* hi = SYS_GetArenaHi();
    hi -= STACK_SIZE;
    SYS_SetArenaHi(hi);
    IRQ_Restore(level);
    struct thread th = {
        .func = func,
        .usr_ptr = usr_ptr,
        .last_thread = LWP_OBJMASKID(LWP_GetSelf())
    };
    return LWP_CreateThread(NULL, run_thread, &th, hi, STACK_SIZE, 64);
}


/* Thread-specific values (set on fork) */
void pspl_api_set_load_state(intptr_t state) {
    api_load_states[LWP_OBJMASKID(LWP_GetSelf())] = state;
}
void pspl_api_set_load_subject_index(intptr_t index) {
    api_load_subject_indices[LWP_OBJMASKID(LWP_GetSelf())] = index;
}

intptr_t pspl_api_load_state() {
    return api_load_states[LWP_OBJMASKID(LWP_GetSelf())];
}
intptr_t pspl_api_load_subject_index() {
    return api_load_subject_indices[LWP_OBJMASKID(LWP_GetSelf())];
}


#elif defined(PSPL_THREADING_WIN32)
#include <winbase.h>

static DWORD api_load_states = 0;
static DWORD api_load_subject_indices = 0;

struct thread {
    void(*func)(void*);
    void* usr_ptr;
    intptr_t states;
    intptr_t indices;
};
static DWORD run_thread(LPVOID thread) {
    TlsSetValue(api_load_states, (LPVOID)((struct thread*)thread)->states);
    TlsSetValue(api_load_subject_indices, (LPVOID)((struct thread*)thread)->indices);
    ((struct thread*)thread)->func(((struct thread*)thread)->usr_ptr);
    return 0;
}
int pspl_thread_fork(void(*func)(void*), void* usr_ptr) {
    struct thread th = {
        .func = func,
        .usr_ptr = usr_ptr,
        .states = pspl_api_load_state(),
        .indices = pspl_api_load_subject_index()
    };
    if (!CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)run_thread, &th, 0, NULL))
        return -1;
    return 0;
}


/* Thread-specific values (set on fork) */
void pspl_api_set_load_state(intptr_t state) {
    if (!api_load_states)
        api_load_states = TlsAlloc();
    TlsSetValue(api_load_states, (LPVOID)state);
}
void pspl_api_set_load_subject_index(intptr_t index) {
    if (!api_load_subject_indices)
        api_load_subject_indices = TlsAlloc();
    TlsSetValue(api_load_subject_indices, (LPVOID)index);
}

intptr_t pspl_api_load_state() {
    return (intptr_t)TlsGetValue(api_load_states);
}
intptr_t pspl_api_load_subject_index() {
    return (intptr_t)TlsGetValue(api_load_subject_indices);
}


#endif
