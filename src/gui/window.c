#include "window.h"

const char* ok = mdi_check " Ok";
const char* cancel = mdi_cancel " Cancel";
const char* select_process = mdi_select_search " Select Process";
const char* detach_process = mdi_exit_run " Detach Process";
const char* first_search = mdi_magnify_plus " First Search";
const char* next_search = mdi_magnify_expand " Next Search";
const char* undo_search = mdi_magnify_minus " Undo Search";
const char* cancel_search = mdi_magnify_close " Cancel Search";
const char* reset_search = mdi_magnify_close " Reset Search";

const uint8_t pane_spacing_mult = 3;
const ImVec2 options_min_size = {378.0f, 250.0f};

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
    bool is_searching = memory_search_is_searching(gui->memory_search);
    ImGui_BeginDisabled(is_searching);
    if(is_attached) {
        if(ImGui_Button(detach_process)) {
            memory_search_process_detach(gui->memory_search);
        }
    } else {
        if(ImGui_Button(select_process)) {
            ImGui_OpenPopup(select_process, ImGuiPopupFlags_None);
        }
    }
    ImGui_EndDisabled();
    gui_window_draw_select_process_popup(gui);

    ImGui_SameLineEx(0.0f, pane_spacing_mult * gui->style->ItemSpacing.x);

    ImGui_BeginDisabled(!is_attached);
    const ImVec2 progressbar_size = {ImGui_GetContentRegionAvail().x, 0.0f};
    if(is_searching) {
        flt32_t progress = memory_search_get_search_progress(gui->memory_search);
        char progress_str[5];
        snprintf(progress_str, sizeof(progress_str), "%.0f%%", progress);
        ImGui_ProgressBar(progress, progressbar_size, progress_str);
    } else {
        // FIXME: use process commandline
        const char* label = is_attached ? "/sbin/init" : "No Process Selected";
        ImGui_ProgressBar(0.0f, progressbar_size, label);
    }
    ImGui_EndDisabled();
}

static void gui_window_draw_addresses_pane(Gui* gui, ImVec2 size) {
    bool is_searching = memory_search_is_searching(gui->memory_search);
    ImGui_BeginDisabled(is_searching);
    if(ImGui_BeginTableEx(
           "###addresses",
           4,
           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
           size,
           0.0f)) {
        ImGui_PushFont(gui->fonts.mono);
        ImGui_TableSetupColumnEx(
            "Address",
            ImGuiTableColumnFlags_WidthFixed,
            ImGui_CalcTextSize("0x112233445566").x,
            0);
        ImGui_TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
        ImGui_TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Previous", ImGuiTableColumnFlags_WidthStretch);
        ImGui_PushFont(gui->fonts.base);
        ImGui_TableHeadersRow();
        ImGui_PopFont();

        MemorySearchResults results = memory_search_get_results(gui->memory_search);
        if(!is_searching && results.batches_count >= 1 && results.batches != NULL) {
            MemorySearchResultBatch* batch = &results.batches[results.batches_count - 1];
            MemorySearchResultBatch* prev_batch =
                results.batches_count >= 2 ? &results.batches[results.batches_count - 2] : NULL;
            for(size_t set_i = 0; set_i < batch->sets_count; set_i++) {
                MemorySearchResultSet* set = &batch->sets[set_i];
                MemorySearchResultSet* prev_set = NULL;
                for(size_t prev_set_i = 0; prev_set_i < prev_batch->sets_count; prev_set_i++) {
                    prev_set = &prev_batch->sets[prev_set_i];
                    if(prev_set->type == set->type) {
                        break;
                    }
                }
                const char* type_str = memory_type_get_short_name(set->type);
                for(size_t result_i = 0; result_i < set->results_count; result_i++) {
                    MemorySearchResultDisplay display =
                        memory_search_get_result_display(set, result_i);
                    ImGui_TableNextRow();
                    // Address
                    ImGui_TableNextColumn();
                    ImGui_TextUnformatted(display.address_str);
                    // Type
                    ImGui_TableNextColumn();
                    ImGui_TextUnformatted(type_str);
                    // Value
                    ImGui_TableNextColumn();
                    ImGui_TextUnformatted(display.value_str);
                    if(prev_set != NULL) {
                        // Previous
                        MemoryAddress address = memory_search_get_result_address(set, result_i);
                        for(size_t prev_i = 0; prev_i < prev_set->results_count; prev_i++) {
                            if(memory_search_get_result_address(prev_set, prev_i) == address) {
                                display = memory_search_get_result_display(prev_set, prev_i);
                                ImGui_TableNextColumn();
                                ImGui_TextUnformatted(display.value_str);
                            }
                        }
                    }
                }
            }
        }
        ImGui_PopFont();
        ImGui_EndTable();
    }
    ImGui_EndDisabled();
}

static void gui_window_draw_options_pane(Gui* gui, ImVec2 size) {
    if(ImGui_BeginChild("###options", size, ImGuiChildFlags_Borders, ImGuiWindowFlags_None)) {
        ImGui_BeginDisabled(!memory_search_process_is_attached(gui->memory_search));
        MemorySearchParams params = memory_search_get_params(gui->memory_search);
        bool is_searching = memory_search_is_searching(gui->memory_search);

        // FIXME: make the search buttons work
        static uint32_t search_count = 0;
        ImGui_BeginDisabled(is_searching);
        if(search_count == 0) {
            if(ImGui_Button(first_search)) {
                search_count = 1;
            }
        } else {
            if(ImGui_Button(next_search)) {
                search_count++;
            }
        }
        ImGui_EndDisabled();

        ImGui_SameLine();

        if(is_searching) {
            if(ImGui_Button(cancel_search)) {
            }
        } else {
            ImGui_BeginDisabled(search_count < 1);
            if(ImGui_Button(undo_search)) {
                search_count--;
            }
            ImGui_EndDisabled();
        }

        ImGui_SameLine();

        ImGui_BeginDisabled(is_searching || search_count < 1);
        if(ImGui_Button(reset_search)) {
            search_count = 0;
        }
        ImGui_EndDisabled();

        if(memory_search_is_searching(gui->memory_search)) {
            ImGui_Text("Searching...");
        } else {
            MemorySearchResults results = memory_search_get_results(gui->memory_search);
            ImGui_Text(
                "Search Depth: %zu\tCurrent Results: %zu",
                results.batches_count,
                results.current_results_count);
        }

        if(ImGui_BeginTable("###columns", 2, ImGuiTableFlags_None)) {
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
            ImGui_SetNextItemWidth(-FLT_MIN);
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
            ImGui_TableNextRow();
            ImGui_TableNextColumn();
            ImGui_AlignTextToFramePadding();
            ImGui_Text("Precision:");
            ImGui_TableNextColumn();
            flt32_t precision_alignment_width =
                (ImGui_GetContentRegionAvail().x - ImGui_CalcTextSize("Alignment:").x -
                 (pane_spacing_mult + 1) * gui->style->ItemSpacing.x) /
                2.0f;
            snprintf(temp_str, sizeof(temp_str), "%.*Lg", LDBL_DIG, params.precision);
            ImGui_SetNextItemWidth(precision_alignment_width);
            if(ImGui_InputText(
                   "###precision",
                   temp_str,
                   sizeof(temp_str),
                   ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsDecimal)) {
                params.precision = ABS(strtold(temp_str, NULL));
                memory_search_set_params(gui->memory_search, params);
            }
            ImGui_EndDisabled();

            ImGui_BeginDisabled(search_count != 0);
            // Alignment
            ImGui_SameLineEx(0.0f, pane_spacing_mult * gui->style->ItemSpacing.x);
            // ImGui_SameLine();
            ImGui_Text("Alignment:");
            ImGui_SameLine();
            snprintf(temp_str, sizeof(temp_str), "%u", params.alignment);
            ImGui_SetNextItemWidth(precision_alignment_width);
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

            // Type
            ImGui_TableNextRow();
            ImGui_TableNextColumn();
            ImGui_AlignTextToFramePadding();
            ImGui_Text("Type:");
            ImGui_TableNextColumn();
            temp_int = params.type;
            ImGui_SetNextItemWidth(-FLT_MIN);
            if(ImGui_ComboChar(
                   "###type",
                   &temp_int,
                   memory_type_names,
                   COUNT_OF(memory_type_names))) {
                params.type = temp_int;
                memory_search_set_params(gui->memory_search, params);
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
        MAX(options_min_size.y, avail.y / 2.0f),
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
