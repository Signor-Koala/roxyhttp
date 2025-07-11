#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
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

#include "default_handlers.h"
#include "main.h"
#include "translate.h"
#include "user_handlers.h"

#define BACKLOG 10
char conf_port[6] = "8080";
size_t conf_buffer_size = 8196;
size_t conf_max_filepath = 256;
size_t conf_max_response_size = 16777216;
char *conf_file_path;
struct timeval clienttimeout = {5, 0};

lua_State *L;

void sigchild_handler(int s) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

void intHandler(int dummy) {
    fprintf(stderr, "Shutting down...\n");
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
    printf("Starting up...\n");

    int server_sock, client_sock, rv;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    conf_file_path = (char *)malloc(sizeof(char) * 3);
    conf_file_path = "./";

    compile_regexes();

    L = luaL_newstate();
    luaL_openlibs(L);

    conf_file_path = get_config(L);

    init_handlers(L);

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

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, conf_port, &hints, &servinfo))) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((server_sock =
                 socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket: ");
            continue;
        }
        if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
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

    printf("server: waiting for connections...\n");

    char buffer[conf_buffer_size];
    while (1) {
        sin_size = sizeof(their_addr);
        client_sock =
            accept(server_sock, (struct sockaddr *)&their_addr, &sin_size);
        if (client_sock == -1) {
            perror("accept: ");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s));
        printf("server: got connection from %s\n", s);

        memset(buffer, 0, conf_buffer_size);
        pid_t childPID = fork();

        if (childPID == 0) {
            if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1) {
                perror("Setting up child genocide failed: ");
                exit(1);
            }
            if (getppid() != parentPID)
                exit(1);

            setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &clienttimeout,
                       sizeof(clienttimeout));

            size_t body_size = 0, line_num, header_size = 0, req_size = 0;
            hheader req_hheader;
            char *end_of_header = NULL;
            char **lines;
            enum ERROR_STATUS error_status = INVALID_REQUEST;

            // TODO: Extract this into a seperate function
            /**
             * Receive the entirity of the request and turn it into a usable
             * format. Most of the request is encoded into a lua table with the
             * exception of the first line. Any errors reading the request are
             * also detected and noted in the `req_status` enum
             */
            while (1) {
                ssize_t recv_size = recv(client_sock, &buffer[req_size],
                                         conf_buffer_size - req_size, 0);

                if (recv_size == -1) {
                    perror("recv: ");
                    close(client_sock);
                    exit(1);
                }
                req_size += recv_size;
                if (req_size > conf_buffer_size) {
                    error_status = REQUEST_TOO_LONG;
                    break;
                }
                if (end_of_header == NULL)
                    end_of_header = strstr(buffer, "\r\n\r\n");
                if (end_of_header == NULL) {
                    // Header not fully recieved, resuming
                    continue;
                }
                if (header_size == 0) {
                    end_of_header = &end_of_header[4];
                    header_size = end_of_header - buffer;
                    lines = split_request(buffer, &line_num);
                    req_hheader = split_hheader(lines[0]);
                    if (strcmp(req_hheader.protocol, "HTTP/1.1")) {
                        error_status = HTTP_PROTO_NOT_IMP;
                        break;
                    }
                    int status = build_request(L, req_hheader, lines, line_num,
                                               &body_size);
                    if (status) {
                        error_status = INVALID_REQUEST;
                        break;
                    }
                    if (body_size == 0) {
                        lua_pushnil(L);
                        lua_setfield(L, -2, "body");
                        error_status = OK;
                        break;
                    }
                }
                if (req_size - header_size == body_size) {
                    lua_pushlstring(L, lines[line_num - 1], body_size);
                    lua_setfield(L, -2, "body");
                    error_status = OK;
                    break;
                } else if (req_size - header_size > body_size) {
                    fprintf(stderr, "Garbage data read\n");
                    error_status = INVALID_REQUEST;
                    break;
                }
            }
            if (shutdown(client_sock, SHUT_RD)) {
                perror("shutdown: ");
                close(client_sock);
                exit(1);
            }
            lua_pushstring(L, s);
            lua_setfield(L, -2, "their_addr");

            /**
             * Construction of response based on the request
             */
            size_t response_size; // +ve = Response size (No Errors);
                                  // 0 = User Handler not found;
                                  // -ve = ERROR_STATUS enum;
            char *response = NULL;

            if (error_status == OK) {

                error_status = exec_middleware(L, req_hheader);
                if (error_status != OK) {
                    break;
                }

                response_size = get_handler_response(L, req_hheader, &response);

                if (response_size == 0) {
                    if (!strcmp(req_hheader.method, "GET")) {
                        response_size = handle_get(req_hheader, &response);
                    } else {
                        error_status = METHOD_NOT_ALLOWED;
                    }
                }
                if ((long)response_size < 0) {
                    error_status = (long)response_size;
                }
            }

            if (error_status != OK) {
                response_size = handle_error(L, error_status, &response);
                if ((long)response_size <= 0) {
                    fprintf(stderr, "Error handling failed");
                    exit(0);
                }
            }

            /**
             * Sending of the response constructed
             */
            int num = response_size / conf_buffer_size;
            for (int i = 0; i < num; i++) {
                if (send(client_sock, &response[i * conf_buffer_size],
                         conf_buffer_size, 0) == -1)
                    perror("send: ");
            }
            if (send(client_sock, &response[num * conf_buffer_size],
                     response_size % conf_buffer_size, 0) == -1)
                perror("send: ");

            close(client_sock);
            exit(0);
        }
        close(client_sock);
    }
    return 0;
}
