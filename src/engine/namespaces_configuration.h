#pragma once

#include <sys/types.h>

#define CHILD_STACK_SIZE (8 * 1024 * 1024)
#define CLONE_CONTAINER_FLAGS CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWUSER | CLONE_NEWPID | CLONE_NEWNET | SIGCHLD

int process_container_creation_and_namespaces_isolation(struct run_config* config, int syncpipefd[2], pid_t* container_pid);
