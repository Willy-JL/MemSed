#include "window.h"
#include "../process/list.h"

const char* ok = mdi_check " Ok";
const char* cancel = mdi_cancel " Cancel";
const char* attach_process = mdi_application_import " Attach Process";
const char* detach_process = mdi_exit_run " Detach Process";
const char* add_to_scratchpad = mdi_plus_box_multiple " Add to Scratchpad";
const char* remove_from_scratchpad = mdi_minus_box_multiple " Remove from Scratchpad";
const char* first_search = mdi_magnify_plus " First Search";
const char* next_search = mdi_magnify_expand " Next Search";
const char* undo_search = mdi_magnify_minus " Undo Search";
const char* stop_search = mdi_magnify_remove_cursor " Stop Search";
const char* reset_search = mdi_magnify_close " Reset Search";

const uint8_t pane_spacing_mult = 3;
const ImVec2 options_min_size = {378.0f, 250.0f};

const size_t imgui_clipper_max = 1'000'000;

typedef struct {
    char** buf;
    int32_t* len;
} ImguiInputTextResizeCallbackCtx;

static int32_t imgui_input_text_resize_callback(ImGuiInputTextCallbackData* data) {
    ImguiInputTextResizeCallbackCtx* ctx = data->UserData;
    if(data->EventFlag & ImGuiInputTextFlags_CallbackResize && data->BufTextLen + 1 != *ctx->len) {
        *ctx->len = data->BufTextLen + 1;
        *ctx->buf = realloc(*ctx->buf, *ctx->len);
        data->Buf = *ctx->buf;
    }
    return 0;
}

static void gui_window_draw_attach_process_popup(Gui* gui) {
    ImVec2 display = gui->io->DisplaySize;
    ImVec2 size = display;
    size.x *= 0.8f;
    size.y *= 0.8f;
    ImGui_SetNextWindowSize(size, ImGuiCond_Always);
    ImGui_SetNextWindowPosEx(
        (ImVec2){display.x / 2.0f, display.y / 2.0f},
        ImGuiCond_Always,
        (ImVec2){0.5f, 0.5f});
    if(ImGui_BeginPopupModal(
           attach_process,
           NULL,
           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
               ImGuiWindowFlags_AlwaysAutoResize)) {
        static ProcessList* list = NULL;
        if(list == NULL) {
            list = process_list_init();
        }
        static ProcessPid selected = 0;
        static char search[129] = "";
        bool any_selected = false;

        ImGui_SetNextItemWidth(-FLT_MIN);
        if(ImGui_IsWindowAppearing()) {
            ImGui_SetKeyboardFocusHere();
        }
        ImGui_InputTextWithHint(
            "###search",
            "Search...",
            search,
            sizeof(search),
            ImGuiInputTextFlags_None);

        ImVec2 avail = ImGui_GetContentRegionAvail();
        avail.y -= ImGui_GetFrameHeightWithSpacing();
        ImGui_PushFont(gui->fonts.mono);
        bool confirmed = false;
        if(ImGui_BeginListBox("###processes", avail)) {
            // FIXME: use clipper
            char label[257];
            for(size_t i = 0; i < list->processes_count; i++) {
                Process* process = list->processes[i];
                snprintf(
                    label,
                    sizeof(label),
                    "%s (%i, %s, %s)%s%s",
                    process->name[0] ? process->name : "unknown process",
                    process->pid,
                    process->user ? process->user : "unknown user",
                    process->executable ? process->executable : "unknown exe",
                    process->command ? ": " : "",
                    process->command ? process->command : "");
                if(search[0] != '\0' && strcasestr(label, search) == NULL) {
                    continue;
                }
                ImGui_PushIDInt(process->pid);
                bool is_selected = process->pid == selected;
                if(ImGui_SelectableBoolPtr(label, &is_selected, ImGuiSelectableFlags_None)) {
                    selected = process->pid;
                }
                if(ImGui_IsItemHovered(ImGuiHoveredFlags_None) &&
                   ImGui_IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    selected = process->pid;
                    confirmed = true;
                }
                if(is_selected) {
                    any_selected = true;
                    ImGui_SetItemDefaultFocus();
                }
                ImGui_PopID();
            }
            ImGui_EndListBox();
        }
        ImGui_PopFont();

        ImGui_BeginDisabled(!any_selected);
        if(ImGui_Button(ok) || confirmed) {
            process_list_free(list);
            list = NULL;
            memory_search_process_attach(gui->memory_search, selected);
            ImGui_CloseCurrentPopup();
        }
        ImGui_EndDisabled();

        ImGui_SameLine();
        if(ImGui_Button(cancel)) {
            process_list_free(list);
            list = NULL;
            ImGui_CloseCurrentPopup();
        }

        ImGui_EndPopup();
    }
}

static void gui_window_draw_toolbar(Gui* gui) {
    ImGui_BeginDisabled(memory_search_is_searching(gui->memory_search));
    if(memory_search_process_is_attached(gui->memory_search)) {
        if(ImGui_Button(detach_process)) {
            // FIXME: confirm button if search/scratchpad in use
            memory_search_process_detach(gui->memory_search);
        }
    } else {
        flt32_t width = ImGui_CalcTextSize(detach_process).x + gui->style->FramePadding.x * 2;
        if(ImGui_ButtonEx(attach_process, (ImVec2){width, 0.0f})) {
            ImGui_OpenPopup(attach_process, ImGuiPopupFlags_None);
        }
    }
    ImGui_EndDisabled();
    gui_window_draw_attach_process_popup(gui);

    ImGui_SameLineEx(0.0f, pane_spacing_mult * gui->style->ItemSpacing.x);

    ImGui_BeginDisabled(!memory_search_process_is_attached(gui->memory_search));
    const ImVec2 progressbar_size = {ImGui_GetContentRegionAvail().x, ImGui_GetFrameHeight()};
    ImGui_PushFont(gui->fonts.mono);
    if(memory_search_is_searching(gui->memory_search)) {
        flt32_t progress = memory_search_get_search_progress(gui->memory_search);
        char progress_str[5];
        snprintf(progress_str, sizeof(progress_str), "%.0f%%", progress * 100);
        ImGui_ProgressBar(progress, progressbar_size, progress_str);
    } else {
        char label[257] = "No Process Selected";
        const char* command = NULL;
        if(memory_search_process_is_attached(gui->memory_search)) {
            Process* process = memory_search_get_process(gui->memory_search);
            if(process != NULL) {
                snprintf(
                    label,
                    sizeof(label),
                    "%s (%i, %s, %s)",
                    process->name[0] ? process->name : "unknown process",
                    process->pid,
                    process->user ? process->user : "unknown user",
                    process->executable ? process->executable : "unknown exe");
                command = process->command;
            }
        }
        ImGui_ProgressBar(0.0f, progressbar_size, label);
        if(command && ImGui_IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
            ImGui_BeginItemTooltip();
            ImGui_PushTextWrapPos(progressbar_size.x);
            ImGui_Text("Commandline:\n%s", command);
            ImGui_PopTextWrapPos();
            ImGui_EndTooltip();
        }
    }
    ImGui_PopFont();
    ImGui_EndDisabled();
}

static void gui_window_draw_addresses_pane(Gui* gui, ImVec2 size) {
    ImGui_BeginDisabled(memory_search_is_searching(gui->memory_search));
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
            ImGui_CalcTextSize("0x1122334455667788").x,
            0);
        ImGui_TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
        ImGui_TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Previous", ImGuiTableColumnFlags_WidthStretch);
        MemorySearchResults* results = memory_search_get_results(gui->memory_search);
        MemorySearchResultBatch* batch = NULL;
        bool too_many = false;
        if(!memory_search_is_searching(gui->memory_search) && results->batches_count >= 1) {
            batch = &results->batches[results->batches_count - 1];
            too_many = batch->total_results_count > memory_search_update_max_results;
        }
        ImGui_TableSetupScrollFreeze(0, 1 + too_many);
        ImGui_PushFont(gui->fonts.base);
        ImGui_TableHeadersRow();
        if(too_many) {
            ImGui_TableNextRow();
            ImGui_TableNextColumn();
            ImVec2 text_pos = ImGui_GetCursorScreenPos();
            ImVec2 rect_pos = text_pos;
            ImGui_Text("");
            const char* text = "Too many results, no live updates!";
            ImVec2 text_size = ImGui_CalcTextSize(text);
            ImDrawList* foreground = ImGui_GetWindowDrawList();
            rect_pos.x -= gui->style->ItemSpacing.x;
            rect_pos.y -= gui->style->FrameBorderSize;
            ImVec2 rect_max = {
                rect_pos.x + size.x - gui->style->ScrollbarSize - gui->style->FrameBorderSize,
                rect_pos.y + ImGui_GetTextLineHeightWithSpacing() - gui->style->FrameBorderSize};
            text_pos.x += (size.x - text_size.x) / 2;
            ImGui_PushClipRect(rect_pos, rect_max, false);
            if(ImGui_IsMouseHoveringRect(rect_pos, rect_max) && ImGui_BeginTooltip()) {
                ImGui_Text(
                    "Max 100,000 results for live value updates"); // memory_search_update_max_results
                if(batch->total_results_count > imgui_clipper_max) {
                    ImGui_Text("Only first 1,000,000 results shown in list"); // imgui_clipper_max
                }
                ImGui_EndTooltip();
            }
            ImDrawList_AddRectFilled(foreground, rect_pos, rect_max, 0xFF000042);
            ImDrawList_AddText(
                foreground,
                text_pos,
                ImGui_GetColorU32ImVec4(gui->style->Colors[ImGuiCol_Text]),
                text);
            ImGui_PopClipRect();
        }
        ImGui_PopFont();

        if(batch != NULL) {
            MemorySearchResultBatch* prev_batch =
                results->batches_count >= 2 ? &results->batches[results->batches_count - 2] : NULL;
            ImGuiListClipper clipper = {0};
            ImGuiListClipper_Begin(
                &clipper,
                MIN(batch->total_results_count, imgui_clipper_max),
                ImGui_GetTextLineHeightWithSpacing());
            size_t set_i = 0;
            size_t sets_progress = 0;
            const char* type_str = NULL;
            MemorySearchResultSet* set = &batch->sets[set_i];
            MemorySearchResultSet* prev_set = (void*)-1;
            while(ImGuiListClipper_Step(&clipper)) {
                for(int32_t clip_i = clipper.DisplayStart; clip_i < clipper.DisplayEnd; clip_i++) {
                    ImGui_PushIDInt(clip_i);
                    while(clip_i - sets_progress >= set->results_count) {
                        sets_progress += set->results_count;
                        set_i++;
                        set = &batch->sets[set_i];
                        prev_set = (void*)-1;
                    }
                    if(prev_set == (void*)-1) {
                        type_str = memory_type_get_short_name(set->type);
                        if(prev_batch) {
                            for(size_t prev_set_i = 0; prev_set_i < prev_batch->sets_count;
                                prev_set_i++) {
                                prev_set = &prev_batch->sets[prev_set_i];
                                if(prev_set->type == set->type) {
                                    break;
                                }
                            }
                        } else {
                            prev_set = NULL;
                        }
                    }
                    size_t result_i = clip_i - sets_progress;
                    MemorySearchResultBase* base = memory_search_get_result_base(set, result_i);
                    MemorySearchResultDisplay display =
                        memory_search_get_result_display(set, result_i);
                    ImGui_TableNextRow();
                    // Address
                    // FIXME: if address is in region mapped from file, show filename+offset
                    ImGui_TableNextColumn();
                    ImGui_TextUnformatted(display.address_str);
                    // Type
                    ImGui_TableNextColumn();
                    ImGui_TextUnformatted(type_str);
                    // Value
                    ImGui_TableNextColumn();
                    ImGui_TextUnformatted(display.value_str);
                    // Previous
                    ImGui_TableNextColumn();
                    if(prev_set == NULL) {
                        ImGui_TextDisabled("N/A");
                    } else {
                        display = memory_search_get_result_display(prev_set, base->prev_i);
                        ImGui_TextUnformatted(display.value_str);
                    }
                    // Hitbox
                    ImGui_SameLine();
                    bool is_selected = false;
                    static size_t last_total = 0;
                    static int32_t last_selected = -1;
                    static int32_t selected[UINT8_MAX] = {-1};
                    if(last_total != batch->total_results_count) {
                        selected[0] = -1;
                        last_selected = -1;
                        last_total = batch->total_results_count;
                    }
                    for(uint8_t i = 0; i < COUNT_OF(selected) && selected[i] != -1; i++) {
                        if(selected[i] == clip_i) {
                            is_selected = true;
                            break;
                        }
                    }
                    ImVec4 hover_col = gui->style->Colors[ImGuiCol_HeaderHovered];
                    ImVec4 active_col = gui->style->Colors[ImGuiCol_HeaderActive];
                    hover_col.w *= 0.25;
                    active_col.w *= 0.25;
                    ImGui_PushStyleColorImVec4(ImGuiCol_HeaderHovered, hover_col);
                    ImGui_PushStyleColorImVec4(ImGuiCol_HeaderActive, active_col);
                    ImGui_SelectableBoolPtr(
                        "###hitbox",
                        &is_selected,
                        ImGuiSelectableFlags_SpanAllColumns);
                    if(ImGui_BeginPopupContextItem()) {
                        ImGui_PushFont(gui->fonts.base);
                        if(ImGui_Selectable(add_to_scratchpad)) {
                            if(!is_selected) {
                                memory_search_scratchpad_add(
                                    gui->memory_search,
                                    base->address,
                                    set->type);
                            } else {
                                for(uint8_t i = 0; i < COUNT_OF(selected) && selected[i] != -1;
                                    i++) {
                                    size_t add_i = selected[i];
                                    size_t add_set_i = 0;
                                    while(add_i >= batch->sets[add_set_i].results_count) {
                                        add_i -= batch->sets[add_set_i].results_count;
                                        add_set_i++;
                                    }
                                    MemorySearchResultSet* add_set = &batch->sets[add_set_i];
                                    MemoryAddress add_addr =
                                        memory_search_get_result_base(add_set, add_i)->address;
                                    memory_search_scratchpad_add(
                                        gui->memory_search,
                                        add_addr,
                                        add_set->type);
                                }
                                selected[0] = -1;
                                last_selected = -1;
                            }
                        }
                        ImGui_PopFont();
                        ImGui_EndPopup();
                    } else if(
                        ImGui_IsItemHovered(ImGuiHoveredFlags_None) &&
                        ImGui_IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        memory_search_scratchpad_add(gui->memory_search, base->address, set->type);
                        selected[0] = -1;
                        last_selected = -1;
                    } else if(ImGui_IsItemClicked()) {
                        uint8_t next_i = 0;
                        while(next_i < COUNT_OF(selected) && selected[next_i] != -1) {
                            next_i++;
                        }
                        if(ImGui_IsKeyDown(ImGuiMod_Shift)) {
                            if(last_selected != -1) {
                                int32_t range_min = clip_i > last_selected ? last_selected :
                                                                             clip_i;
                                int32_t range_max = clip_i > last_selected ? clip_i :
                                                                             last_selected;
                                for(int32_t range_i = range_min;
                                    range_i <= range_max && next_i != COUNT_OF(selected);
                                    range_i++) {
                                    bool already_selected = false;
                                    for(uint8_t i = 0; i < COUNT_OF(selected) && selected[i] != -1;
                                        i++) {
                                        if(selected[i] == range_i) {
                                            already_selected = true;
                                            break;
                                        }
                                    }
                                    if(!already_selected) {
                                        selected[next_i++] = range_i;
                                        if(next_i != COUNT_OF(selected)) {
                                            selected[next_i] = -1;
                                        }
                                    }
                                }
                            }
                        } else if(ImGui_IsKeyDown(ImGuiMod_Ctrl)) {
                            if(is_selected) {
                                for(uint8_t i = 0; i < COUNT_OF(selected) && selected[i] != -1;
                                    i++) {
                                    if(selected[i] == clip_i) {
                                        if(i == COUNT_OF(selected) - 1) {
                                            selected[i] = -1;
                                        } else {
                                            memmove(
                                                &selected[i],
                                                &selected[i + 1],
                                                sizeof(*selected) * (COUNT_OF(selected) - i - 1));
                                        }
                                        break;
                                    }
                                }
                            } else if(next_i != COUNT_OF(selected)) {
                                selected[next_i] = clip_i;
                                if(next_i + 1 != COUNT_OF(selected)) {
                                    selected[next_i + 1] = -1;
                                }
                            }
                        } else {
                            if(selected[0] != -1) {
                                selected[0] = -1;
                            } else {
                                selected[0] = clip_i;
                                selected[1] = -1;
                            }
                        }
                        last_selected = clip_i;
                    }
                    ImGui_PopStyleColorEx(2);
                    ImGui_PopID();
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
        MemorySearchResults* results = memory_search_get_results(gui->memory_search);
        MemorySearchParams params = memory_search_get_params(gui->memory_search);

        ImGui_BeginDisabled(memory_search_is_searching(gui->memory_search));
        if(results->batches_count == 0) {
            if(ImGui_Button(first_search)) {
                memory_search_begin(gui->memory_search);
            }
        } else {
            ImGui_BeginDisabled(results->current_results_count == 0);
            if(ImGui_Button(next_search)) {
                memory_search_next(gui->memory_search);
            }
            ImGui_EndDisabled();
        }
        ImGui_EndDisabled();

        ImGui_SameLine();

        if(memory_search_is_searching(gui->memory_search)) {
            flt32_t width = ImGui_CalcTextSize(undo_search).x + gui->style->FramePadding.x * 2;
            if(ImGui_ButtonEx(stop_search, (ImVec2){width, 0.0f})) {
                memory_search_stop(gui->memory_search);
            }
        } else {
            ImGui_BeginDisabled(results->batches_count < 2);
            if(ImGui_Button(undo_search)) {
                memory_search_undo(gui->memory_search);
            }
            ImGui_EndDisabled();
        }

        ImGui_SameLine();

        ImGui_BeginDisabled(memory_search_is_searching(gui->memory_search));
        ImGui_BeginDisabled(results->batches_count < 1);
        if(ImGui_Button(reset_search)) {
            memory_search_reset(gui->memory_search);
        }
        ImGui_EndDisabled();

        if(memory_search_is_searching(gui->memory_search)) {
            ImGui_Text("Searching...");
        } else {
            ImGui_Text(
                "Search Depth: %zu\tCurrent Results: %zu",
                results->batches_count,
                results->current_results_count);
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
            ImGui_PushFont(gui->fonts.mono);
            ImGui_SetNextItemWidth(-FLT_MIN);
            if(ImGui_InputText(
                   "###value",
                   temp_str,
                   sizeof(temp_str),
                   ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsDecimal)) {
                params.value = strtold(temp_str, NULL);
                memory_search_set_params(gui->memory_search, params);
            }
            ImGui_PopFont();

            ImGui_BeginDisabled(params.type <= MemoryTypeInteger);
            // Deviation
            ImGui_TableNextRow();
            ImGui_TableNextColumn();
            ImGui_AlignTextToFramePadding();
            ImGui_Text("Value ± :");
            ImGui_TableNextColumn();
            flt32_t deviation_alignment_width =
                (ImGui_GetContentRegionAvail().x - ImGui_CalcTextSize("Alignment:").x -
                 (pane_spacing_mult + 1) * gui->style->ItemSpacing.x) /
                2.0f;
            snprintf(temp_str, sizeof(temp_str), "%.*Lg", LDBL_DIG, params.deviation);
            ImGui_PushFont(gui->fonts.mono);
            ImGui_SetNextItemWidth(deviation_alignment_width);
            if(ImGui_InputText(
                   "###deviation",
                   temp_str,
                   sizeof(temp_str),
                   ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsDecimal)) {
                params.deviation = strtold(temp_str, NULL);
                memory_search_set_params(gui->memory_search, params);
            }
            ImGui_PopFont();
            ImGui_EndDisabled();

            ImGui_BeginDisabled(results->batches_count != 0);
            // Alignment
            ImGui_SameLineEx(0.0f, pane_spacing_mult * gui->style->ItemSpacing.x);
            ImGui_Text("Alignment:");
            ImGui_SameLine();
            snprintf(temp_str, sizeof(temp_str), "%u", params.alignment);
            ImGui_PushFont(gui->fonts.mono);
            ImGui_SetNextItemWidth(deviation_alignment_width);
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
            ImGui_PopFont();

            // Type
            ImGui_TableNextRow();
            ImGui_TableNextColumn();
            ImGui_AlignTextToFramePadding();
            ImGui_Text("Type:");
            ImGui_TableNextColumn();
            temp_int = params.type;
            ImGui_PushFont(gui->fonts.mono);
            ImGui_SetNextItemWidth(-FLT_MIN);
            if(ImGui_ComboChar(
                   "###type",
                   &temp_int,
                   memory_type_names,
                   COUNT_OF(memory_type_names))) {
                params.type = temp_int;
                memory_search_set_params(gui->memory_search, params);
            }
            ImGui_PopFont();
            ImGui_EndDisabled();

            ImGui_EndTable();
        }
        ImGui_EndDisabled();

        ImGui_EndDisabled();
    }
    ImGui_EndChild();
}

static void gui_window_draw_scratchpad_pane(Gui* gui, ImVec2 size) {
    ImGui_BeginDisabled(memory_search_is_searching(gui->memory_search));
    if(ImGui_BeginTableEx(
           "###scratchpad",
           5,
           ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
           size,
           0.0f)) {
        ImGui_PushFont(gui->fonts.mono);
        ImGui_TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed);
        ImGui_TableSetupColumnEx(
            "Address",
            ImGuiTableColumnFlags_WidthFixed,
            ImGui_CalcTextSize("0x1122334455667788").x,
            0);
        ImGui_TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
        ImGui_TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui_TableSetupScrollFreeze(0, 1);
        ImGui_PushFont(gui->fonts.base);
        ImGui_TableHeadersRow();
        ImGui_PopFont();
        MemorySearchScratchpad* scratchpad = memory_search_get_scratchpad(gui->memory_search);
        if(scratchpad->items_count >= 1) {
            ImGuiListClipper clipper = {0};
            ImGuiListClipper_Begin(
                &clipper,
                scratchpad->items_count,
                ImGui_GetFrameHeightWithSpacing());
            while(ImGuiListClipper_Step(&clipper)) {
                for(int32_t clip_i = clipper.DisplayStart; clip_i < clipper.DisplayEnd; clip_i++) {
                    MemorySearchScratchpadItem* item = &scratchpad->items[clip_i];
                    ImGui_PushIDInt(clip_i);
                    MemorySearchResultDisplay display = memory_search_get_scratchpad_display(item);
                    ImGui_TableNextRow();
                    // Active
                    // FIXME: keep applying the value
                    ImGui_TableNextColumn();
                    ImGui_Checkbox("###active", &item->active);
                    // Address
                    // FIXME: if address is in region mapped from file, show filename+offset
                    ImGui_TableNextColumn();
                    ImGui_TextUnformatted(display.address_str);
                    // Type
                    ImGui_TableNextColumn();
                    ImGui_TextUnformatted(memory_type_get_short_name(item->type));
                    // Value
                    ImGui_TableNextColumn();
                    static int32_t editing = -1;
                    if(clip_i == editing) {
                        char temp_str[33];
                        strlcpy(temp_str, display.value_str, sizeof(temp_str));
                        ImGui_SetNextItemWidth(-FLT_MIN);
                        ImGui_SetKeyboardFocusHere();
                        if(ImGui_InputText(
                               "###value",
                               temp_str,
                               sizeof(temp_str),
                               ImGuiInputTextFlags_AutoSelectAll |
                                   ImGuiInputTextFlags_CharsDecimal |
                                   ImGuiInputTextFlags_EnterReturnsTrue) ||
                           ImGui_IsItemDeactivated()) {
                            if(!ImGui_IsKeyDown(ImGuiKey_Escape)) {
                                flt128_t value = strtold(temp_str, NULL);
                                memory_search_scratchpad_set(gui->memory_search, item, value);
                            }
                            editing = -1;
                        }
                    } else {
                        ImGui_TextUnformatted(display.value_str);
                    }
                    // Description
                    ImGui_TableNextColumn();
                    ImGui_SetNextItemWidth(-FLT_MIN);
                    ImguiInputTextResizeCallbackCtx resize_ctx = {
                        .buf = &item->description,
                        .len = &item->description_len,
                    };
                    ImGui_InputTextEx(
                        "###description",
                        item->description,
                        item->description_len,
                        ImGuiInputTextFlags_CallbackResize,
                        imgui_input_text_resize_callback,
                        &resize_ctx);
                    // Hitbox
                    ImGui_SameLine();
                    bool is_selected = false;
                    static size_t last_total = 0;
                    static int32_t last_selected = -1;
                    static int32_t selected[UINT8_MAX] = {-1};
                    if(last_total != scratchpad->items_count) {
                        selected[0] = -1;
                        last_selected = -1;
                        last_total = scratchpad->items_count;
                    }
                    for(uint8_t i = 0; i < COUNT_OF(selected) && selected[i] != -1; i++) {
                        if(selected[i] == clip_i) {
                            is_selected = true;
                            break;
                        }
                    }
                    ImGui_SetCursorPosY(ImGui_GetCursorPosY() - gui->style->FramePadding.y);
                    ImVec4 hover_col = gui->style->Colors[ImGuiCol_HeaderHovered];
                    ImVec4 active_col = gui->style->Colors[ImGuiCol_HeaderActive];
                    hover_col.w *= 0.25;
                    active_col.w *= 0.25;
                    ImGui_PushStyleColorImVec4(ImGuiCol_HeaderHovered, hover_col);
                    ImGui_PushStyleColorImVec4(ImGuiCol_HeaderActive, active_col);
                    ImGui_SelectableBoolPtrEx(
                        "###hitbox",
                        &is_selected,
                        ImGuiSelectableFlags_SpanAllColumns,
                        (ImVec2){0.0f, ImGui_GetFrameHeight()});
                    if(ImGui_BeginPopupContextItem()) {
                        ImGui_PushFont(gui->fonts.base);
                        if(ImGui_Selectable(remove_from_scratchpad)) {
                            if(!is_selected) {
                                memory_search_scratchpad_del(gui->memory_search, item);
                            } else {
                                for(uint8_t del_i = 0;
                                    del_i < COUNT_OF(selected) && selected[del_i] != -1;
                                    del_i++) {
                                    MemorySearchScratchpadItem* del_item =
                                        &scratchpad->items[selected[del_i]];
                                    memory_search_scratchpad_del(gui->memory_search, del_item);
                                    for(uint8_t fix_i = del_i + 1;
                                        fix_i < COUNT_OF(selected) && selected[fix_i] != -1;
                                        fix_i++) {
                                        if(selected[fix_i] > selected[del_i]) {
                                            selected[fix_i]--;
                                        }
                                    }
                                }
                                selected[0] = -1;
                                last_selected = -1;
                            }
                        }
                        ImGui_PopFont();
                        ImGui_EndPopup();
                    } else if(
                        ImGui_IsItemHovered(ImGuiHoveredFlags_None) &&
                        ImGui_IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        editing = clip_i;
                        selected[0] = -1;
                        last_selected = -1;
                    } else if(ImGui_IsItemClicked()) {
                        uint8_t next_i = 0;
                        while(next_i < COUNT_OF(selected) && selected[next_i] != -1) {
                            next_i++;
                        }
                        if(ImGui_IsKeyDown(ImGuiMod_Shift)) {
                            if(last_selected != -1) {
                                int32_t range_min = clip_i > last_selected ? last_selected :
                                                                             clip_i;
                                int32_t range_max = clip_i > last_selected ? clip_i :
                                                                             last_selected;
                                for(int32_t range_i = range_min;
                                    range_i <= range_max && next_i != COUNT_OF(selected);
                                    range_i++) {
                                    bool already_selected = false;
                                    for(uint8_t i = 0; i < COUNT_OF(selected) && selected[i] != -1;
                                        i++) {
                                        if(selected[i] == range_i) {
                                            already_selected = true;
                                            break;
                                        }
                                    }
                                    if(!already_selected) {
                                        selected[next_i++] = range_i;
                                        if(next_i != COUNT_OF(selected)) {
                                            selected[next_i] = -1;
                                        }
                                    }
                                }
                            }
                        } else if(ImGui_IsKeyDown(ImGuiMod_Ctrl)) {
                            if(is_selected) {
                                for(uint8_t i = 0; i < COUNT_OF(selected) && selected[i] != -1;
                                    i++) {
                                    if(selected[i] == clip_i) {
                                        if(i == COUNT_OF(selected) - 1) {
                                            selected[i] = -1;
                                        } else {
                                            memmove(
                                                &selected[i],
                                                &selected[i + 1],
                                                sizeof(*selected) * (COUNT_OF(selected) - i - 1));
                                        }
                                        break;
                                    }
                                }
                            } else if(next_i != COUNT_OF(selected)) {
                                selected[next_i] = clip_i;
                                if(next_i + 1 != COUNT_OF(selected)) {
                                    selected[next_i + 1] = -1;
                                }
                            }
                        } else {
                            if(selected[0] != -1) {
                                selected[0] = -1;
                            } else {
                                selected[0] = clip_i;
                                selected[1] = -1;
                            }
                        }
                        last_selected = clip_i;
                    }
                    ImGui_PopStyleColorEx(2);
                    ImGui_PopID();
                }
            }
        }

        ImGui_PopFont();
        ImGui_EndTable();
    }
    ImGui_EndDisabled();
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
