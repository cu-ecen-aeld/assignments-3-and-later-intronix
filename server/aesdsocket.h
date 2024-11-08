#ifndef AESDSOCKET_H__
#define AESDSOCKET_H__

#include <sys/queue.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "hashmap.h"
#include "thread_pool.h"

typedef struct thread_info
{
    pthread_t thread_id;
    int client_sockfd;
    SLIST_ENTRY(thread_info)
    entries;
} thread_info_t;


typedef struct client_data
{
    int client_sockfd;
    char* buffer;
    ssize_t count;
} client_data_t;


int setup_signal_handlers();
int setup_syslog();
int setup_socket();

void log_error(const char *format, ...);
void log_signal(int signal);
void log_client_connection(struct sockaddr_in* client_addr);

int create_timer();
int create_file();
int create_detached_thread();

void handle_signal(int signal);
void handle_timer(union sigval sv);
void* handle_client(void *arg);
int handle_client_non_blocking(client_data_t* client_sockfd);

int listen_socket();

int send_data_to_client(int client_sockfd);
int send_data_to_client_non_blocking(int client_sockfd);

void write_data_to_file(const char* buffer);

int run_daemon();

void cleanup_resources();

#endif
