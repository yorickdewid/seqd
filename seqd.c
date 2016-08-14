#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFLEN 512 // max length of buffer
#define PORT 53 // the port on which to listen for incoming data

//const unsigned char sequence[4] = {0x17, 0x82, 0x3a, 0x0};
const unsigned char sequence[4] = {'k', 'a', 'a', 's'};
const char command_add[] = "echo \"pass in on vtnet0 proto tcp from any to any port 8080 keep state\" | pfctl -a temp -f -";
const char command_del[] = "pfctl -a temp -F rules";
int timeout = 60;

void die(char *s) {
    perror(s);
    exit(1);
}

void release() {
    int i;

    for (i=timeout; i>0; --i) {
        sleep(1);
        if (i%10==0)
            printf("Closing in %d\n", i);
    }

    system(command_del);
}

int main(void) {
    struct sockaddr_in si_me, si_other;

	int s, i, recv_len, matchpos = 0;
    unsigned int slen = sizeof(si_other);
    char buf[BUFLEN];

    // create a UDP socket
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        die("socket");
    }

    // zero out the structure
    memset((char *)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket to port
    if (bind(s, (struct sockaddr*)&si_me, sizeof(si_me)) < 0) {
        die("bind");
    }

    // keep listening for data
    while (1) {
        // try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen)) < 0) {
            die("recvfrom()");
        }

        // match the sequence
        if (memcmp((const void *)sequence, buf, sizeof(sequence)/sizeof(sequence[0])))
            continue;

        puts("Activated");
        system(command_add);
        release();
    }

    close(s);
    return 0;
}
