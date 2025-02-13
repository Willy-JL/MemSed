#include "pid.h"

#include <errno.h>
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
