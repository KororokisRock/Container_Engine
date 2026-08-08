#define _GNU_SOURCE
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "cli_parser.h"

static int parse_run(int argc, char* argv[], void** config_out) {
    struct run_config* config = calloc(1, sizeof(struct run_config));
    *config_out = config;

    config->command = argv[0];

    struct option long_options[] = {
        {"rootfs", required_argument, NULL, 'r'},
        {"hostname", required_argument, NULL, 'h'},
        {"memory", required_argument, NULL, 'm'},
        {"pids", required_argument, NULL, 'p'},
        {"cpu", required_argument, NULL, 'c'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "r:h:m:p:c:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'r':
                config->rootfs_path = optarg;
                break;
            case 'h':
                config->hostname = optarg;
                break;
            case 'm':
                config->memory_limit = optarg;
                break;
            case 'p':
                config->pids = optarg;
                break;
            case 'c':
                config->cpu = optarg;
                break;
            case '?':
                printf("ERROR: option error\n");
                return -1;
            default:
                printf("ERROR: undefined option\n");
                return -1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "ERROR: Expected command to execute after options\n");
        return -1;
    }

    config->target_cmd = &argv[optind];

    return 0;
}


static int parse_exec(int argc, char* argv[], void** config_out) {
    struct exec_config* config = calloc(1, sizeof(struct exec_config));
    *config_out = config;

    config->command = argv[0];

    struct option long_options[] = {
        {"pid", required_argument, NULL, 'p'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "p:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                config->pid_to_exec = optarg;
                break;
            case '?':
                printf("ERROR: option error\n");
                return -1;
            default:
                printf("ERROR: undefined option\n");
                return -1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "ERROR: Expected command to execute after options\n");
        return -1;
    }

    config->target_cmd = &argv[optind];

    return 0;
}


int parse_cli(int argc, char* argv[], void** config_out) {
    if (argc < 2) {
        printf("ERROR: count arguments must be equal or more than 2\n");
        return -1;
    }

    char* main_command = argv[1];

    if (strcmp(main_command, "run") == 0) {
        parse_run(argc - 1, &argv[1], config_out);
    } else if (strcmp(main_command, "exec") == 0) {
        parse_exec(argc - 1, &argv[1], config_out);
    } else {
        printf("ERROR: main command is undefined\n");
        return -1;
    }

    return 0;
}
