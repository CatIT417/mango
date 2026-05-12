#!/usr/bin/env python3
"""
Converts NixOS options JSON into clean, table-formatted Markdown.
"""

import json
import sys
import re

def clean_description(desc: str) -> str:
    """Removes Nix tags, fixes dangling periods, and formats blockquotes."""
    if not desc:
        return "*No description provided.*"

    desc = re.sub(r'\{[a-zA-Z]+\}', '', desc).replace('\n.', '.')

    lines = desc.splitlines()
    cleaned = []
    in_note = False
    
    for line in lines:
        if line.startswith("::: {.note"):
            in_note = True
            cleaned.append("> **Note:**\n>")
        elif line.startswith(":::"):
            in_note = False
        else:
            cleaned.append(f"> {line}" if in_note else line)

    return "\n".join(cleaned)

def format_default_value(default_data) -> str:
    """Safely formats the default value, handling HTML escaping for tables."""
    if default_data is None:
        return "*None*"

    val_text = default_data.get("text", "") if isinstance(default_data, dict) and default_data.get("_type") == "literalExpression" else str(default_data)
    val_text = val_text.replace('|', '&#124;')

    if '\n' in val_text:
        safe_html = val_text.replace('<', '&lt;').replace('>', '&gt;').replace('\n', '<br>')
        return f"<code>{safe_html}</code>"
    
    return f"`{val_text}`"

def main():
    if len(sys.argv) != 4:
        sys.exit("Usage: format_docs.py <input.json> <output.md> <title>")

    input_json, output_md, title = sys.argv[1:4]

    with open(input_json, 'r', encoding='utf-8') as f:
        data = json.load(f)

    with open(output_md, 'a', encoding='utf-8') as out:
        out.write(f"## {title}\n\n")

        for key, opt in sorted(data.items()):
            if key.startswith("_module"):
                continue

            desc = clean_description(opt.get("description", ""))
            opt_type = str(opt.get("type", "unknown")).replace('|', '&#124;')
            default_val = format_default_value(opt.get("default"))
            
            markdown_block = (
                f"### `{key}`\n\n"
                f"{desc}\n\n"
                f"| Attribute | Value |\n"
                f"| :--- | :--- |\n"
                f"| **Type** | `{opt_type}` |\n"
                f"| **Default** | {default_val} |\n\n"
                f"---\n\n"
            )
            out.write(markdown_block)

    print(f"Appended {title} to {output_md} successfully.")

if __name__ == "__main__":
    main()
