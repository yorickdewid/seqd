#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFLEN 512 // max length of buffer
#define PROGNAME "seqd"
#define DEF_PORT 53 // the port on which to listen for incoming data

const char command_add[] = "echo \"pass in on vtnet0 proto tcp from any to any port 8080 keep state\" | pfctl -a temp -f -";
const char command_del[] = "pfctl -a temp -F rules";
int timeout = 60;

#define nsz(a) (sizeof(a)/sizeof(a[0]))

// options
struct tagopt {
    char v4_only;
    char v6_only;
    char silent;
    short port;
    unsigned char seq[4];
    char *command_up;
    char *command_down;
};

void release() {
    int i;

    for (i=timeout; i>0; --i) {
        sleep(1);
        if (i%10==0)
            printf("Down in %d\n", i);
    }

    system(command_del);
}

void usage() {
    puts(
        "usage: " PROGNAME " [-46q] [-p port] [-s seq]\n"
        "        [-u command] [-d command]"
    );
}

int plisten(struct tagopt *options) {
    struct sockaddr_in si_me, si_other;

    int s, i, recv_len, matchpos = 0;
    unsigned int slen = sizeof(si_other);
    char buf[BUFLEN];

    // create a UDP socket
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket()");
        return 1;
    }

    if (options->v4_only)
        printf("binding ipv4\n");
    else if (options->v6_only)
        printf("binding ipv4\n");
    else
        printf("binding dual stack\n");
    printf("port %d\n", options->port);

    // zero out the structure
    memset((char *)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(options->port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket to port
    if (bind(s, (struct sockaddr*)&si_me, sizeof(si_me)) < 0) {
        perror("bind()");
        return 1;
    }

    // keep listening for data
    while (1) {
        // try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen)) < 0) {
            perror("recvfrom()");
            return 1;
        }

        // match the sequence
        if (memcmp((const void *)options->seq, buf, nsz(options->seq)))
            continue;

        puts("Up");
        system(options->command_up);

        if (!options->command_down)
            continue;

        for (i=timeout; i>0; --i) {
            sleep(1);
            printf("\rDown in %02d", i);
            fflush(stdout);
        }
        system(options->command_down);
        puts("\rDown            \n");
    }

    close(s);
    return 0;
}

int main(int argc, char *argv[]) {
    int i, opt = 0;

    struct tagopt options;
    options.port = DEF_PORT;
    options.v4_only = 0;
    options.v6_only = 0;
    options.silent = 0;
    options.seq[0] = 0x0;
    options.command_up = NULL;
    options.command_down = NULL;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s")) {
            if (i+1>argc-1) {
                usage();
                return 1;
            }
            memcpy(options.seq, argv[++i], 4);
        } else if (!strcmp(argv[i], "-u")) {
            if (i+1>argc-1) {
                usage();
                return 1;
            }
            options.command_up = argv[++i];
        } else if (!strcmp(argv[i], "-d")) {
            if (i+1>argc-1) {
                usage();
                return 1;
            }
            options.command_down = argv[++i];
        } else if (!strcmp(argv[i], "-p")) {
            if (i + 1 <= argc - 1) {
                options.port = atoi(argv[++i]);
                if (!options.port) {
                    usage();
                    return 1;
                }
            }
        } else if (!strcmp(argv[i], "-4")) {
            puts("v4 only");
            if (options.v6_only) {
                usage();
                return 1;
            }
            options.v4_only = 1;
        } else if (!strcmp(argv[i], "-6")) {
            puts("v6 only");
            if (options.v4_only) {
                usage();
                return 1;
            }
            options.v6_only = 1;
        } else if (!strcmp(argv[i], "-q")) {
            puts("quiet");
            options.silent = 1;
        } else {
            usage();
            return 1;
        }
    }

    if (!options.seq || !options.command_up) {
        usage();
        return 1;
    }

    return plisten(&options);
}
