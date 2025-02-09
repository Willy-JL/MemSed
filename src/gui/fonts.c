#include "fonts.h"

#include <dcimgui/dcimgui.h>
#include <fonts/fonts.h>

static const ImFontConfig font_karla_config = {
    .OversampleH = 2,
    .OversampleV = 2,
    .GlyphOffset.y = -0.5f,
    .GlyphRanges = (ImWchar[]){0x1, 0x25ca, 0},
    .GlyphMinAdvanceX = 0.0f,
    .GlyphMaxAdvanceX = FLT_MAX,
    .RasterizerMultiply = 1.0f,
    .RasterizerDensity = 1.0f,
};

static const ImFontConfig font_meslo_config = {
    .OversampleH = 2,
    .OversampleV = 2,
    .GlyphRanges = (ImWchar[]){0x1, 0x2e2e, 0},
    .GlyphMinAdvanceX = 0.0f,
    .GlyphMaxAdvanceX = FLT_MAX,
    .RasterizerMultiply = 1.0f,
    .RasterizerDensity = 1.0f,
};

static const ImFontConfig font_mdi_config = {
    .MergeMode = true,
    .GlyphOffset.y = +1.0f,
    .GlyphRanges = (ImWchar[]){mdi_char_min, mdi_char_max, 0},
    .GlyphMinAdvanceX = 0.0f,
    .GlyphMaxAdvanceX = FLT_MAX,
    .RasterizerMultiply = 1.0f,
    .RasterizerDensity = 1.0f,
};

void gui_fonts_load(Gui* gui) {
    gui->fonts.base = ImFontAtlas_AddFontFromMemoryCompressedTTF(
        gui->io->Fonts,
        font_karla_regular_compressed_data,
        font_karla_regular_compressed_size,
        18.0f,
        &font_karla_config,
        NULL);
    ImFontAtlas_AddFontFromMemoryCompressedTTF(
        gui->io->Fonts,
        font_materialdesignicons_webfont_compressed_data,
        font_materialdesignicons_webfont_compressed_size,
        18.0f,
        &font_mdi_config,
        NULL);

    gui->fonts.bold = ImFontAtlas_AddFontFromMemoryCompressedTTF(
        gui->io->Fonts,
        font_karla_bold_compressed_data,
        font_karla_bold_compressed_size,
        22.0f,
        &font_karla_config,
        NULL);
    ImFontAtlas_AddFontFromMemoryCompressedTTF(
        gui->io->Fonts,
        font_materialdesignicons_webfont_compressed_data,
        font_materialdesignicons_webfont_compressed_size,
        18.0f,
        &font_mdi_config,
        NULL);

    gui->fonts.big = ImFontAtlas_AddFontFromMemoryCompressedTTF(
        gui->io->Fonts,
        font_karla_bold_compressed_data,
        font_karla_bold_compressed_size,
        32.0f,
        &font_karla_config,
        NULL);
    ImFontAtlas_AddFontFromMemoryCompressedTTF(
        gui->io->Fonts,
        font_materialdesignicons_webfont_compressed_data,
        font_materialdesignicons_webfont_compressed_size,
        28.0f,
        &font_mdi_config,
        NULL);

    gui->fonts.small = ImFontAtlas_AddFontFromMemoryCompressedTTF(
        gui->io->Fonts,
        font_karla_bold_compressed_data,
        font_karla_bold_compressed_size,
        14.0f,
        &font_karla_config,
        NULL);
    ImFontAtlas_AddFontFromMemoryCompressedTTF(
        gui->io->Fonts,
        font_materialdesignicons_webfont_compressed_data,
        font_materialdesignicons_webfont_compressed_size,
        14.0f,
        &font_mdi_config,
        NULL);

    gui->fonts.mono = ImFontAtlas_AddFontFromMemoryCompressedTTF(
        gui->io->Fonts,
        font_meslolgs_regular_compressed_data,
        font_meslolgs_regular_compressed_size,
        17.0f,
        &font_meslo_config,
        NULL);
}
