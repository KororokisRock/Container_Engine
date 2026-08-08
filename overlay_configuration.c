#define _GNU_SOURCE

#include "config.h"
#include "logger.h"
#include "util.h"
#include "overlay_configuration.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


static int overlayfs_dirs_creation(pid_t container_pid, struct path_to_containers_dirs* container_paths) {
    int path_to_container_size = -1;

    char path_to_container_tmp[512];
    LOG_SYSERR_AND_CLEANUP(path_to_container_size = snprintf(path_to_container_tmp, 512, "/tmp/container_%d", container_pid), -1);
    LOG_SYSERR_AND_CLEANUP(mkdir(path_to_container_tmp, 0755), -1);

    char path_to_upper[512];
    LOG_SYSERR_AND_CLEANUP(snprintf(path_to_upper, 512, "%s/upper", path_to_container_tmp), -1);
    LOG_SYSERR_AND_CLEANUP(mkdir(path_to_upper, 0755), -1);

    container_paths->upper = strdup(path_to_upper);

    char path_to_work[512];
    LOG_SYSERR_AND_CLEANUP(snprintf(path_to_work, 512, "%s/work", path_to_container_tmp), -1);
    LOG_SYSERR_AND_CLEANUP(mkdir(path_to_work, 0755), -1);

    container_paths->work = strdup(path_to_work);

    char path_to_merge[512];
    LOG_SYSERR_AND_CLEANUP(snprintf(path_to_merge, 512, "%s/merge", path_to_container_tmp), -1);
    LOG_SYSERR_AND_CLEANUP(mkdir(path_to_merge, 0755), -1);

    container_paths->merge = strdup(path_to_merge);

    return 0;
cleanup:
    return -1;
}


static int mount_overlayfs(struct path_to_containers_dirs* container_paths) {
    char mount_opts[4096];
    LOG_SYSERR_AND_CLEANUP(snprintf(
        mount_opts,
        sizeof(mount_opts),
        "lowerdir=%s,upperdir=%s,workdir=%s,userxattr",
        container_paths->lower,
        container_paths->upper,
        container_paths->work),
    -1);

    LOG_SYSERR_AND_CLEANUP(mount("overlay", container_paths->merge, "overlay", 0, mount_opts), -1);

    return 0;

cleanup:
    return -1;
}


static int mount_proc_filesystems(const char* merged_path) {
    char proc_path[512];
    LOG_SYSERR_AND_CLEANUP(snprintf(proc_path, sizeof(proc_path), "%s/proc", merged_path), -1);

    int proc_mkdir_status;
    if ((proc_mkdir_status = mkdir(proc_path, 0755)) == -1) {
        if (errno != EEXIST) {
            LOG_SYSERR_AND_CLEANUP(proc_mkdir_status, -1);
        }
    }
    
    LOG_SYSERR_AND_CLEANUP(mount("proc", proc_path, "proc", 0, NULL), -1);
    return 0;
cleanup:
    return -1;
}


int process_overlayfs_creation_and_hostname_set(pid_t container_pid, struct run_config* config) {
    struct path_to_containers_dirs* container_paths = calloc(1, sizeof(struct path_to_containers_dirs));
    container_paths->lower = strdup(config->rootfs_path);

    IF_CLEANUP(overlayfs_dirs_creation(container_pid, container_paths), -1);

    IF_CLEANUP(mount_overlayfs(container_paths), -1);

    IF_CLEANUP(mount_proc_filesystems(container_paths->merge), -1);

    if (config->hostname) {
        IF_CLEANUP(sethostname(config->hostname, strlen(config->hostname)), -1);
    }

    return 0;

cleanup:
    if (container_paths->lower != NULL) free(container_paths->lower);
    if (container_paths->upper != NULL) free(container_paths->upper);
    if (container_paths->work != NULL) free(container_paths->work);
    if (container_paths->merge != NULL) free(container_paths->merge);
    if (container_paths != NULL) free(container_paths);
    return -1;
}

