#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "cli_parser.h"
#include "exec.h"
#include "run.h"

int main(int argc, char* argv[]) {
    void* config = NULL;
    
    if (parse_cli(argc, argv, &config) != 0) {
        return 1;
    }

    char* main_command = argv[1];
    
    if (strcmp(main_command, "run") == 0) {
        handle_run(config);
    } else if (strcmp(main_command, "exec") == 0) {
        handle_exec(config);
    } else {
        printf("ERROR: unknown command\n");
        free(config);
        return 1;
    }

    free(config);
    return 0;
}
