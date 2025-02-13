#include "process.h"

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

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
    } else {
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
    }

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

static char* process_read_proc_file(Process* process, const char* name) {
    char temp_str[257];

    snprintf(temp_str, sizeof(temp_str), "/proc/%i/%s", process->pid, name);
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

Process* process_init(ProcessPid pid) {
    if(!process_pid_is_alive(pid)) {
        return NULL;
    }

    Process* process = malloc(sizeof(Process));
    process->pid = pid;
    char temp_str[31];
    int32_t res;

    char* name = process_read_proc_file(process, "comm");
    if(name == NULL) {
        process->name[0] = '\0';
    } else {
        strlcpy(process->name, name, sizeof(process->name));
        free(name);
    }

    snprintf(temp_str, sizeof(temp_str), "/proc/%i/exe", process->pid);
    char executable[257];
    res = readlink(temp_str, executable, sizeof(executable));
    if(res < 0) {
        if(res == -1) {
            if(errno == ENOENT) {
                res = 0;
            } else {
                perror(temp_str);
                free(process);
                return NULL;
            }
        } else {
            unreachable();
        }
    }
    if(res == sizeof(executable)) {
        res--;
    }
    if(res == 0) {
        process->executable = NULL;
    } else {
        executable[res] = '\0';
        process->executable = strdup(executable);
    }

    process->command = process_read_proc_file(process, "cmdline");

    snprintf(temp_str, sizeof(temp_str), "/proc/%i", process->pid);
    struct stat process_stat;
    res = stat(temp_str, &process_stat);
    if(res != 0) {
        if(res == -1) {
            perror(temp_str);
            process_free(process);
            return NULL;
        }
        unreachable();
    }
    if(uid_usernames == NULL) {
        load_uid_usernames();
    }
    for(size_t i = 0; i < uid_usernames_count; i++) {
        if(uid_usernames[i].uid == process_stat.st_uid) {
            process->user = strdup(uid_usernames[i].username);
            break;
        }
    }
    if(process->user == NULL) {
        snprintf(temp_str, sizeof(temp_str), "%i", process_stat.st_uid);
        process->user = strdup(temp_str);
    }

    return process;
}

void process_free(Process* process) {
    char* executable = process->executable;
    char* command = process->command;
    char* user = process->user;
    process->executable = NULL;
    process->command = NULL;
    process->user = NULL;
    if(executable != NULL) {
        free(executable);
    }
    if(command != NULL) {
        free(command);
    }
    if(user != NULL) {
        free(user);
    }
    free(process);
}
