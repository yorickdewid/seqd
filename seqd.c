#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFLEN 512
#define PROGNAME "seqd"
#define DEF_PORT 53
#define DEF_TIMEOUT 60

const char command_add[] = "echo \"pass in on vtnet0 proto tcp from any to any port 8080 keep state\" | pfctl -a temp -f -";
const char command_del[] = "pfctl -a temp -F rules";

#define nsz(a) (sizeof(a)/sizeof(a[0]))

// options
struct tagopt {
    char v4_only;
    char v6_only;
    char silent;
    short port;
    short timeout;
    unsigned char seq[4];
    char *command_up;
    char *command_down;
};

void usage() {
    puts(
        "usage: " PROGNAME " [-46q] [-p port] [-s seq]\n"
        "        [-u command] [-d command] [-t sec]"
    );
}

int plisten(struct tagopt *options) {
    struct sockaddr_in si_me, si_other;
    struct sockaddr_in6 si6_me, si6_other;

    int s, i, recv_len, matchpos = 0, _y = 1, _n = 0;
    unsigned int slen = sizeof(si_other);
    unsigned int slen6 = sizeof(si6_other);
    char buf[BUFLEN];

    // create a UDP socket
    if ((s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket()");
        return 1;
    }

    // reuse socket
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &_y, sizeof(_y)) < 0) {
        perror("setsockopt()");
        return 1;
    }

    if (options->v6_only)
        _n = 1;

    // dual stack (or not)
    if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &_n, sizeof(_n)) < 0) {
        perror("setsockopt()");
        return 1;
    }

    if (options->v4_only)
        printf("binding ipv4\n");
    else if (options->v6_only)
        printf("binding ipv6\n");
    else
        printf("binding dual stack\n");

    // zero out the structure
    //memset((char *)&si_me, 0, sizeof(si_me));
    //si_me.sin_family = AF_INET;
    //si_me.sin_port = htons(options->port);
    //si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    memset((char *)&si6_me, 0, sizeof(si6_me));
    si6_me.sin6_family = AF_INET6;
    si6_me.sin6_port = htons(options->port);
    si6_me.sin6_addr = in6addr_any;

    // bind socket to port
    if (bind(s, (struct sockaddr*)&si6_me, sizeof(si6_me)) < 0) {
        perror("bind()");
        return 1;
    }

    // keep listening for data
    while (1) {
        // try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si6_other, &slen6)) < 0) {
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

        for (i=options->timeout; i>0; --i) {
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
    options.timeout = DEF_TIMEOUT;
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
            const char *seq = argv[++i];
            memcpy(options.seq, seq, 4);
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
        } else if (!strcmp(argv[i], "-t")) {
            if (i + 1 <= argc - 1) {
                options.timeout = atoi(argv[++i]);
                if (!options.timeout) {
                    usage();
                    return 1;
                }
            }
        } else if (!strcmp(argv[i], "-4")) {
            if (options.v6_only) {
                usage();
                return 1;
            }
            options.v4_only = 1;
        } else if (!strcmp(argv[i], "-6")) {
            if (options.v4_only) {
                usage();
                return 1;
            }
            options.v6_only = 1;
        } else if (!strcmp(argv[i], "-q")) {
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
