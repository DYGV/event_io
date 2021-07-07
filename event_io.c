/**
 * @file event_io.c
 * @brief イベント処理まわりのプログラム
 * @author E.Okazaki
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "event_io.h"

/**
 * イベント監視の初期化処理をする関数
 * @return strcut event_config*
 */
struct event_config* init_event()
{
    struct event_config* event_config = malloc(sizeof(struct event_config));
    if (event_config == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }
    // epollのインスタンスを作成
    event_config->epfd = epoll_create(MAX_EPOLL);
    if (event_config->epfd == -1) {
        perror("epoll_create()");
        exit(EXIT_FAILURE);
    }
    event_config->head = NULL;
    event_config->tail = NULL;
    return event_config;
}

/**
 * イベント監視を新たに追加する関数
 * @param struct event_config* event_config
 * @param void (*handler)(struct io_event*) イベント発生時に呼び出すための関数ポインタ
 * @param void* arg handlerを呼び出すときの引数
 * @param int fd 監視対象のファイルディスクリプタ
 * @return struct io_event*
 */
struct io_event* add_event(struct event_config* event_config, void (*handler)(struct io_event*), void* arg, int fd, observe_type type)
{
    if (event_config == NULL) {
        perror("NULL");
        exit(EXIT_FAILURE);
    }
    struct io_event* io = malloc(sizeof(struct io_event));
    if (io == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }
    io->handler = handler;
    io->arg = arg;
    io->fd = fd;
    io->next = NULL;
    io->prev = NULL;
    io->timestamp = NULL;
    // まだ監視対象がないとき(初回)
    if (event_config->head == NULL) {
        event_config->head = io;
        event_config->tail = io;
    } else {
        // 監視対象が少なくとも1つあるときは末尾につなげていく
        event_config->tail->next = io;
        io->prev = event_config->tail;
        event_config->tail = io;
    }
    struct epoll_event event;
    event.events = type;
    event.data.ptr = io;
    // インタレストリスト(監視対象のリスト)に追加する
    // fdが既にインタレストリストに存在する場合はEEXIST
    if (epoll_ctl(event_config->epfd, EPOLL_CTL_ADD, fd, &event) == -1) {
        perror("epoll_ctl()");
        exit(EXIT_FAILURE);
    }
    return io;
}

/**
 * イベント監視を外す関数
 * @param struct event_config* event_config
 * @param struct io_event* io 監視対象を外したいイベント
 * @return void
 */
void delete_event(struct event_config* event_config, struct io_event* io)
{
    if (event_config == NULL) {
        perror("NULL");
        exit(EXIT_FAILURE);
    }
    // 監視対象から外す
    // fdがインタレストリストに存在しない場合は、ENOENT(no entry)
    if (epoll_ctl(event_config->epfd, EPOLL_CTL_DEL, io->fd,  NULL) == -1) {
        perror("epoll_ctl()");
        exit(EXIT_FAILURE);
    }
    if (io->prev != NULL) {
        // io_eventがリストが途中の要素なら
        // 前の要素のnextと次の要素をつなげる
        io->prev->next = io->next;
    } else {
        // io_eventがリストの先頭の要素なら
        // 先頭の位置を進める
        event_config->head = io->next;
    }
    if (io->next != NULL) {
        // io_eventがリストの途中の要素なら
        // 次の要素のprevと前の要素をつなげる
        io->next->prev = io->prev;
    } else {
        // io_eventがリストの末尾の要素なら
        // 末尾の位置を戻す
        event_config->tail = io->prev;
    }
    free(io);
    io = NULL;
}

/**
 * add_eventで追加した全てのイベントを削除する関数
 * @param struct event_config* event_config
 * @return void
 */
void erase_events(struct event_config* event_config)
{
    if (event_config == NULL) {
        perror("NULL");
        exit(EXIT_FAILURE);
    }
    struct io_event* head;
    while ((head = event_config->head) != NULL) {
        // 監視対象から外す
        // fdがインタレストリストに存在しない場合は、ENOENT(no entry)
        if (epoll_ctl(event_config->epfd, EPOLL_CTL_DEL, head->fd, NULL) == -1) {
            perror("epoll_ctl()");
            exit(EXIT_FAILURE);
        }
        free(head);
        head = NULL;
        event_config->head = event_config->head->next;
    }
}

/**
 * イベントを走らせる関数
 * @param struct event_config* event_config
 * @param int timout IO可否に関係なく抜けるタイムアウト値(ミリ秒)
 * @return void
 */
void run_event(struct event_config* event_config, int timeout)
{
    if (event_config == NULL) {
        perror("NULL");
        exit(EXIT_FAILURE);
    }
    struct epoll_event events[MAX_EPOLL];
    // 監視対象がIO可能になるか、timoutまでブロックされる
    int event_readable = epoll_wait(event_config->epfd, events, MAX_EPOLL, timeout);
    if (event_readable == -1) {
        perror("epoll_wait()");
        exit(EXIT_FAILURE);
    }
    // IO可能になった時点のタイムスタンプ
    time_t t = time(NULL);
    if (t == -1) {
        perror("time()");
        exit(EXIT_FAILURE);
    }
    struct tm* time = localtime(&t);
    for (int i = 0; i < event_readable; i++) {
        struct io_event* io = events[i].data.ptr;
        io->timestamp = time;
        io->type = events[i].events;
        io->handler(io);
    }
}

