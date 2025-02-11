#include "process.h"

char* memory_process_get_description(MemoryProcessPid pid) {
    char temp_str[257];
    char process_description[513];
    FILE* file;

    snprintf(temp_str, sizeof(temp_str), "/proc/%i/comm", pid);
    file = fopen(temp_str, "r");
    if(file == NULL) {
        return NULL;
    }
    fgets(temp_str, sizeof(temp_str), file);
    fclose(file);
    temp_str[strlen(temp_str) - 1] = '\0'; // Remove trailing newline
    snprintf(process_description, sizeof(process_description), "%s (", temp_str);

    snprintf(temp_str, sizeof(temp_str), "/proc/%i/cmdline", pid);
    file = fopen(temp_str, "r");
    if(file == NULL) {
        return NULL;
    }
    fgets(temp_str, sizeof(temp_str), file);
    fclose(file);
    strlcat(process_description, temp_str, sizeof(process_description));
    strlcat(process_description, ")", sizeof(process_description));

    return strdup(process_description);
}
