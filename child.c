#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "util.h"
#include "child.h"
#include "overlay_configuration.h"

int child_process(void* arg) {
    struct container_init_data* init_data = (struct container_init_data*) arg;
    struct run_config* config = init_data->config;
    pid_t container_pid = init_data->container_pid;
    UNUSED(config);

    int readpipefd = init_data->readpipefd;
    char buf;

    IF_CLEANUP(read(readpipefd, &buf, 1), -1);
    LOG_SYSERR_AND_CLEANUP(unshare(CLONE_NEWCGROUP), -1);
    LOG_SYSERR_AND_CLEANUP(close(readpipefd), -1);

    free(init_data);
    init_data = NULL;

    IF_CLEANUP(process_overlayfs_creation_and_hostname_set(container_pid, config), -1);

    return 0;

cleanup:
    if (readpipefd >= 0) close(readpipefd);
    if (init_data != NULL) free(init_data);
    return -1;
}
