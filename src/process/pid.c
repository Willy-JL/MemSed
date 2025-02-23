#include "pid.h"

#include <signal.h>

bool process_pid_is_alive(ProcessPid pid) {
    int32_t res = kill(pid, 0);
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
            perror("Unknown result checking if PID is alive");
            return false;
        }
    }
    unreachable();
}

void process_pid_pause(ProcessPid pid) {
    int32_t res = kill(pid, SIGSTOP);
    if(res == 0) {
        return;
    }
    if(res == -1) {
        switch(errno) {
        case ESRCH:
        case EPERM:
            return;
        default:
            perror("Unknown result sending STOP signal");
            return;
        }
    }
    unreachable();
}

void process_pid_resume(ProcessPid pid) {
    int32_t res = kill(pid, SIGCONT);
    if(res == 0) {
        return;
    }
    if(res == -1) {
        switch(errno) {
        case ESRCH:
        case EPERM:
            return;
        default:
            perror("Unknown result sending CONT signal");
            return;
        }
    }
    unreachable();
}
