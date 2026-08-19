CC = gcc

INCLUDES = -Isrc/engine -Isrc/util

SRCS = src/engine/main.c \
       src/engine/cli_parser.c \
       src/engine/run.c \
       src/engine/exec.c \
       src/engine/namespaces_configuration.c \
       src/engine/child.c \
       src/engine/cgroups_configuration.c \
       src/engine/overlay_configuration.c \
       src/engine/container_cleanup.c

OBJS = $(SRCS:.c=.o)

CFLAGS = -Wall -Wextra -Werror $(INCLUDES)

ifeq ($(BUILD_MODE),test)
	CFLAGS += -g -fsanitize=address -fsanitize=undefined
	TARGET = engine_test
else
	CFLAGS += -O2
	TARGET = engine_prod
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) engine_test engine_prod

.PHONY: all clean
