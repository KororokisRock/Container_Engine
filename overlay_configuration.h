#pragma once

#include <sys/types.h>

int process_overlayfs_creation_and_hostname_set(pid_t container_pid, struct run_config* config);
