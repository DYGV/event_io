/**
 * @file 01.c
 * @brief 単純な例
 * @author E.Okazaki
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "../event_io.h"

void tirm_nl(char* str)
{
    char* p;
    p = strchr(str, '\n');
    if (p != NULL) {
        *p = '\0';
    }
}

// コールバック関数とする場合は必ずstruct io_eventを受け取る
void print(struct io_event* io)
{
    char buf[256];
    read(io->fd, buf, sizeof(buf));
    tirm_nl(buf);
    printf("FD: %d\n", io->fd);
    printf("arg: %s\n", (char*)io->arg);
    printf("event type: %d\n", io->type);
    printf("timestamp: %s", asctime(io->timestamp));
    printf("input: %s\n", buf);
    printf("---------------------------------------\n");
}

int main()
{
    struct event_config* config;
    config = init_event();
    add_event(config, print, "標準入力だよ", STDIN_FILENO, OBS_IN);
    while (1) {
        run_event(config, -1);
    }
}
