#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>


#include "config.h"
#include "logger.h"
#include "util.h"
#include "cgroups_configuration.h"

static int get_self_cgroup_value(char** self_cgroup_value) {
    FILE* file = NULL;
    LOG_SYSERR_AND_CLEANUP(file = fopen("/proc/self/cgroup", "r"), NULL);

    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "0::", 3) == 0) {
            *self_cgroup_value = strdup(line + 3);
            fclose(file);
            return 0;
        }
    }

cleanup:
    if (file != NULL) fclose(file);
    return -1;
}

int get_cgroup_path(pid_t container_pid, char** cgroup_path) {
    char* self_cgroup_value = NULL;
    IF_CLEANUP(get_self_cgroup_value(&self_cgroup_value), -1);

    char subtree_path[512];
    snprintf(subtree_path, sizeof(subtree_path), "/sys/fs/cgroup%s/cgroup.subtree_control", self_cgroup_value);
    
    int subtree_fd = open(subtree_path, O_WRONLY);
    if (subtree_fd >= 0) {
        UNUSED(write(subtree_fd, "+memory", 7));
        UNUSED(write(subtree_fd, "+pids", 5));
        UNUSED(write(subtree_fd, "+cpu", 4));
        LOG_SYSERR_AND_CLEANUP(close(subtree_fd), -1);
    }

    int path_len = snprintf(NULL, 0, "/sys/fs/cgroup%s/container_%d", self_cgroup_value, container_pid);
    LOG_SYSERR_AND_CLEANUP(*cgroup_path = malloc(path_len + 1), NULL);

    snprintf(*cgroup_path, path_len + 1, "/sys/fs/cgroup%s/container_%d", self_cgroup_value, container_pid);
    free(self_cgroup_value);

    return 0;

cleanup:
    if (self_cgroup_value != NULL) free(self_cgroup_value);
    return -1;
}

static int create_cgroup(pid_t container_pid, char** cgroup_path) {
    IF_CLEANUP(get_cgroup_path(container_pid, cgroup_path), -1);
    
    int mkdir_status;
    if ((mkdir_status = mkdir(*cgroup_path, 0755)) != 0) {
        if (errno == EACCES || errno == ENOENT || errno == EROFS) {
            LOG_WARN("Cgroups not delegated. Running container without resource limits.");
            return 1; 
        }
        LOG_SYSERR_AND_CLEANUP(mkdir_status, -1);
    }
    return 0;

cleanup:
    return -1;
}

static int set_setting_in_cgroup(char* setting_path, char* setting_value) {
    int setting_fd = open(setting_path, O_WRONLY);
    if (setting_fd < 0) {
        if (errno == ENOENT) return 0;
        perror("open");
        goto cleanup;
    }

    if (write(setting_fd, setting_value, strlen(setting_value)) < 0) {
        if (errno == EOPNOTSUPP) {
            close(setting_fd);
            return 0;
        }
        perror("write");
        goto cleanup;
    }

    LOG_SYSERR_AND_CLEANUP(close(setting_fd), -1);
    return 0;

cleanup:
    if (setting_fd >= 0) close(setting_fd);
    return -1;
}

static int set_limits_in_cgroup(char* cgroup_path, struct run_config* config) {
    char setting_path[512];
    
    if (config->memory_limit != NULL) {
        snprintf(setting_path, sizeof(setting_path), "%s/memory.max", cgroup_path);
        IF_CLEANUP(set_setting_in_cgroup(setting_path, config->memory_limit), -1);
    }

    if (config->pids != NULL) {
        snprintf(setting_path, sizeof(setting_path), "%s/pids.max", cgroup_path);
        IF_CLEANUP(set_setting_in_cgroup(setting_path, config->pids), -1);
    }

    if (config->cpu != NULL) {
        snprintf(setting_path, sizeof(setting_path), "%s/cpu.max", cgroup_path);
        IF_CLEANUP(set_setting_in_cgroup(setting_path, config->cpu), -1);
    }

    return 0;

cleanup:
    return -1;
}

static int set_process_in_cgroup(pid_t process_pid, char* cgroup_path) {
    char cgroup_procs[200];
    char setting_path[512];

    snprintf(setting_path, sizeof(setting_path), "%s/cgroup.procs", cgroup_path);
    snprintf(cgroup_procs, sizeof(cgroup_procs), "%d\n", process_pid);
    IF_CLEANUP(set_setting_in_cgroup(setting_path, cgroup_procs), -1);

    return 0;

cleanup:
    return -1;
}

int process_cgroup_creation_and_setting_limits(pid_t container_pid, struct run_config* config) {
    char* cgroup_path = NULL;

    int cg_status;
    LOG_SYSERR_AND_CLEANUP(cg_status = create_cgroup(container_pid, &cgroup_path), -1);
    if (cg_status == 1) {
        if (cgroup_path != NULL) free(cgroup_path);
        return 0; 
    }

    IF_CLEANUP(set_limits_in_cgroup(cgroup_path, config), -1);
    IF_CLEANUP(set_process_in_cgroup(container_pid, cgroup_path), -1);

    free(cgroup_path);
    return 0;

cleanup:
    if (cgroup_path != NULL) free(cgroup_path);
    return -1;
}
