#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include "connection.h"
#include "files.h"

#define CAFIINE_PORT 7332
#define BACKLOG 10

static int socket_fd;
bool running = true;

// functions to handle the server socket
static int listener_init();
static void listener_run();
static void listener_shutdown();

// options
#define IS_OPT(n) (strcmp(long_options[option_index].name, n) == 0)

static const struct option long_options[] = {
    { "root", required_argument, NULL, 0 },
    { NULL, no_argument, NULL, 0 }
};

int main(int argc, char *argv[]) {
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
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("creating socket");
        return errno;
    }

    {
        int yes = 1;
        int err = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        if (err == -1) {
            perror("setsockopt(SO_REUSEADDR)");
            return errno;
        }
    }

    struct sockaddr_in server_address = { 0 };
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons((unsigned short)CAFIINE_PORT);

    int err = bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));
    if (err == -1) {
        perror("bind");
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
            perror("accept");
            return;
        }

        struct sockaddr_in *sin = (struct sockaddr_in*)&client_address;

        unsigned char *addr = (unsigned char *)&sin->sin_addr.s_addr;
        printf("%d: Incoming connection from %d.%d.%d.%d:%d\n", fd, addr[0], addr[1], addr[2], addr[3], sin->sin_port);

        connection_handle(fd);
    }
}

void listener_shutdown() {
    close(socket_fd);
}