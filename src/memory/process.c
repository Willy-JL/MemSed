#include "process.h"

#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

static char* memory_process_read_proc_file(MemoryProcess* memory_process, const char* name) {
    char temp_str[257];

    snprintf(temp_str, sizeof(temp_str), "/proc/%i/%s", memory_process->pid, name);
    FILE* file = fopen(temp_str, "r");
    if(file == NULL) {
        return NULL;
    }
    fgets(temp_str, sizeof(temp_str), file);
    fclose(file);

    if(strcmp(name, "comm") == 0) {
        temp_str[strlen(temp_str) - 1] = '\0'; // Remove trailing newline
    }

    return strdup(temp_str);
}

MemoryProcess* memory_process_init(MemoryProcessPid pid) {
    MemoryProcess* memory_process = malloc(sizeof(MemoryProcess));
    memory_process->pid = pid;
    if(!memory_process_is_alive(memory_process)) {
        free(memory_process);
        return NULL;
    }
    memory_process->name = memory_process_read_proc_file(memory_process, "comm");
    memory_process->command = memory_process_read_proc_file(memory_process, "cmdline");

    char temp_str[33];
    snprintf(temp_str, sizeof(temp_str), "/proc/%i", memory_process->pid);
    struct stat process_stat;
    stat(temp_str, &process_stat);
    // FIXME: convert UID to username
    snprintf(temp_str, sizeof(temp_str), "%i", process_stat.st_uid);
    memory_process->user = strdup(temp_str);

    return memory_process;
}

bool memory_process_is_alive(MemoryProcess* memory_process) {
    if(kill(memory_process->pid, 0) == 0 || errno == ESRCH) {
        return false;
    }
    return true;
}

void memory_process_free(MemoryProcess* memory_process) {
    char* name = memory_process->name;
    char* command = memory_process->command;
    char* user = memory_process->user;
    memory_process->name = NULL;
    memory_process->command = NULL;
    memory_process->user = NULL;
    if(name) {
        free(name);
    }
    if(command) {
        free(command);
    }
    if(user) {
        free(user);
    }
    free(memory_process);
}
