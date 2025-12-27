#ifdef ONECORE_FREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct oc_library_s {
    FT_Library ft_library;
} oc_library;

typedef struct oc_face_s {
    FT_Face ft_face;
} oc_face;

#endif // ONECORE_FREETYPE
