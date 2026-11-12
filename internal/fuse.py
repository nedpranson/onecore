#!/usr/bin/env python3
"""
fuse.py - Single-header C/C++ amalgamation tool using '@fuse' comment metadata.

Usage:
    python fuse.py <header_path> <source_dirs_or_files...> [-o <output_path>] [--prefix <PREFIX>]
"""

import argparse
import pathlib
import re
import sys


def parse_fuse_block(content: str, file_path: pathlib.Path):
    """
    Looks for the unique /* ... @fuse ... */ metadata comment in the file.
    The @fuse block acts as an opening marker for dev-only boilerplate (like includes),
    which is closed by the first /* ---- */. Everything in between is stripped.
    """
    fuse_matches = list(re.finditer(r'/\*[\s\*]*@fuse\b.*?\*/', content, re.DOTALL))
    if len(fuse_matches) > 1:
        sys.exit(f"Error in {file_path}: Multiple @fuse comment blocks found.")
    if not fuse_matches:
        return None, content

    block = fuse_matches[0]
    block_text = block.group(0)

    # The @fuse comment acts as the opening cut-out marker.
    # Look for the closing /* ---- */ that follows it.
    after_fuse_idx = block.end()
    dash_match = re.search(r'/\*\s*----\s*\*/', content[after_fuse_idx:])

    if dash_match:
        # Strip from start of @fuse comment through the closing /* ---- */
        end_idx = after_fuse_idx + dash_match.end()
        content_without_header = content[:block.start()] + content[end_idx:]
    else:
        # No closing /* ---- */, just remove the @fuse block itself
        content_without_header = content[:block.start()] + content[after_fuse_idx:]

    # Also strip any additional standalone /* ---- */ ... /* ---- */ cut-out blocks
    cleaned_body = re.sub(r'/\*\s*----\s*\*\/.*?/\*\s*----\s*\*/', '', content_without_header, flags=re.DOTALL)
    cleaned_body = cleaned_body.strip()

    # Parse metadata tags from the @fuse block text
    kind = None
    name = None
    rank = None
    deps = []
    current_tag = None

    for raw_line in block_text.splitlines():
        line = raw_line.strip()
        # Strip leading comment markers and whitespace
        if line.startswith("/*"):
            line = line[2:].strip()
        if line.endswith("*/"):
            line = line[:-2].strip()
        if line.startswith("*"):
            line = line.lstrip("*").strip()

        if not line:
            continue

        if line.startswith("@"):
            parts = line.split(None, 1)
            tag = parts[0]
            val = parts[1].strip() if len(parts) > 1 else ""
            current_tag = tag

            if tag in ("@impl", "@base"):
                kind = tag[1:]  # 'impl' or 'base'
                name = val
            elif tag == "@rank":
                try:
                    rank = int(val)
                except ValueError:
                    sys.exit(f"Error in {file_path}: @rank must be an integer, got '{val}'.")
        elif current_tag == "@deps" and line.startswith("-"):
            dep_name = line.lstrip("-").strip()
            if dep_name:
                deps.append(dep_name)

    if not kind or not name:
        sys.exit(f"Error in {file_path}: @fuse block must define either @impl <NAME> or @base <NAME>.")

    # Default rank: 0 for @base, 1 for @impl
    if rank is None:
        rank = 0 if kind == "base" else 1

    meta = {
        "kind": kind,
        "name": name,
        "rank": rank,
        "deps": deps,
        "body": cleaned_body,
        "file": file_path,
    }
    return meta, cleaned_body


def to_macro_name(name: str, prefix: str) -> str:
    """Format name into PREFIX_NAME_IMPLEMENTATION."""
    if prefix and not name.startswith(prefix + "_"):
        name = f"{prefix}_{name}"
    if not name.endswith("_IMPLEMENTATION"):
        name = f"{name}_IMPLEMENTATION"
    return name


def topological_sort(modules: dict, max_depth: int = 64) -> list:
    """
    Sort modules so dependencies appear BEFORE dependees.
    Independent modules are ordered by their @rank (lower rank first).
    Detects dependency cycles and depth limit.
    """
    order = []
    visited = set()
    visiting = set()

    def dfs(name: str, depth: int):
        if depth > max_depth:
            sys.exit(f"Error: Max dependency chain depth ({max_depth}) exceeded at '{name}'.")
        if name in visiting:
            sys.exit(f"Error: Cyclic dependency detected involving '{name}'.")
        if name in visited:
            return

        visiting.add(name)
        node = modules[name]
        for dep in sorted(node["deps"], key=lambda k: (modules[k]["rank"], k)):
            if dep not in modules:
                sys.exit(f"Error: Module '{name}' in {node['file']} depends on unknown module '{dep}'.")
            dfs(dep, depth + 1)

        visiting.remove(name)
        visited.add(name)
        order.append(name)

    # Sort root traversal by (rank, name)
    for name in sorted(modules.keys(), key=lambda k: (modules[k]["rank"], k)):
        if name not in visited:
            dfs(name, depth=0)

    return order


def resolve_base_guards(modules: dict) -> dict:
    """
    For each @base module, compute which @impl modules depend on it directly or transitively.
    Returns:
        dict: { base_name: [list of root @impl names] }
    """
    reverse_deps = {name: set() for name in modules}
    for name, node in modules.items():
        for dep in node["deps"]:
            reverse_deps[dep].add(name)

    base_roots = {}
    for name, node in modules.items():
        if node["kind"] != "base":
            continue

        roots = set()
        visited = set()
        queue = [name]

        while queue:
            curr = queue.pop(0)
            if curr in visited:
                continue
            visited.add(curr)

            for parent in reverse_deps.get(curr, []):
                parent_node = modules[parent]
                if parent_node["kind"] == "impl":
                    roots.add(parent)
                # Keep traversing upwards through intermediate base nodes
                queue.append(parent)

        base_roots[name] = sorted(roots)

    return base_roots


def main():
    parser = argparse.ArgumentParser(description="Fuse single-header C/C++ libraries from modular sources.")
    parser.add_argument("header", type=pathlib.Path, help="Path to public header (e.g. onecore.h)")
    parser.add_argument("sources", nargs="+", type=pathlib.Path, help="Source directories or files to fuse")
    parser.add_argument("-o", "--output", type=pathlib.Path, default=None, help="Output destination (defaults to stdout)")
    parser.add_argument("--prefix", type=str, default="ONECORE", help="Macro prefix for implementations (default: ONECORE)")
    args = parser.parse_args()

    if not args.header.is_file():
        sys.exit(f"Error: Header file not found: {args.header}")

    # Collect source files
    source_files = []
    for src in args.sources:
        if src.is_dir():
            for ext in ("*.c", "*.h"):
                source_files.extend(src.rglob(ext))
        elif src.is_file():
            source_files.append(src)
        else:
            sys.exit(f"Error: Source path not found: {src}")

    # Deduplicate and sort for deterministic builds
    source_files = sorted(set(f.resolve() for f in source_files))

    modules = {}
    for file_path in source_files:
        if file_path == args.header.resolve():
            continue

        try:
            content = file_path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue

        meta, _ = parse_fuse_block(content, file_path)
        if meta is not None:
            name = meta["name"]
            if name in modules:
                sys.exit(f"Error: Duplicate module name '{name}' found in:\n  - {modules[name]['file']}\n  - {file_path}")
            modules[name] = meta

    # Topological sort (dependencies come first, ranks break ties)
    sorted_names = topological_sort(modules)
    base_guards = resolve_base_guards(modules)

    # Build fused modules buffer
    fused_blocks = []
    for name in sorted_names:
        node = modules[name]
        body = node["body"]

        if node["kind"] == "impl":
            macro = to_macro_name(name, args.prefix)
            fused_blocks.append(f"#ifdef {macro}\n{body}\n#endif /* {macro} */")

        elif node["kind"] == "base":
            roots = base_guards.get(name, [])
            if not roots:
                # Nobody depends on this base module
                continue

            cond = " || ".join(f"defined({to_macro_name(r, args.prefix)})" for r in roots)
            fused_blocks.append(f"#if {cond}\n{body}\n#endif /* {name} */")

    fused_code = "\n\n".join(fused_blocks).strip()

    # Read base header
    header_content = args.header.read_text(encoding="utf-8")

    # If /* ---- */ is present in the header, replace it in place.
    # Otherwise, append to the end.
    marker = re.search(r'/\*\s*----\s*\*/', header_content)
    if marker:
        final_output = (
            header_content[:marker.start()].rstrip()
            + "\n\n"
            + fused_code
            + "\n\n"
            + header_content[marker.end():].lstrip()
        )
    else:
        final_output = (
            header_content.rstrip()
            + "\n\n"
            + fused_code
            + "\n"
        )

    final_output = final_output.rstrip() + "\n"

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(final_output, encoding="utf-8")
    else:
        sys.stdout.write(final_output)


if __name__ == "__main__":
    main()
