/**
 * @file event_io.h
 * @brief event_io.cのヘッダ
 * @author E.Okazaki
 */
#pragma once
#include <time.h>
//! 一度のepoll_wait()で得られる最大イベント数
#define MAX_EPOLL 80


/**
 * @enum observe_type
 * 監視するイベントの種類が定義された列挙体
 */
typedef enum {
    //! EPOLLINに相当
    OBS_IN = 1u,
    //! EPOLLOUTに相当
    OBS_OUT = 4u,
    //! EPOLLONESHOTに相当
    OBS_ONESHOT = 1u << 30,
} observe_type;

/**
 * epollのstruct epoll_eventのユーザ定義のフィールドで使いたい構造体
 */
struct io_event {
    //! 監視対象のファイルディスクリプタ
    int fd;
    //! そのイベントがIO可能になった時に呼び出される関数(ポインタ)
    void (*handler)(struct io_event*);
    //! 呼び出される関数内で使いたい変数など(任意)
    void* arg;
    //! 前のio_event(連結リスト)
    struct io_event* prev;
    //! 後のio_event(連結リスト)
    struct io_event* next;
    //! 最後に発生したイベント日時
    struct tm* timestamp;
    //! 発生したイベントの種類
    observe_type type;
};

/**
 * event_io全体で使われる変数一式が定義された構造体
 */
struct event_config {
    //! epollのファイルディスクリプタ
    int epfd;
    //! struct io_eventの先頭ポインタ
    struct io_event* head;
    //! struct io_eventの末尾ポインタ
    struct io_event* tail;
};

struct event_config* init_event();

struct io_event* add_event(struct event_config* event_config, void (*handler)(struct io_event*), void* arg, int fd, observe_type type);

void delete_event(struct event_config* event_config, struct io_event* io_event);

void erase_events(struct event_config* event_config);

void run_event(struct event_config* event_config, int timeout);
