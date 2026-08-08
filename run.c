#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "config.h"
#include "logger.h"
#include "run.h"
#include "util.h"
#include "namespaces_configuration.h"
#include "cgroups_configuration.h"

int handle_run(struct run_config* config) {
    int syncpipefd[2] = {-1, -1};
    pid_t container_pid;
    IF_CLEANUP(process_container_creation_and_namespaces_isolation(config, syncpipefd, &container_pid), -1);

    IF_CLEANUP(process_cgroup_creation_and_setting_limits(container_pid, config), -1);

    char msg = '1';
    LOG_SYSERR_AND_CLEANUP(write(syncpipefd[1], &msg, 1), -1);

    close(syncpipefd[1]);
    syncpipefd[1] = -1;

    LOG_SYSERR_AND_CLEANUP(waitpid(container_pid, NULL, 0), -1);

    return 0;

cleanup:
    if (syncpipefd[0] >= 0) close(syncpipefd[0]);
    if (syncpipefd[1] >= 0) close(syncpipefd[1]);
    return -1;
}
