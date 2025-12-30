
#ifdef ONECORE_CORETEXT

#include <CoreText/CoreText.h>

typedef struct oc_library_s {
} oc_library;

typedef struct oc_face_s {
    CGFontRef ct_font_ref;
} oc_face;

#endif // ONECORE_CORETEXT
