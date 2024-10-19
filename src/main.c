#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cache.h"
#include "handlers.h"
#include "main.h"
#include "translate.h"

#define PORT "8080"

#define BACKLOG 10

lru_table *cache;
size_t cache_size =
    sizeof(lru_table) + MAX_CACHE_ENTRIES * (MAX_ENTRY_SIZE + MAX_FILEPATH);

void sigchild_handler(int s) {
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

void intHandler(int dummy) {
  fprintf(stderr, "Shutting down...\n");
  if (pthread_mutex_lock(&(cache->lock))) {
    perror("Failed to lock: ");
    exit(1);
  }
  pthread_mutex_unlock(&(cache->lock));
  if (munmap(cache, cache_size)) {
    perror("Unlink: ");
    exit(1);
  }
  shm_unlink("roxyhttp_cache");
  fprintf(stderr, "Goodbye\n");
  exit(0);
}

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
  int server_sock, client_sock, rv;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  compile_regexes();

  // START

  cache = create_cache();
  pthread_mutexattr_t attrmutex;
  pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&(cache->lock), &attrmutex);

  // END

  if ((rv = getaddrinfo(NULL, "8080", &hints, &servinfo))) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
    exit(1);
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((server_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      perror("server: socket: ");
      continue;
    }
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
      perror("setsockopt: ");
      exit(1);
    }
    if (bind(server_sock, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_sock);
      perror("server: bind: ");
      continue;
    }
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(server_sock, BACKLOG) == -1) {
    perror("listen: ");
    exit(1);
  }

  freeaddrinfo(servinfo);

  sa.sa_handler = sigchild_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction: ");
    exit(1);
  }
  signal(SIGINT, intHandler);
  signal(SIGTERM, intHandler);

  pid_t parentPID = getpid();

  printf("server: waiting for connections...\n");

  char buffer[BUFFER_SIZE];
  while (1) {
    sin_size = sizeof(their_addr);
    client_sock =
        accept(server_sock, (struct sockaddr *)&their_addr, &sin_size);
    if (client_sock == -1) {
      perror("accept: ");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof(s));
    printf("server: got connection from %s\n", s);

    memset(buffer, 0, BUFFER_SIZE);
    pid_t childPID = fork();

    if (childPID == 0) {
      if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1) {
        perror("Setting up child genocide failed: ");
        exit(1);
      }
      if (getppid() != parentPID)
        exit(1);

      if (recv(client_sock, buffer, BUFFER_SIZE, 0) == -1) {
        perror("recv: ");
        close(client_sock);
        exit(0);
      }

      req request = request_decode(buffer);

      size_t response_size;
      char *response = NULL;

      if (0) {
        // TODO: Handle overrides here
      } else if (request.method == GET) {
        response_size = handle_get(request, &response, cache);
      }

      int num = response_size / BUFFER_SIZE;
      for (int i = 0; i < num; i++) {
        if (send(client_sock, &response[i * BUFFER_SIZE], BUFFER_SIZE, 0) == -1)
          perror("send: ");
      }
      if (send(client_sock, &response[num * BUFFER_SIZE],
               response_size % BUFFER_SIZE, 0) == -1)
        perror("send: ");

      close(client_sock);
      exit(0);
    }
    close(client_sock);
  }
  return 0;
}
