#ifndef ONECORE_UNEXPECTED_H_
#define ONECORE_UNEXPECTED_H_

#include <inttypes.h>
#include <stdio.h>

#ifdef NDEBUG
#define unexpected(err) (oc_error_unexpected)
#else

static inline oc_error __unexpected(int64_t err, const char* file, int line) {
    fprintf(stderr, "%s:%d: unexpected error: %" PRId64 "\n", file, line, err);
    return oc_error_unexpected;
}

#define unexpected(err) __unexpected((int64_t)err, __FILE__, __LINE__)
#endif

#endif // ONECORE_UNEXPECTED_H_
