# onecore.h

Provides an abstracted API over native OS font handling.
The premise of this library is to produce deterministic results when loading or finding fonts.

[onecore.h](https://github.com/nedpranson/onecore/blob/master/onecore.h) provides:
- A way to iterate over system fonts.
- FreeType-like text loading and handling for OpenType fonts.
- Glyph rasterization.

## Usage

Add [onecore.h](https://github.com/nedpranson/onecore/blob/master/onecore.h) to your project and
select exactly one C/C++ source file to instantiate the code.

```c
/* optional finder implementation */
#define ONECORE_FINDER_IMPLEMENTATION
/* optional loader implementation */
#define ONECORE_LOADER_IMPLEMENTATION
#include "onecore.h"
```

```c
void example() {
    oc_library lib;
    oc_init_library(&lib);

    oc_collection col;
    ocf_init_collection(&lib, &col);
    ocf_load_fonts(&col);

     

    ocf_free_collection(&col);
    oc_free_library(&lib);
}
```

// for more docs read the sourcef file
