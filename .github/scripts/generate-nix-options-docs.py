#!/usr/bin/env python3
"""
Converts NixOS options JSON into clean Markdown documentation.
"""

import json
import sys
import re

SECTIONS = [
    {
        "title": "NixOS",
        "subtitle": "**System-level options via `programs.mango`.**",
    },
    {
        "title": "Home Manager",
        "subtitle": "**Configure mangowm declaratively via `wayland.windowManager.mango`.**",
    },
]

HEADER = (
    "---\n"
    "title: Nix Module Options\n"
    "description: NixOS and Home Manager configuration options for mangowm.\n"
    "---\n\n"
    "> **Note:** This document is automatically generated from the Nix module source code.\n\n"
)

def clean_description(desc: str) -> str:
    """Strips Nix inline markup tags, fixes dangling periods, and formats blockquotes."""
    if not desc:
        return "*No description provided.*"

    # Strip Nix inline markup: {tag}`content` → `content`; bare tags → ""
    desc = re.sub(r'\{(?:var|option|manpage|file|env|command|program)\}(`[^`]+`)', r'\1', desc)
    desc = re.sub(r'\{(?:var|option|manpage|file|env|command|program)\}', '', desc)
    # Remove period left on its own line after tag removal
    desc = re.sub(r'\n\s*\.(\s|$)', r'.\1', desc)

    lines = desc.splitlines()
    cleaned = []
    in_block = False

    for line in lines:
        m = re.match(r':::\s*\{\.(\w+)\}', line)
        if m:
            block_type = m.group(1).capitalize()
            in_block = True
            cleaned.append(f"> **{block_type}:**\n>")
        elif line.startswith(":::"):
            in_block = False
        else:
            cleaned.append(f"> {line}" if in_block else line)

    return "\n".join(cleaned)

def format_value(val_data) -> str:
    """Formats a value as inline code or a nix code block."""
    if val_data is None:
        return None

    if isinstance(val_data, dict) and val_data.get("_type") == "literalMD":
        return val_data.get("text", "").strip()
    elif isinstance(val_data, dict) and val_data.get("_type") == "literalExpression":
        text = val_data.get("text", "").strip()
    elif isinstance(val_data, bool):
        text = "true" if val_data else "false"
    elif val_data == {} or val_data == []:
        text = "{ }" if val_data == {} else "[ ]"
    else:
        text = str(val_data).strip()

    if '\n' in text:
        return f"\n```nix\n{text}\n```"
    return f"`{text}`"

def write_section(out, data, title, subtitle):
    out.write(f"## {title}\n\n{subtitle}\n\n")
    for key, opt in sorted(data.items()):
        if key.startswith("_module"):
            continue

        desc = clean_description(opt.get("description", ""))
        opt_type = str(opt.get("type", "unknown"))
        default_val = format_value(opt.get("default"))
        example_val = format_value(opt.get("example"))

        block = f"### `{key}`\n\n{desc}\n\n**Type:** `{opt_type}`\n\n"
        if default_val is not None:
            block += f"**Default:** {default_val}\n\n"
        if example_val is not None:
            block += f"**Example:** {example_val}\n\n"
        block += "---\n\n"
        out.write(block)

def main():
    if len(sys.argv) != 4:
        sys.exit("Usage: generate-nix-options-docs.py <nixos.json> <hm.json> <output.md>")

    nixos_json, hm_json, output_md = sys.argv[1:4]
    inputs = [nixos_json, hm_json]

    with open(output_md, 'w', encoding='utf-8') as out:
        out.write(HEADER)
        for path, section in zip(inputs, SECTIONS):
            with open(path, 'r', encoding='utf-8') as f:
                data = json.load(f)
            write_section(out, data, section["title"], section["subtitle"])
            print(f"Written {section['title']} section.")

if __name__ == "__main__":
    main()
