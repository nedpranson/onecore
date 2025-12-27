#ifdef ONECORE_DWRITE

#include <initguid.h>
#include <dwrite.h>

typedef struct oc_library_s {
  IDWriteFactory* dw_factory;
} oc_library;

#endif // ONECORE_DWRITE
