#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "config.h"
#include "logger.h"
#include "run.h"
#include "util.h"
#include "namespaces_configuration.h"
#include "cgroups_configuration.h"
#include "container_cleanup.h"

int handle_run(struct run_config* config) {
    int syncpipefd[2] = {-1, -1};
    pid_t container_pid = -1;
    IF_CLEANUP(process_container_creation_and_namespaces_isolation(config, syncpipefd, &container_pid), -1);

    IF_CLEANUP(process_cgroup_creation_and_setting_limits(container_pid, config), -1);

    LOG_SYSERR_AND_CLEANUP(write(syncpipefd[1], &container_pid, sizeof(pid_t)), -1);

    close(syncpipefd[1]);
    syncpipefd[1] = -1;

    LOG_SYSERR_AND_CLEANUP(waitpid(container_pid, NULL, 0), -1);

    return cleanup_container_resources(container_pid);

cleanup:
    if (syncpipefd[0] >= 0) close(syncpipefd[0]);
    if (syncpipefd[1] >= 0) close(syncpipefd[1]);

    if (container_pid > 0) {
        cleanup_container_resources(container_pid);
    }

    return -1;
}
