#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "logger.h"
#include "util.h"
#include "namespaces_configuration.h"
#include "child.h"

static int sync_pipe_create(int pipefd[2]) {
    LOG_SYSERR_AND_CLEANUP(pipe(pipefd), -1);
    return 0;

cleanup:
    return -1;
}

static int create_process_and_isolate_namespaces(int pipereadfd, struct run_config* config, pid_t* container_pid) {
    void* child_stack = NULL;
    struct container_init_data* container_init_data = NULL;

    LOG_SYSERR_AND_CLEANUP(child_stack = mmap(NULL, CHILD_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0), MAP_FAILED);

    LOG_SYSERR_AND_CLEANUP(container_init_data = calloc(1, sizeof(struct container_init_data)), NULL);
    container_init_data->readpipefd = pipereadfd;
    container_init_data->config = config;

    LOG_SYSERR_AND_CLEANUP(*container_pid = clone(child_process, (char*)child_stack + CHILD_STACK_SIZE, CLONE_CONTAINER_FLAGS, container_init_data), -1);

    free(container_init_data);
    return 0;

cleanup:
    if (child_stack != MAP_FAILED && child_stack != NULL) munmap(child_stack, CHILD_STACK_SIZE);
    if (container_init_data != NULL) free(container_init_data);
    return -1;
}

int process_map_setting_in_file(char* path_to_file, char* setting_value) {
    int proc_fd = -1;
    LOG_SYSERR_AND_CLEANUP(proc_fd = open(path_to_file, O_WRONLY), -1);
    LOG_SYSERR_AND_CLEANUP(write(proc_fd, setting_value, strlen(setting_value)), -1);
    close(proc_fd);
    
    return 0;

cleanup:
    if (proc_fd >= 0) close(proc_fd);
    return -1;
}

int process_mapping_uid_gid(pid_t pid) {
    uid_t uid = getuid();
    gid_t gid = getgid();

    char path_to_setgroups_proc[50];
    snprintf(path_to_setgroups_proc, sizeof(path_to_setgroups_proc), "/proc/%d/setgroups", pid);

    char path_to_uidmap_proc[50];
    snprintf(path_to_uidmap_proc, sizeof(path_to_uidmap_proc), "/proc/%d/uid_map", pid);

    char uidmap_value[50];
    snprintf(uidmap_value, sizeof(uidmap_value), "0 %d 1\n", uid);

    char path_to_gidmap_proc[50];
    snprintf(path_to_gidmap_proc, sizeof(path_to_gidmap_proc), "/proc/%d/gid_map", pid);

    char gidmap_value[50];
    snprintf(gidmap_value, sizeof(gidmap_value), "0 %d 1\n", gid);

    IF_CLEANUP(process_map_setting_in_file(path_to_setgroups_proc, "deny\n"), -1);
    IF_CLEANUP(process_map_setting_in_file(path_to_uidmap_proc, uidmap_value), -1);
    IF_CLEANUP(process_map_setting_in_file(path_to_gidmap_proc, gidmap_value), -1);

    return 0;

cleanup:
    return -1;
}

int process_container_creation_and_namespaces_isolation(struct run_config* config, int syncpipefd[2], pid_t* container_pid) {
    IF_CLEANUP(sync_pipe_create(syncpipefd), -1);

    IF_CLEANUP(create_process_and_isolate_namespaces(syncpipefd[0], config, container_pid), -1);

    close(syncpipefd[0]);
    syncpipefd[0] = -1;

    IF_CLEANUP(process_mapping_uid_gid(*container_pid), -1);

    return 0;

cleanup:
    return -1;
}
