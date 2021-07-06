/**
 * @file 02.c
 * @brief サーバプログラムの例
 * @author E.Okazaki
 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../event_io.h"

/// 送受信できるバイト数
#define BUF_LEN 1024
/// サーバのポート番号
#define PORT 8080

/**
 * サーバに関する情報を持つ構造体
 */
struct server {
    //! サーバのアドレス情報
    struct sockaddr_in address;
    //! サーバのファイルディスクリプタ
    int fd;
    //! 送受信に使う変数
    char buf[BUF_LEN];
    //! イベントに関する情報を持つ構造体
    struct event_config* event;
};

void init_server(struct server* server);
void write_(struct io_event* head, char* msg);
void read_(struct io_event* io);
void accept_(struct io_event* io);

/**
 * サーバを立ち上げる関数
 * 立ち上げが完了したらそのファイルディスクリプタをイベント監視に追加する
 * @param struct server* server
 * @return void
 */
void init_server(struct server* server)
{
    int opt = 1;
    // ソケット(エンドポイント)を作成する
    if ((server->fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    // オプションを付加する
    if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }
    // アドレスファミリの設定
    server->address.sin_family = AF_INET;
    // IPv4のワイルドカード
    server->address.sin_addr.s_addr = INADDR_ANY;
    // ポート設定
    server->address.sin_port = htons(PORT);
    // ソケットへアドレスを付加する
    if (bind(server->fd, (struct sockaddr*)&server->address, sizeof(server->address)) < 0) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    // 待ち行列を5個にしてserver_fdをパッシブ(リッスン)ソケットとしてマークする
    if (listen(server->fd, 5) < 0) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }
    // サーバのファイルディスクリプタを
    // イベントの監視対象として追加する。
    // IO可能になった(新規接続が来た)時にaccept_()を実行するよう登録する。
    add_event(server->event, accept_, server, server->fd, OBS_IN);
}

/**
 * クライアントへ文字列を送信する関数
 * @param struct io_event* head クライアントのイベント情報の先頭ポインタ
 * @param char* msg 送信したい文字列
 * @return void
 * @details headがNULLになるまで再帰的にポインタを進め、クライアントへ書き込む。
 */
void write_(struct io_event* head, char* msg)
{

    if (head == NULL) {
        return;
    }
    write(head->fd, msg, strlen(msg));
    write_(head->next, msg);
}

/**
 * クライアントのイベント発生時の処理をする関数
 * @param struct io_event* io イベントが発生したクライアントのイベント情報
 * @return void
 */
void read_(struct io_event* io)
{
    struct server* server = (struct server*)io->arg;

    // 読み取り時に何らかのエラーが発生したら切断処理をする。
    if (read(io->fd, server->buf, sizeof(server->buf)) <= 0) {
        // イベントの監視を外す(delete_eventでメモリの解放も行っている)
        delete_event(server->event, io);
        return;
    }
    // 接続済みの全クライアントに文字列を送信する
    write_(server->event->head->next, server->buf);
    printf("%s\n", server->buf);
    // 0埋めしておく
    memset(server->buf, 0, sizeof(server->buf));
}

/**
 * 接続要求を受け入れる関数
 * @param struct io_event* io サーバのファイルディスクリプタのイベント情報
 * @return void
 * @details クライアントからの接続要求を受け入れ、そのクライアントのファイルディスクリプタを監視対象に追加する。
 * IO可能になった(メッセージが来た)ときにread_()を実行するように登録する。
 */
void accept_(struct io_event* io)
{
    int new_socket;
    struct server* server = (struct server*)io->arg;
    struct sockaddr_in address =  server->address;
    int addrlen = sizeof(address);
    if ((new_socket = accept(io->fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) == -1) {
        perror("accept()");
        return;
    }
    add_event(server->event, read_, server, new_socket, OBS_IN);
}

int main()
{
    // サーバに関する情報を保持する構造体
    struct server server;
    // 監視するイベント処理をするための初期化処理
    server.event = init_event();
    // サーバを立ち上げる
    init_server(&server);
    printf("サーバを立ち上げました。\n");
    while (1) {
        // イベント監視処理を開始する
        run_event(server.event, -1);
    }
}
