#ifndef ONECORE_LIBRARY_H_
#define ONECORE_LIBRARY_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#define OC_EXPORT __declspec(dllexport)
#else
#if __GNUC__ >= 4
#define OC_EXPORT __attribute__((visibility("default")))
#else
#define OC_EXPORT
#endif
#endif

#if !defined(ONECORE_DWRITE) && !defined(ONECORE_FREETYPE)
#if defined(_WIN32) || defined(__CYGWIN__)
#define ONECORE_DWRITE
#elif defined(__APPLE__) && defined(__MACH__)
#define ONECORE_CORETEXT
#else
#define ONECORE_FREETYPE
#endif
#endif

#include <stddef.h>
#include <stdint.h>

typedef enum {
    oc_error_ok,
    oc_error_invalid_param,
    oc_error_failed_to_open,
    oc_error_table_missing,
    oc_error_out_of_memory,
    oc_error_unexpected,
} oc_error;

#include "onecore/coretext.h"
#include "onecore/dwrite.h"
#include "onecore/freetype.h"

typedef uint32_t oc_tag;

typedef struct oc_table_s {
    const uint8_t* buffer;
    size_t size;

    void* __handle;
} oc_table;

#define OC_MAKE_TAG(x1, x2, x3, x4) \
    (((uint8_t)x1) << 24 | ((uint8_t)x2) << 16 | ((uint8_t)x3) << 8 | ((uint8_t)x4))

OC_EXPORT oc_error
oc_library_init(oc_library* plibrary);

OC_EXPORT void
oc_library_free(oc_library library);

OC_EXPORT oc_error
oc_face_new(oc_library library, const char* path, long face_index, oc_face* pface);

OC_EXPORT void
oc_face_free(oc_face face);

OC_EXPORT uint16_t
oc_face_get_char_index(oc_face face, uint32_t charcode);

OC_EXPORT oc_error
oc_face_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable);

OC_EXPORT void
oc_table_free(oc_table table);

OC_EXPORT void
oc_face_get_metrics(oc_face face);

#ifdef __cplusplus
}
#endif

#endif // ONECORE_LIBRARY_H_
