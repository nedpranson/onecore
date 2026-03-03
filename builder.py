#!/usr/bin/env python3
import pathlib

output_path = pathlib.Path("onecore.h")
source_dir = pathlib.Path("src")

onecore_h = source_dir / "onecore.h"

sources = [
    (source_dir / "shared.c", "ONECORE_IMPLEMENTATION"),
    (source_dir / "freetype.c", "ONECORE_FREETYPE_IMPLEMENTATION"),
    (source_dir / "dwrite.c", "ONECORE_DIRECTWRITE_IMPLEMENTATION"),
]

def strip(path, marker):
    with path.open("r", encoding="utf-8") as file:
        for line in file:
            if line.strip() == f"/* {marker} */":
                return file.read()

def concat(source, marker, body):
    begin = source.find(f"#ifdef {marker}")
    end = source.find(f"#endif /* {marker} */")

    header = source[:begin]
    footer = source[end:]
    
    return f"{header}#ifdef {marker}\n{body}{footer}"

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
