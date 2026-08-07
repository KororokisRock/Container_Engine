#pragma once

struct container_init_data {
    int readpipefd;
    void* config;
};

int child_process(void* arg);
