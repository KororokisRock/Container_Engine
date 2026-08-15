#define _GNU_SOURCE
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "logger.h"
#include "util.h"
#include "cgroups_configuration.h"
#include "container_cleanup.h"

static int remove_file_or_dir(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    UNUSED(sb);
    UNUSED(typeflag);
    UNUSED(ftwbuf);
    return remove(fpath);
}

static int cleanup_overlayfs(pid_t container_pid) {
    char path_to_container[512];
    LOG_SYSERR_AND_CLEANUP(snprintf(path_to_container, sizeof(path_to_container), "/tmp/container_%d", container_pid), -1);

    int flags = FTW_DEPTH | FTW_PHYS;

    LOG_SYSERR_AND_CLEANUP(nftw(path_to_container, remove_file_or_dir, 64, flags), -1);

    return 0;

cleanup:
    return -1;
}

static int cleanup_cgroup(pid_t container_pid) {
    char* cgroup_path = NULL;

    IF_CLEANUP(get_cgroup_path(container_pid, &cgroup_path), -1);

    LOG_SYSERR_AND_CLEANUP(rmdir(cgroup_path), -1);

    free(cgroup_path);
    return 0;

cleanup:
    if (cgroup_path != NULL) free(cgroup_path);
    return -1;
}

int cleanup_container_resources(pid_t container_pid) {
    int status = 0;

    if (cleanup_overlayfs(container_pid) != 0) {
        status = -1;
    }

    if (cleanup_cgroup(container_pid) != 0) {
        status = -1;
    }

    return status;
}
