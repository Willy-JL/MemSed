#include "fonts.h"
#include "font_data.h"

#include <dcimgui/dcimgui.h>
#include <float.h>

const ImFontConfig font_karla_config = {
    .OversampleH = 2,
    .OversampleV = 2,
    .GlyphOffset.y = -0.5f,
    .GlyphRanges = (ImWchar[]){0x1, 0x25ca, 0},
    .GlyphMinAdvanceX = 0.0f,
    .GlyphMaxAdvanceX = FLT_MAX,
    .RasterizerMultiply = 1.0f,
    .RasterizerDensity = 1.0f,
};

const ImFontConfig font_meslo_config = {
    .OversampleH = 2,
    .OversampleV = 2,
    .GlyphRanges = (ImWchar[]){0x1, 0x2e2e, 0},
    .GlyphMinAdvanceX = 0.0f,
    .GlyphMaxAdvanceX = FLT_MAX,
    .RasterizerMultiply = 1.0f,
    .RasterizerDensity = 1.0f,
};

const ImFontConfig font_mdi_config = {
    .MergeMode = true,
    .GlyphOffset.y = +1.0f,
    .GlyphRanges = (ImWchar[]){0xf0001, 0xf1d17, 0},
    .GlyphMinAdvanceX = 0.0f,
    .GlyphMaxAdvanceX = FLT_MAX,
    .RasterizerMultiply = 1.0f,
    .RasterizerDensity = 1.0f,
};

void gui_fonts_load(Gui* gui) {
    gui->fonts.base = ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(
        gui->io->Fonts,
        font_karla_regular_compressed_data_base85,
        18,
        &font_karla_config,
        NULL);
    ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(
        gui->io->Fonts,
        font_mdi_compressed_data_base85,
        18,
        &font_mdi_config,
        NULL);

    gui->fonts.bold = ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(
        gui->io->Fonts,
        font_karla_bold_compressed_data_base85,
        22,
        &font_karla_config,
        NULL);
    ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(
        gui->io->Fonts,
        font_mdi_compressed_data_base85,
        18,
        &font_mdi_config,
        NULL);

    gui->fonts.big = ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(
        gui->io->Fonts,
        font_karla_bold_compressed_data_base85,
        32,
        &font_karla_config,
        NULL);
    ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(
        gui->io->Fonts,
        font_mdi_compressed_data_base85,
        28,
        &font_mdi_config,
        NULL);

    gui->fonts.small = ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(
        gui->io->Fonts,
        font_karla_bold_compressed_data_base85,
        14,
        &font_karla_config,
        NULL);
    ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(
        gui->io->Fonts,
        font_mdi_compressed_data_base85,
        14,
        &font_mdi_config,
        NULL);

    gui->fonts.mono = ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(
        gui->io->Fonts,
        font_meslo_compressed_data_base85,
        17,
        &font_meslo_config,
        NULL);
}
