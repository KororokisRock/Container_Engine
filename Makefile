CC = gcc
SRCS = engine.c cli_parser.c run.c exec.c namespaces_configuration.c
OBJS = $(SRCS:.c=.o)

CFLAGS = -Wall -Wextra -Werror

ifeq ($(BUILD_MODE),test)
	CFLAGS += -g -fsanitize=address -fsanitize=undefined
	TARGET = test
else
	CFLAGS += -O2
	TARGET = prod
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) test prod

.PHONY: all clean
