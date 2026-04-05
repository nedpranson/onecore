#define ONECORE_FINDER_IMPLEMENTATION
//#define ONECORE_LOADER_IMPLEMENTATION
#include "onecore.h"

int main() {
    oc_library lib;
    oc_init_library(&lib);

    oc_collection col;
    ocf_init_collection(&lib, &col);
    ocf_load_fonts(&col);

    for (uint32_t i = 0; i < col.nfonts; i++) {
        printf("%s: %d\n", col.fonts[i]->family, col.fonts[i]->weight);
    }

    ocf_free_collection(&col);
    oc_free_library(&lib);
}
