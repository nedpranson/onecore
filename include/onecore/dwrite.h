#ifdef ONECORE_DWRITE

// clang-format off
#include <initguid.h>
#include <dwrite.h>
#include <d2d1.h>
// clang-format on

typedef struct oc_library_s {
    IDWriteFactory* dw_factory;
} oc_library;

typedef struct oc_face_s {
    IDWriteFontFace* dw_font_face;
} oc_face;

#endif // ONECORE_DWRITE
