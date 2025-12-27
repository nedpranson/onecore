#ifdef ONECORE_DWRITE

#include <initguid.h>
#include <dwrite.h>

typedef struct oc_library_s {
  IDWriteFactory* dw_factory;
} oc_library;

typedef struct oc_face_s {
  IDWriteFontFace* dw_font_face;
} oc_face;

#endif // ONECORE_DWRITE
