#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define TEMP_PATH "/var/tmp/aesdsocketdata"
#define PORT "9000"
#define BUFFERLENGTH 128 

static bool EXITSIGNAL = 0;


static void sigint_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM) {
        syslog(LOG_USER, "Received Signal, exiting\n");
	EXITSIGNAL = true;
    } else {
	exit(EXIT_FAILURE);
    }
}

void get_client_info(struct sockaddr * p_sockaddr, socklen_t size, char* client_address) {

    if (p_sockaddr->sa_family == AF_INET) {
        struct sockaddr_in* p_sockaddr_in = (struct sockaddr_in*) p_sockaddr;
        inet_ntop(AF_INET,(const void*) &p_sockaddr_in->sin_addr, client_address, size); 
    } else {
	struct sockaddr_in6* p_sockaddr_in6;
        p_sockaddr_in6 = (struct sockaddr_in6*) p_sockaddr;
        inet_ntop(AF_INET6, (const void*) &p_sockaddr_in6->sin6_addr, client_address, size);
    }
    return;
}


void send_file_to_client(size_t total_bytes, int client_socket) {
    FILE* file_ptr = fopen(TEMP_PATH, "r");
    if (file_ptr == NULL) {
        perror("Error opening file");
        return;
    }

    char buffer[2];
    size_t bytes_read;
    size_t bytes_sent = 0;

    while (bytes_sent < total_bytes && (bytes_read = fread(buffer, 1, sizeof(buffer), file_ptr)) > 0) {
        size_t chunk_size = (total_bytes - bytes_sent < bytes_read) ? (total_bytes - bytes_sent) : bytes_read;
        ssize_t send_result = send(client_socket, buffer, chunk_size, 0);
        
        if (send_result == -1) {
            perror("Error sending data");
            break;
        }
        
        bytes_sent += send_result;
    }

    fclose(file_ptr);
}

int main(int argc, char** argv) {
  int enable_daemon = 0;
  int ret = 0;
  FILE* fp = fopen(TEMP_PATH, "w");
  struct addrinfo* addrinfo_ptr = NULL;
  struct addrinfo hints;  
  int byte_written = 0;
  int fd_accept = 0;
  struct sockaddr_storage client_addr;
  socklen_t client_num = sizeof(struct sockaddr);  

  if (signal(SIGTERM, sigint_handler) == SIG_ERR) {
    printf("SIGTERM register failed!\n");
    exit(EXIT_FAILURE);
  }

  if (signal(SIGINT, sigint_handler) == SIG_ERR) {
    printf("SIGINT register failed!\n");
    exit(EXIT_FAILURE);
  }

  int c;
  while ((c = getopt(argc, argv, "d")) != -1) {
    switch (c) {
      case 'd':
        enable_daemon = 1;
        break;
      default:
        printf("Please check option\n");
        exit(1);
    }
  }


  if (fp == NULL) {
    printf("fp is null, exit\n");
    return (1);
  }


  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  ret = getaddrinfo(NULL, PORT, &hints, &addrinfo_ptr);

  if (ret != 0) {
    printf("getaddrinfo() failed!\n");
    return (ret);
  }

  if (enable_daemon) {
    int pid;
    pid = fork();

    if (pid == -1) {
      printf("Failed to fork, exit!\n");
      syslog(LOG_PERROR, "Failed to fork, exit\n");
      exit(1);
    } else if (pid == 0) {
      printf("This is child process, continue..\n");
      if (setsid() == -1) {
      }
    } else {
      exit(0);
    }
  }

  int socket_fd = socket(addrinfo_ptr->ai_family, addrinfo_ptr->ai_socktype,
                         addrinfo_ptr->ai_protocol);

  if (socket_fd == -1) {
    printf("socket() failed! errno: %d\n", errno);
    return (socket_fd);
  }

  int yes = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) ==
      -1) {
    perror("setsockopt error");
    exit(1);
  }

  ret = bind(socket_fd, addrinfo_ptr->ai_addr, addrinfo_ptr->ai_addrlen);

  if (ret == -1) {
    printf("bind() failed!\n");
    return (ret);
  }

  freeaddrinfo(addrinfo_ptr);

  ret = listen(socket_fd, 10);

  if (ret == -1) {
    printf("listen() failed!\n");
    return (ret);
  }



  while (EXITSIGNAL == false) {
    fd_accept = accept(socket_fd, (struct sockaddr*)&client_addr, &client_num);

    if (ret == -1) {
      printf("accept() failed!\n");
      return (ret);
    }

    char client_address[INET_ADDRSTRLEN];
    get_client_info((struct sockaddr*)&client_addr, client_num, client_address);
    syslog(LOG_USER, "Accepted connection from %s\n", client_address);
    printf("Accepted connection from %s\n", client_address);

    char buf[BUFFERLENGTH];
    buf[BUFFERLENGTH] = '\0';

    while (EXITSIGNAL == false) {
      ret = recv(fd_accept, buf, BUFFERLENGTH - 1, 0);

      if (ret == -1) {
        printf("recv() failed!\n");
        return (ret);
      }

      fwrite(buf, ret, 1, fp);

      byte_written += ret;
      if (strchr(buf, '\n') != NULL) {
        fflush(fp);
        send_file_to_client(byte_written, fd_accept);
        break;
      }
    }
  }

  fclose(fp);
  shutdown(fd_accept, 2);
  shutdown(socket_fd, 2);
  close(socket_fd);
  close(fd_accept);

  if (remove(TEMP_PATH) == 0) {
    printf("Removed successfully\n");
  }

  printf("Exit...\n");
  return 0;
}