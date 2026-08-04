#include <stdio.h>

#include "cli_parser.h"


int main(int argc, char* argv[]) {
    void* config;
    parse_cli(argc, argv, &config);

    
}
