#include "list.h"

#include <dirent.h>

ProcessList* process_list_init() {
    size_t capacity = 1;
    size_t count = 0;
    ProcessList* list = malloc(sizeof(ProcessList) + sizeof(Process*) * capacity);
    list->processes_count = count;

    DIR* dir = opendir("/proc");
    if(dir == NULL) {
        perror("/proc");
        process_list_free(list);
        return 0;
    }

    struct dirent* item;
    while((item = readdir(dir)) != NULL) {
        size_t name_len = strlen(item->d_name);
        bool is_pid = true;
        for(size_t i = 0; i < name_len; i++) {
            if(!isdigit(item->d_name[i])) {
                is_pid = false;
                break;
            }
        }
        if(!is_pid) {
            continue;
        }
        ProcessPid pid;
        if(sscanf(item->d_name, "%i", &pid) != 1) {
            continue;
        }
        Process* process = process_init(pid);
        if(process == NULL) {
            continue;
        }
        if(count == capacity) {
            capacity *= 2;
            list = realloc(list, sizeof(ProcessList) + sizeof(Process*) * capacity);
        }
        list->processes[count] = process;
        count++;
    }
    closedir(dir);
    list = realloc(list, sizeof(ProcessList) + sizeof(Process*) * count);
    list->processes_count = count;

    return list;
}

void process_list_free(ProcessList* list) {
    size_t processes_count = list->processes_count;
    list->processes_count = 0;
    for(size_t i = 0; i < processes_count; i++) {
        process_free(list->processes[i]);
    }
    free(list);
}
