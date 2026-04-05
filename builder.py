#!/usr/bin/env python3
import pathlib

output_path = pathlib.Path("onecore.h")
source_dir = pathlib.Path("src")

onecore_h = source_dir / "onecore.h"

sources = [
    (source_dir / "onecore.c", "ONECORE_IMPLEMENTATION"),
    (source_dir / "loader/freetype.c", "ONECORE_FREETYPE_LOADER_IMPLEMENTATION"),
    (source_dir / "loader/coretext.c", "ONECORE_CORETEXT_LOADER_IMPLEMENTATION"),
    (source_dir / "loader/dwrite.c", "ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION"),
    (source_dir / "finder/fontconfig.c", "ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION"),
    (source_dir / "finder/coretext.c", "ONECORE_CORETEXT_FINDER_IMPLEMENTATION"),
    (source_dir / "finder/dwrite.c", "ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION"),
]

def strip(path, marker):
    with path.open("r", encoding="utf-8") as file:
        for line in file:
            if line.strip() == f"/* {marker} */":
                return file.read()

def concat(source, marker, body):
    header = source.find(f"#ifdef {marker}")
    pos = source.find('\n', header) + 1

    header = source[:pos]
    footer = source[pos:]
    
    return f"{header}{body}{footer}"

def inject(source, file, marker):
    body = strip(file, marker)
    return concat(source, marker, body)

body = onecore_h.read_text(encoding="utf-8", newline=None)

for path, marker in sources:
    body = inject(body, path, marker)

output_path.write_text(
    body,
    encoding="utf-8",
    newline="\n",
)
