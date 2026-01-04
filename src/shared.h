#ifndef ONECORE_SHARED_H_
#define ONECORE_SHARED_H_

#include <assert.h>
#include <inttypes.h>
#include <onecore.h>
#include <stdio.h>

#if defined(_WIN32) || defined(__CYGWIN__)
// perhaps use SRWLOCK as i do not need safe recursion for locks
#include <windows.h>
typedef CRITICAL_SECTION mutex_t;

static inline void mutex_init(mutex_t* m) {
    InitializeCriticalSection(m);
}

static inline void mutex_lock(mutex_t* m) {
    EnterCriticalSection(m);
}

static inline void mutex_unlock(mutex_t* m) {
    LeaveCriticalSection(m);
}

static inline void mutex_destroy(mutex_t* m) {
    DeleteCriticalSection(m);
}

#else
#include <pthread.h>
typedef pthread_mutex_t mutex_t;

static inline void mutex_init(mutex_t* m) {
    pthread_mutex_init(m, NULL);
}

static inline void mutex_lock(mutex_t* m) {
    pthread_mutex_lock(m);
}

static inline void mutex_unlock(mutex_t* m) {
    pthread_mutex_unlock(m);
}

static inline void mutex_destroy(mutex_t* m) {
    pthread_mutex_destroy(m);
}

#endif

#if !defined(ONECORE_DWRITE) && !defined(ONECORE_FREETYPE) && !defined(ONECORE_CORETEXT)
#if defined(_WIN32) || defined(__CYGWIN__)
#define ONECORE_DWRITE
#elif defined(__APPLE__) && defined(__MACH__)
#define ONECORE_CORETEXT
#else
#define ONECORE_FREETYPE
#endif
#endif

#ifdef NDEBUG
#define unexpected(err) (oc_error_unexpected)
#else

static inline oc_error __unexpected(int64_t err, const char* file, int line) {
    fprintf(stderr, "%s:%d: unexpected error: %" PRId64 "\n", file, line, err);
    return oc_error_unexpected;
}

#define unexpected(err) __unexpected((int64_t)err, __FILE__, __LINE__)
#endif

#endif // ONECORE_SHARED_H_
