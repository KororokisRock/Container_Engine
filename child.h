#pragma once

struct container_init_data {
    int readpipefd;
    void* config;
    pid_t container_pid;
};

int child_process(void* arg);
