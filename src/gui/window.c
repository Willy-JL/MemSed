#include "window.h"

const char* ok = mdi_check " Ok";
const char* cancel = mdi_cancel " Cancel";
const char* select_process = mdi_select_search " Select Process";
const char* first_scan = mdi_magnify_plus " First Scan";
const char* cancel_scan = mdi_magnify_close " Cancel Scan";

static void gui_window_select_process(Gui* gui) {
    UNUSED(gui);

    if(ImGui_BeginPopupModal(
           select_process,
           NULL,
           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui_Text("Such Empty");

        ImGui_BeginDisabled(true);
        if(ImGui_Button(ok)) {
            ImGui_CloseCurrentPopup();
        }
        ImGui_EndDisabled();

        ImGui_SameLine();
        if(ImGui_Button(cancel)) {
            ImGui_CloseCurrentPopup();
        }

        ImGui_EndPopup();
    }
}

void gui_window_draw(Gui* gui) {
    ImGui_SetNextWindowPos((ImVec2){0.0f, 0.0f}, ImGuiCond_Once);
    int32_t width, height;
    SDL_GetWindowSize(gui->window, &width, &height);
    ImVec2 size = {width, height};
    if(size.x != gui->prev_size.x || size.y != gui->prev_size.y) {
        ImGui_SetNextWindowSize(size, ImGuiCond_Always);
    }

    ImGui_PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui_Begin(
        "MemSed",
        NULL,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);
    ImGui_PopStyleVar();
    const uint8_t pane_spacing_mult = 3;

    // Toolbar

    if(ImGui_Button(select_process)) {
        ImGui_OpenPopup(select_process, ImGuiPopupFlags_None);
    }
    gui_window_select_process(gui);

    ImGui_SameLine();
    ImGui_BeginDisabled(true);
    ImGui_ProgressBar(0.0f, (ImVec2){ImGui_GetContentRegionAvail().x, 0.0f}, "0%");
    ImGui_EndDisabled();

    for(uint8_t i = 1; i < pane_spacing_mult; i++) {
        ImGui_Spacing();
    }

    // Main panes

    ImVec2 avail = ImGui_GetContentRegionAvail();
    const ImVec2 options_size = {500.0f, 600.0f};
    ImVec2 addresses_size = {
        avail.x - options_size.x - (pane_spacing_mult * gui->style->ItemSpacing.x),
        MAX(options_size.y, avail.y * 2.0f / 3.0f),
    };

    // Addresses
    if(ImGui_BeginTableEx(
           "###addresses",
           4,
           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
           addresses_size,
           0.0f)) {
        ImGui_TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed);
        ImGui_TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Previous", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableHeadersRow();
        const ImVec2* example_addresses[] = {&addresses_size, &options_size};
        for(size_t i = 0; i < COUNT_OF(example_addresses); i++) {
            ImGui_TableNextRow();
            // Address
            ImGui_TableNextColumn();
            ImGui_Text("0x%" PRIXPTR, (uintptr_t)example_addresses[i]);
            // Type
            ImGui_TableNextColumn();
            ImGui_Text("Example");
            // Value
            ImGui_TableNextColumn();
            ImGui_Text("%f", example_addresses[i]->x);
            // Previous
            ImGui_TableNextColumn();
            ImGui_Text("%f", example_addresses[i]->y);
        }
        ImGui_EndTable();
    }

    ImGui_SameLineEx(0.0f, pane_spacing_mult * gui->style->ItemSpacing.x);

    // Options
    if(ImGui_BeginChild(
           "###options",
           (ImVec2){options_size.x, addresses_size.y},
           ImGuiChildFlags_Borders,
           ImGuiWindowFlags_None)) {
        if(ImGui_Button(first_scan)) {
        }

        ImGui_SameLine();
        ImGui_BeginDisabled(true);
        if(ImGui_Button(cancel_scan)) {
        }
        ImGui_EndDisabled();
    }
    ImGui_EndChild();

    for(uint8_t i = 1; i < pane_spacing_mult; i++) {
        ImGui_Spacing();
    }

    // Scratchpad
    if(ImGui_BeginTableEx(
           "###scratchpad",
           5,
           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
           ImGui_GetContentRegionAvail(),
           0.0f)) {
        ImGui_TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed);
        ImGui_TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed);
        ImGui_TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableHeadersRow();
        const ImVec2* example_addresses[] = {&addresses_size, &options_size};
        for(size_t i = 0; i < COUNT_OF(example_addresses); i++) {
            ImGui_PushIDInt(i);
            ImGui_TableNextRow();
            // Active
            ImGui_TableNextColumn();
            bool x = false;
            ImGui_Checkbox("###active", &x);
            // Address
            ImGui_TableNextColumn();
            ImGui_Text("0x%" PRIXPTR, (uintptr_t)example_addresses[i]);
            // Type
            ImGui_TableNextColumn();
            ImGui_Text("Example");
            // Description
            ImGui_TableNextColumn();
            char y[20] = "";
            ImGui_SetNextItemWidth(-FLT_MIN);
            ImGui_InputText("###description", y, sizeof(y), ImGuiInputTextFlags_None);
            // Value
            ImGui_TableNextColumn();
            ImGui_Text("%f", example_addresses[i]->x);
            ImGui_PopID();
        }
        ImGui_EndTable();
    }

    ImGui_End();
}
