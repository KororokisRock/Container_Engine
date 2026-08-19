#pragma once

int process_cgroup_creation_and_setting_limits(pid_t container_pid, struct run_config* config);
int get_cgroup_path(pid_t container_pid, char** cgroup_path);
