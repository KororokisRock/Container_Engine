#pragma once

struct run_config {
    char* command;
    char* rootfs_path;
    char* hostname;
    char* memory_limit;
    char* pids;
    char* cpu;
    char** target_cmd;
};

struct exec_config {
    char* command;
    char* pid_to_exec;
    char** target_cmd;
};


int parse_cli(int argc, char* argv[], void** config_out);
