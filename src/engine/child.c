#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "util.h"
#include "child.h"
#include "overlay_configuration.h"

int child_process(void* arg) {
    struct container_init_data* init_data = (struct container_init_data*) arg;
    struct run_config* config = init_data->config;
    UNUSED(config);

    int readpipefd = init_data->readpipefd;
    pid_t container_pid = -1;

    struct path_to_containers_dirs* container_paths = NULL;

    IF_CLEANUP(read(readpipefd, &container_pid, sizeof(pid_t)), -1);

    LOG_SYSERR_AND_CLEANUP(unshare(CLONE_NEWCGROUP), -1);
    LOG_SYSERR_AND_CLEANUP(close(readpipefd), -1);

    free(init_data);
    init_data = NULL;

    container_paths = calloc(1, sizeof(struct path_to_containers_dirs));
    IF_CLEANUP(process_overlayfs_creation_and_hostname_set(container_paths, container_pid, config), -1);

    LOG_SYSERR_AND_CLEANUP(chdir(container_paths->merge), -1);

    char path_to_put_old[512];
    LOG_SYSERR_AND_CLEANUP(snprintf(path_to_put_old, 512, "%s/put_old", container_paths->merge), -1);
    LOG_SYSERR_AND_CLEANUP(mkdir(path_to_put_old, 0755), -1);   

    LOG_SYSERR_AND_CLEANUP(syscall(SYS_pivot_root, container_paths->merge, path_to_put_old), -1);

    LOG_SYSERR_AND_CLEANUP(chdir("/"), -1);

    LOG_SYSERR_AND_CLEANUP(umount2("/put_old", MNT_DETACH), -1);

    LOG_SYSERR_AND_CLEANUP(rmdir("/put_old"), -1);

    LOG_SYSERR_AND_CLEANUP(execvp(config->target_cmd[0], config->target_cmd), -1);

    LOG_ERROR("execvp failed!");

cleanup:
    if (readpipefd >= 0) close(readpipefd);
    if (init_data != NULL) free(init_data);
    if (container_paths != NULL) {
        if (container_paths->lower != NULL) free(container_paths->lower);
        if (container_paths->upper != NULL) free(container_paths->upper);
        if (container_paths->work != NULL) free(container_paths->work);
        if (container_paths->merge != NULL) free(container_paths->merge);
        free(container_paths);
    }
    return -1;
}
