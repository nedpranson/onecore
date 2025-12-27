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

typedef enum {
  oc_error_ok,
  oc_error_invalid_param,
  oc_error_failed_to_open,
  oc_error_out_of_memory,
  oc_error_unexpected,
} oc_error;

#include "onecore/freetype.h"
#include "onecore/dwrite.h"
#include "onecore/coretext.h"

OC_EXPORT oc_error
oc_library_init(oc_library *plibrary);

OC_EXPORT void
oc_library_free(oc_library library);

OC_EXPORT oc_error
oc_face_new(oc_library library, const char* path, long face_index, oc_face *pface);

OC_EXPORT void
oc_face_free(oc_face face);

#ifdef __cplusplus
}
#endif

#endif // ONECORE_LIBRARY_H_
