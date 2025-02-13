#include "regions.h"

ProcessRegions* process_regions_init(ProcessPid pid) {
    char line[1024];

    snprintf(line, sizeof(line), "/proc/%i/maps", pid);
    FILE* maps = fopen(line, "r");
    if(maps == NULL) {
        perror(line);
        return NULL;
    }

    size_t capacity = 1;
    size_t count = 0;
    ProcessRegions* regions = malloc(sizeof(ProcessRegions) + sizeof(ProcessRegion) * capacity);
    regions->regions_count = count;

    while(fgets(line, sizeof(line), maps) != NULL) {
        // address           perms offset  dev   inode       pathname
        // 00400000-00452000 rwxs 00000000 08:02 173521      /usr/bin/dbus-daemon
        line[strlen(line) - 1] = '\0'; // Remove trailing newline
        MemoryAddress start, end;
        char r, w, x, s;
        uint32_t file_offset;
        int32_t file_path_start;
        if(sscanf(
               line,
               "%lx-%lx %c%c%c%c %x %*x:%*x %*u %n",
               &start,
               &end,
               &r,
               &w,
               &x,
               &s,
               &file_offset,
               &file_path_start) == 7) {
            char* file_path = line + file_path_start;

            if(count == capacity) {
                capacity *= 2;
                regions =
                    realloc(regions, sizeof(ProcessRegions) + sizeof(ProcessRegion) * capacity);
            }
            ProcessRegion* region = &regions->regions[count];

            region->start = start;
            region->end = end;

            if(file_path[0] == '\0') {
                region->type = ProcessRegionTypeAnonymous;
            } else if(strcmp(file_path, "[heap]") == 0) {
                region->type = ProcessRegionTypeHeap;
            } else if(strcmp(file_path, "[stack]") == 0) {
                region->type = ProcessRegionTypeStack;
            } else {
                region->type = ProcessRegionTypeFile;
            }

            region->flags = 0;
            if(r == 'r') {
                region->flags |= ProcessRegionFlagRead;
            }
            if(w == 'w') {
                region->flags |= ProcessRegionFlagWrite;
            }
            if(x == 'x') {
                region->flags |= ProcessRegionFlagExecute;
            }
            if(s == 's') {
                region->flags |= ProcessRegionFlagShared;
            }

            if(region->type == ProcessRegionTypeFile) {
                region->file_path = strdup(file_path);
            } else {
                region->file_path = NULL;
            }
            region->file_offset = file_offset;

            count++;
        }
    }
    fclose(maps);
    regions = realloc(regions, sizeof(ProcessRegions) + sizeof(ProcessRegion) * count);
    regions->regions_count = count;

    return regions;
}

void process_regions_free(ProcessRegions* regions) {
    size_t regions_count = regions->regions_count;
    regions->regions_count = 0;
    for(size_t i = 0; i < regions_count; i++) {
        ProcessRegion* region = &regions->regions[i];
        if(region->file_path != NULL) {
            free(region->file_path);
        }
    }
    free(regions);
}
