#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

#include "logger.h"
#include "util.h"
#include "child.h"

int child_process(void* arg) {
    struct container_init_data* init_data = (struct container_init_data*) arg;
    struct run_config* config = init_data->config;
    UNUSED(config);

    int readpipefd = init_data->readpipefd;
    char buf;

    IF_CLEANUP(read(readpipefd, &buf, 1), -1);
    LOG_SYSERR_AND_CLEANUP(unshare(CLONE_NEWCGROUP), -1);
    LOG_SYSERR_AND_CLEANUP(close(readpipefd), -1);

    free(init_data);

    return 0;

cleanup:
    if (readpipefd >= 0) close(readpipefd);
    if (init_data != NULL) free(init_data);
    return -1;
}
