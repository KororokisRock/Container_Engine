#pragma once

#include <sys/types.h>

#define CHILD_STACK_SIZE 1024 * 8
#define CLONE_CONTAINER_FLAGS CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWUSER | CLONE_NEWPID | CLONE_NEWNET | SIGCHLD

struct run_config;

int process_container_creation_and_namespaces_isolation(struct run_config* config, int syncpipefd[2], pid_t* container_pid);
