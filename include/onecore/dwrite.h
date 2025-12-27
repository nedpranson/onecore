#ifdef ONECORE_DWRITE

#include <dwrite.h>

typedef struct oc_library_s {
  IDWriteFactory* dw_factory;
} oc_library;

#endif // ONECORE_DWRITE
