#include "window.h"

const char* ok = mdi_check " Ok";
const char* cancel = mdi_cancel " Cancel";
const char* select_process = mdi_select_search " Select Process";
const char* detach_process = mdi_exit_run " Detach Process";
const char* first_search = mdi_magnify_plus " First Search";
const char* next_search = mdi_magnify_expand " Next Search";
const char* undo_search = mdi_magnify_minus " Undo Search";
const char* cancel_search = mdi_magnify_close " Cancel Search";

const uint8_t pane_spacing_mult = 3;
const ImVec2 options_min_size = {500.0f, 600.0f};

static void gui_window_draw_select_process_popup(Gui* gui) {
    if(ImGui_BeginPopupModal(
           select_process,
           NULL,
           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        // FIXME: process picker
        ImGui_Text("Such Empty");

        ImGui_BeginDisabled(false);
        if(ImGui_Button(ok)) {
            memory_search_process_attach(gui->memory_search, 1);
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

static void gui_window_draw_toolbar(Gui* gui) {
    bool is_attached = memory_search_process_is_attached(gui->memory_search);
    if(is_attached) {
        if(ImGui_Button(detach_process)) {
            memory_search_process_detach(gui->memory_search);
        }
    } else {
        if(ImGui_Button(select_process)) {
            ImGui_OpenPopup(select_process, ImGuiPopupFlags_None);
        }
    }
    gui_window_draw_select_process_popup(gui);

    ImGui_SameLineEx(0.0f, pane_spacing_mult * gui->style->ItemSpacing.x);

    ImGui_BeginDisabled(!is_attached);
    // FIXME: process commandline when idle
    // FIXME: progress when searching
    const char* label = is_attached ? "/sbin/init" : "No Process Selected";
    ImGui_ProgressBar(0.0f, (ImVec2){ImGui_GetContentRegionAvail().x, 0.0f}, label);
    ImGui_EndDisabled();
}

static void gui_window_draw_addresses_pane(Gui* gui, ImVec2 size) {
    UNUSED(gui);

    if(ImGui_BeginTableEx(
           "###addresses",
           4,
           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
           size,
           0.0f)) {
        ImGui_TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed);
        ImGui_TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Previous", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableHeadersRow();
        // FIXME: use results from memory search
        const ImVec2* example_addresses[] = {&size, &gui->prev_size, &gui->io->DisplaySize};
        for(size_t i = 0; i < COUNT_OF(example_addresses); i++) {
            ImGui_TableNextRow();
            // Address
            ImGui_TableNextColumn();
            ImGui_Text("0x%" PRIXPTR, (uintptr_t)example_addresses[i]);
            // Type
            ImGui_TableNextColumn();
            ImGui_Text("Example");
            // FIXME: handle different value types
            // Value
            ImGui_TableNextColumn();
            ImGui_Text("%f", example_addresses[i]->x);
            // Previous
            ImGui_TableNextColumn();
            ImGui_Text("%f", example_addresses[i]->y);
        }
        ImGui_EndTable();
    }
}

static void gui_window_draw_options_pane(Gui* gui, ImVec2 size) {
    if(ImGui_BeginChild("###options", size, ImGuiChildFlags_Borders, ImGuiWindowFlags_None)) {
        ImGui_BeginDisabled(!memory_search_process_is_attached(gui->memory_search));
        MemorySearchParams params = memory_search_get_params(gui->memory_search);

        // FIXME: make the search buttons work
        static uint32_t search_count = 0;
        if(search_count == 0) {
            if(ImGui_Button(first_search)) {
                search_count = 1;
            }
        } else {
            if(ImGui_Button(next_search)) {
                search_count++;
            }
        }

        ImGui_SameLine();

        ImGui_BeginDisabled(search_count < 1);
        if(ImGui_Button(undo_search)) {
            search_count--;
        }
        ImGui_EndDisabled();

        ImGui_SameLine();

        ImGui_BeginDisabled(search_count < 1);
        if(ImGui_Button(cancel_search)) {
            search_count = 0;
        }
        ImGui_EndDisabled();

        // FIXME: use results from memory search
        ImGui_Text("Search Depth: %u\tCurrent Results: %u", search_count, 3);

        if(ImGui_BeginTable("###columns", 4, ImGuiTableFlags_None)) {
            ImGui_TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
            ImGui_TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
            ImGui_TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
            ImGui_TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
            int32_t temp_int;
            char temp_str[33];

            // Value
            ImGui_TableNextRow();
            ImGui_TableNextColumn();
            ImGui_AlignTextToFramePadding();
            ImGui_Text("Value:");
            ImGui_TableNextColumn();
            snprintf(temp_str, sizeof(temp_str), "%.*Lg", LDBL_DIG, params.value);
            ImGui_SetNextItemWidth(269.0f);
            if(ImGui_InputText(
                   "###value",
                   temp_str,
                   sizeof(temp_str),
                   ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsDecimal)) {
                params.value = strtold(temp_str, NULL);
                memory_search_set_params(gui->memory_search, params);
            }

            ImGui_BeginDisabled(params.type <= MemoryTypeInteger);
            // Precision
            ImGui_TableNextColumn();
            ImGui_Text("Precision:");
            ImGui_TableNextColumn();
            snprintf(temp_str, sizeof(temp_str), "%.*Lg", LDBL_DIG, params.precision);
            ImGui_SetNextItemWidth(-FLT_MIN);
            if(ImGui_InputText(
                   "###precision",
                   temp_str,
                   sizeof(temp_str),
                   ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsDecimal)) {
                params.precision = ABS(strtold(temp_str, NULL));
                memory_search_set_params(gui->memory_search, params);
            }
            ImGui_EndDisabled();

            ImGui_BeginDisabled(search_count);
            // Type
            ImGui_TableNextRow();
            ImGui_TableNextColumn();
            ImGui_AlignTextToFramePadding();
            ImGui_Text("Type:");
            ImGui_TableNextColumn();
            temp_int = params.type;
            ImGui_SetNextItemWidth(269.0f);
            if(ImGui_ComboChar(
                   "###type",
                   &temp_int,
                   memory_type_names,
                   COUNT_OF(memory_type_names))) {
                params.type = temp_int;
                memory_search_set_params(gui->memory_search, params);
            }

            // Alignment
            ImGui_TableNextColumn();
            ImGui_Text("Alignment:");
            ImGui_TableNextColumn();
            snprintf(temp_str, sizeof(temp_str), "%u", params.alignment);
            ImGui_SetNextItemWidth(-FLT_MIN);
            if(ImGui_BeginCombo("###alignment", temp_str, ImGuiComboFlags_None)) {
                for(uint8_t i = 1; i <= 16; i *= 2) {
                    bool is_selected = i == params.alignment;
                    snprintf(temp_str, sizeof(temp_str), "%u", i);
                    if(ImGui_SelectableBoolPtr(temp_str, &is_selected, ImGuiSelectableFlags_None)) {
                        params.alignment = i;
                        memory_search_set_params(gui->memory_search, params);
                    }
                    if(is_selected) {
                        ImGui_SetItemDefaultFocus();
                    }
                }
                ImGui_EndCombo();
            }
            ImGui_EndDisabled();

            ImGui_EndTable();
        }

        ImGui_EndDisabled();
    }
    ImGui_EndChild();
}

static void gui_window_draw_scratchpad_pane(Gui* gui, ImVec2 size) {
    if(ImGui_BeginTableEx(
           "###scratchpad",
           5,
           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
           size,
           0.0f)) {
        ImGui_TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed);
        ImGui_TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed);
        ImGui_TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableHeadersRow();
        // FIXME: use data from memory search
        const ImVec2* example_addresses[] = {&size, &gui->prev_size, &gui->io->DisplaySize};
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
            // FIXME: handle different value types
            // Value
            ImGui_TableNextColumn();
            ImGui_Text("%f", example_addresses[i]->x);
            ImGui_PopID();
        }
        ImGui_EndTable();
    }
}

void gui_window_draw(Gui* gui) {
    ImGui_SetNextWindowPos((ImVec2){0.0f, 0.0f}, ImGuiCond_Once);
    int32_t width, height;
    SDL_GetWindowSize(gui->window, &width, &height);
    const ImVec2 window_size = {width, height};
    if(window_size.x != gui->prev_size.x || window_size.y != gui->prev_size.y) {
        ImGui_SetNextWindowSize(window_size, ImGuiCond_Always);
    }

    ImGui_PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui_Begin(
        "MemSed",
        NULL,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);
    ImGui_PopStyleVar();

    // Toolbar
    gui_window_draw_toolbar(gui);

    for(uint8_t i = 1; i < pane_spacing_mult; i++) {
        ImGui_Spacing();
    }

    // Addresses
    ImVec2 avail = ImGui_GetContentRegionAvail();
    ImVec2 addresses_size = {
        avail.x - options_min_size.x - (pane_spacing_mult * gui->style->ItemSpacing.x),
        MAX(options_min_size.y, avail.y * 2.0f / 3.0f),
    };
    gui_window_draw_addresses_pane(gui, addresses_size);

    ImGui_SameLineEx(0.0f, pane_spacing_mult * gui->style->ItemSpacing.x);

    // Options
    ImVec2 options_size = {options_min_size.x, addresses_size.y};
    gui_window_draw_options_pane(gui, options_size);

    for(uint8_t i = 1; i < pane_spacing_mult; i++) {
        ImGui_Spacing();
    }

    // Scratchpad
    gui_window_draw_scratchpad_pane(gui, ImGui_GetContentRegionAvail());

    ImGui_End();
}
