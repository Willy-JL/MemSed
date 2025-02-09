import builtins
import pathlib
import struct
import sys


def mdi_generate_header(font_path: pathlib.Path, header_path: pathlib.Path):
    font = font_path.read_bytes()

    def unpack(fmt: str, size: int, offset: int):
        return struct.unpack(fmt, font[offset : offset + size])[0]

    ulong_size = 4
    ushort_size = 2
    ubyte_size = 1
    ulong = lambda offset: unpack(">L", ulong_size, offset)
    ushort = lambda offset: unpack(">H", ushort_size, offset)
    ubyte = lambda offset: unpack(">B", ubyte_size, offset)

    cmap_offset = ulong(font.find(b"cmap") + len(b"cmap") + ulong_size)
    cmap_tables = ushort(cmap_offset + ushort_size)
    for table_i in range(cmap_tables):
        cmap_table_offset = cmap_offset + ulong(
            cmap_offset
            + 2 * ushort_size
            + table_i * (2 * ushort_size + ulong_size)
            + 2 * ushort_size
        )
        cmap_table_format = ushort(cmap_table_offset)
        if cmap_table_format == 12:
            break

    encr_offset = cmap_table_offset
    groups = ulong(encr_offset + 2 * ushort_size + 2 * ulong_size)
    groups_offset = encr_offset + 2 * ushort_size + 3 * ulong_size
    glyphs_chars = {}
    min_char = max_char = None
    for group_i in range(groups):
        group_offset = groups_offset + group_i * (3 * ulong_size)
        char_num = ulong(group_offset)
        glyph_id = ulong(group_offset + 2 * ulong_size)
        glyphs_chars[glyph_id] = chr(char_num)
        if min_char is None and max_char is None:
            min_char = max_char = char_num
        min_char = min(min_char, char_num)
        max_char = max(max_char, char_num)

    post_offset = ulong(font.find(b"post") + len(b"post") + ulong_size)
    glyphs = ushort(post_offset + 2 * 4 + 2 * 2 + 5 * ulong_size)
    glyph_offset = (
        post_offset
        + 2 * 4
        + 2 * 2
        + 5 * ulong_size
        + ushort_size
        + glyphs * ushort_size
        + ubyte_size
    )
    names_chars = {}
    for glyph_i in range(glyphs - 1):
        glyph_id = glyph_i + 1
        name_size = ubyte(glyph_offset)
        glyph_offset += ubyte_size
        name = str(font[glyph_offset : glyph_offset + name_size], encoding="utf-8")
        glyph_offset += name_size
        names_chars[name] = glyphs_chars[glyph_id]

    header = (
        "#pragma once\n\n"
        f"#define mdi_char_min {hex(min_char)}\n"
        f"#define mdi_char_max {hex(max_char)}\n\n"
    )
    for name in sorted(names_chars):
        char = names_chars[name]
        name = name.replace("-", "_")
        header += f'#define mdi_{name} "{char}"\n'
    header_path.write_text(header)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Generate mapping header for MaterialDesignIcons font files")
        print("Usage: mdi.py <font path> <header path>")
        sys.exit(1)

    font_path = pathlib.Path(sys.argv[1])
    header_path = pathlib.Path(sys.argv[2])
    mdi_generate_header(font_path, header_path)
