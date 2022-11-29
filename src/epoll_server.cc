#include<cstdio>
#include<unistd.h>
#include<cstdlib>
#include<cstring>
#include<sys/epoll.h>
#include <sys/stat.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include <thread>
#include <iostream>
#include <mutex>
#include <atomic>
#include <condition_variable>

#define MAXSIZE 1000
#define THREAD_NUM 16
using namespace std;
int ep_fd_list[THREAD_NUM] = {0};
std::mutex mtx;
std::condition_variable cv;
bool init_server_done = false;
atomic_uint64_t has_count = 0;
atomic_uint64_t done_count = 0;
auto start = chrono::steady_clock::now();
auto stop = chrono::steady_clock::now();

//发送消息报头
void send_response_head(int cfd) {
    // const char *buf = "HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nContent-Length:4\r\n\r\ntrue";
    const char *buf = "HTTP/1.1 200 OK\r\nconnect: keep-alive\r\nContent-Type:application/json\r\nContent-Length:4\r\n\r\ntrue";
    send(cfd, buf, strlen(buf), 0);
}

void http_request(char *line, int len) {
    for (int i = 5; i < len; ++i) {
        if (line[i] == ' ' || line[i] == '\r' || line[i] == '\n') {
            line[i] = '\0';
            break;
        }
    }
    if (memcmp("/collect_energy/", line + 5, 16) != 0) {
        return;
    }
    line += 5;//去掉POST
    char user_id[64] = {0};
    int user_id_len = 0;
    for (; user_id_len < len; ++user_id_len) {
        if (line[user_id_len + 16] == '/') {
            break;
        }
        user_id[user_id_len] = line[user_id_len + 16];
    }
    int to_collect_energy_id = (int) strtol(line + user_id_len + 17, nullptr, 10);
}


void func_http(int thread_id) {
    int ep_fd = epoll_create(MAXSIZE);
    ep_fd_list[thread_id] = ep_fd;
    struct epoll_event all[MAXSIZE];
    while (true) {
        int ret = epoll_wait(ep_fd, all, MAXSIZE, -1);
        if (ret == -1) {
            perror("epllo_wait error");
            continue;
        }
        //遍历发生变化的节点
        for (int i = 0; i < ret; i++) {
            //只处理读事件，其他默认不处理
            struct epoll_event *pev = &all[i];
            uint32_t events = all[i].events;
            if (events & EPOLLERR || events & EPOLLHUP || !(events & EPOLLIN)) {
                printf("Epoll has error\n");
                close(all[i].data.fd);
                continue;
            }
            if (!(pev->events & EPOLLIN))   //不是读事件
            {
                continue;
            }
            char line[1024] = {0};
            ssize_t len = recv(all[i].data.fd, line, sizeof(line), 0);
            if (len <= 0) {
                break;
            }
            send_response_head(all[i].data.fd);
            http_request(line, (int) len);
            uint64_t cur_done_count = done_count.fetch_add(1);
            if ((cur_done_count + 1) % 10000 == 0) {
                stop = chrono::steady_clock::now();
                chrono::duration<double> diff = stop - start;
                cout << "cur_done_count:" << cur_done_count + 1 << " " << diff.count() << endl;
            }
        }
    }
}

void func() {
    for (int i = 0; i < THREAD_NUM; ++i) {
        (new thread(func_http, i))->detach();
    }
    int ep_fd = epoll_create(1);
    if (ep_fd == 1) {
        perror("epllo_create error");
    }
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket error");
    }
    struct sockaddr_in serv{};
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(8080);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    //端口复用
    int flag = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    int ret = bind(lfd, (struct sockaddr *) &serv, sizeof(serv));
    if (ret == -1) {
        perror("bind error");
    }
    //设置监听
    ret = listen(lfd, 200);
    if (ret == -1) {
        perror("listen error");
    }
    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    epoll_ctl(ep_fd, EPOLL_CTL_ADD, lfd, &ev);
    struct epoll_event all[MAXSIZE];
    init_server_done = true;
    cv.notify_all();
    struct sockaddr_in client{};
    socklen_t len = sizeof(client);
    int ep_fd_idx = 0;
    while (done_count < 100 * 10000) {
        ret = epoll_wait(ep_fd, all, MAXSIZE, -1);
        if (ret == -1) {
            perror("epllo_wait error");
            continue;
        }
        //遍历发生变化的节点
        for (int i = 0; i < ret; i++) {
            //只处理读事件，其他默认不处理
            struct epoll_event *pev = &all[i];
            if (!(pev->events & EPOLLIN))   //不是读事件
            {
                continue;
            }
            if (pev->data.fd == lfd) {
                uint64_t cur_has_count = has_count.fetch_add(1);
                if (cur_has_count == 0) {
                    start = chrono::steady_clock::now();
                }
                cout << "cur_has_count:" << cur_has_count + 1 << endl;
                int cfd = accept(lfd, (struct sockaddr *) &client, &len);
                flag = fcntl(cfd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(cfd, F_SETFL, flag);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = cfd;
                epoll_ctl(ep_fd_list[ep_fd_idx], EPOLL_CTL_ADD, cfd, &ev);
                ep_fd_idx = (ep_fd_idx + 1) % THREAD_NUM;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    thread thread_main(func);
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [] {
        return init_server_done;
    });
    cout << "init done" << endl;
    thread_main.join();
    return 0;
}