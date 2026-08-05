#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "cli_parser.h"
#include "run.h"
#include "util.h"
#include "namespaces_configuration.h"

int handle_run(struct run_config* config) {
    int syncpipefd[2] = {-1, -1};
    pid_t container_pid;
    IF_CLEANUP(process_container_creation_and_namespaces_isolation(config, syncpipefd, &container_pid), -1);

    char msg = '1';
    LOG_ERROR_AND_CLEANUP(write(syncpipefd[1], &msg, 1), -1);

    close(syncpipefd[1]);
    syncpipefd[1] = -1;

    LOG_ERROR_AND_CLEANUP(waitpid(container_pid, NULL, 0), -1);

    return 0;

cleanup:
    if (syncpipefd[0] >= 0) close(syncpipefd[0]);
    if (syncpipefd[1] >= 0) close(syncpipefd[1]);
    return -1;
}
