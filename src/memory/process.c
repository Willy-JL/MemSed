#include "process.h"

#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

typedef struct {
    uint32_t uid;
    char* username;
} UidUsername;

static UidUsername* uid_usernames = NULL;
static size_t uid_usernames_count = 0;

static void load_uid_usernames() {
    char temp_str[257];
    size_t capacity = 1;
    size_t count = 0;
    UidUsername* new_uid_usernames = malloc(sizeof(UidUsername) * capacity);
    FILE* passwd = fopen("/etc/passwd", "r");
    if(passwd == NULL) {
        perror("/etc/passwd");
        goto bail;
    }
    while(fgets(temp_str, sizeof(temp_str), passwd) != NULL) {
        // "username:x:uid:..."
        size_t pos = 0;
        while(temp_str[pos] != '\0' && temp_str[pos] != ':') {
            pos++;
        }
        if(temp_str[pos] != ':') {
            continue;
        }
        uint32_t uid;
        if(sscanf(&temp_str[pos + 3], "%u", &uid) != 1) {
            continue;
        }
        temp_str[pos] = '\0';
        if(count == capacity) {
            capacity *= 2;
            new_uid_usernames = realloc(new_uid_usernames, sizeof(UidUsername) * capacity);
        }
        new_uid_usernames[count].uid = uid;
        new_uid_usernames[count].username = strdup(temp_str);
        count++;
    }
    fclose(passwd);
    if(count > 0) {
        new_uid_usernames = realloc(new_uid_usernames, sizeof(UidUsername) * count);
    }

bail:
    UidUsername* old_uid_usernames = uid_usernames;
    size_t old_count = uid_usernames_count;
    uid_usernames = new_uid_usernames;
    uid_usernames_count = count;
    if(old_uid_usernames) {
        for(size_t i = 0; i < old_count; i++) {
            free(old_uid_usernames[i].username);
        }
        free(old_uid_usernames);
    }
}

static char* memory_process_read_proc_file(MemoryProcess* memory_process, const char* name) {
    char temp_str[257];

    snprintf(temp_str, sizeof(temp_str), "/proc/%i/%s", memory_process->pid, name);
    FILE* file = fopen(temp_str, "r");
    if(file == NULL) {
        perror(temp_str);
        return NULL;
    }
    if(fgets(temp_str, sizeof(temp_str), file) == NULL) {
        temp_str[0] = '\0';
    }
    fclose(file);

    size_t len = strlen(temp_str);
    if(strcmp(name, "comm") == 0) {
        if(len > 1) {
            len--;
            temp_str[len] = '\0'; // Remove trailing newline
        }
    }

    return len ? strdup(temp_str) : NULL;
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
    int32_t res = stat(temp_str, &process_stat);
    if(res != 0) {
        if(res == -1) {
            perror(temp_str);
            memory_process_free(memory_process);
            return NULL;
        }
        unreachable();
    }
    if(uid_usernames == NULL) {
        load_uid_usernames();
    }
    for(size_t i = 0; i < uid_usernames_count; i++) {
        if(uid_usernames[i].uid == process_stat.st_uid) {
            memory_process->user = strdup(uid_usernames[i].username);
            break;
        }
    }
    if(memory_process->user == NULL) {
        snprintf(temp_str, sizeof(temp_str), "%i", process_stat.st_uid);
        memory_process->user = strdup(temp_str);
    }

    return memory_process;
}

bool memory_process_is_alive(MemoryProcess* memory_process) {
    int32_t res = kill(memory_process->pid, 0);
    if(res == 0) {
        return true;
    }
    if(res == -1) {
        switch(errno) {
        case ESRCH:
            return false;
        case EPERM:
            return true;
        default:
            perror("Unknown result checking if process is alive");
            return false;
        }
    }
    unreachable();
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

MemoryProcessList* memory_process_list_init() {
    size_t capacity = 1;
    size_t count = 0;
    MemoryProcessList* list =
        malloc(sizeof(MemoryProcessList) + sizeof(MemoryProcess*) * capacity);
    list->processes_count = count;

    DIR* dir = opendir("/proc");
    if(dir == NULL) {
        perror("/proc");
        memory_process_list_free(list);
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
        MemoryProcessPid pid;
        if(sscanf(item->d_name, "%i", &pid) != 1) {
            continue;
        }
        MemoryProcess* process = memory_process_init(pid);
        if(process == NULL) {
            continue;
        }
        if(count == capacity) {
            capacity *= 2;
            list = realloc(list, sizeof(MemoryProcessList) + sizeof(MemoryProcess*) * capacity);
        }
        list->processes[count] = process;
        count++;
    }
    list = realloc(list, sizeof(MemoryProcessList) + sizeof(MemoryProcess*) * count);
    list->processes_count = count;

    return list;
}

void memory_process_list_free(MemoryProcessList* list) {
    size_t processes_count = list->processes_count;
    list->processes_count = 0;
    for(size_t i = 0; i < processes_count; i++) {
        memory_process_free(list->processes[i]);
    }
    free(list);
}
