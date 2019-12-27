#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include "connection.h"
#include "files.h"
#include "beandef.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#define CAFIINE_PORT 7332
#define BACKLOG 10

static int socket_fd;
bool running = true;

// functions to handle the server socket
static int listener_init();
static void listener_run();
static void listener_shutdown();

#ifdef _WIN32
static char *realpath(const char *path, char resolved_path[PATH_MAX]);
#endif

// options
#define IS_OPT(n) (strcmp(long_options[option_index].name, n) == 0)

static const struct option long_options[] = {
    { "root", required_argument, NULL, 0 },
    { NULL, no_argument, NULL, 0 }
};

int main(int argc, char *argv[]) {
    // Hack required for CLion to properly handle stdout when in the debugger
    setbuf(stdout, 0);
    setbuf(stderr, 0);

    char *rootdir = NULL;

    char defaultrootdir[512];
    snprintf(defaultrootdir, 512, "%s/root/", realpath(".", NULL));
    rootdir = defaultrootdir;

    int c, option_index;
    while ((c = getopt_long(argc, argv, "", (const struct option *)&long_options, &option_index)) != -1) {
        switch (c) {
            case 0: {
                if (IS_OPT("root")) {
                    rootdir = realpath(optarg, NULL);
                    if (rootdir == NULL) {
                        printf("Couldn't access root directory %s - does it exist?\n", optarg);
                        return 1;
                    }
                }
            }
        }
    }

    fs_init(rootdir);

    listener_init();
    listener_run();
    listener_shutdown();

    fs_shutdown();

    return 0;
}

int listener_init() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to start up WinSock.\n");
        return WSAGetLastError();
    }
#endif

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        socket_perror("creating socket");
        return errno;
    }

    {
        int yes = 1;
        int err = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        if (err == -1) {
            socket_perror("setsockopt(SO_REUSEADDR)");
            return errno;
        }
    }

    struct sockaddr_in server_address = { 0 };
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons((unsigned short)CAFIINE_PORT);

    int err = bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));
    if (err == -1) {
        socket_perror("bind");
        return errno;
    }

    err = listen(socket_fd, BACKLOG);
    if (err == -1) {
        perror("listen");
        return errno;
    }

    return 0;
}

void listener_run() {
    // wait for clients
    printf("Now listening and waiting for clients.\n");
    while (running) {
        struct sockaddr_storage client_address;
        socklen_t address_size = sizeof(client_address);
        int fd = accept(socket_fd, (struct sockaddr *)&client_address, &address_size);
        if (fd == -1) {
            socket_perror("accept");
            return;
        }

        int yes = 1;
        //setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

        struct sockaddr_in *sin = (struct sockaddr_in*)&client_address;

        unsigned char *addr = (unsigned char *)&sin->sin_addr.s_addr;
        printf("%d: Incoming connection from %d.%d.%d.%d:%d\n", fd, addr[0], addr[1], addr[2], addr[3], sin->sin_port);

        connection_handle(fd);
    }
}

void listener_shutdown() {
    close(socket_fd);

#ifdef _WIN32
    WSACleanup();
#endif
}

void socket_perror(const char *msg) {
#ifdef _WIN32
    fprintf(stderr, "%s: %d\n", msg, WSAGetLastError());
#else
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
#endif
}

#ifdef _WIN32
char *realpath(const char *path, char resolved_path[PATH_MAX]) {
    char *out = resolved_path;

    if (!out) {
        out = malloc(PATH_MAX + 1);
        memset(out, 0, PATH_MAX + 1);
    }

    GetFullPathNameA(path, PATH_MAX, out, NULL);

    return out;
}
#endif