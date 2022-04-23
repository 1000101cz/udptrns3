#include "confirmations.h"

void send_success(int socket_descriptor, struct sockaddr_in client_address) {
    unsigned char buffer[5];
    for (int i = 0; i < 5; i++) { buffer[i] = '1'; }
    sendto(socket_descriptor, buffer, sizeof(unsigned char)*5,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
}

void send_fail(int socket_descriptor, struct sockaddr_in client_address) {
    unsigned char buffer[5];
    for (int i = 0; i < 5; i++) { buffer[i] = '0'; }
    sendto(socket_descriptor, buffer, sizeof(unsigned char)*5,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
}

// receive confirmation that operation succeeded
_Bool get_conf(int socket_descriptor, struct sockaddr_in server_address, int len, _Bool last_confirmation) {
    unsigned char str[5] = {'\0'};

    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT_S;
    tv.tv_usec =  TIMEOUT_MS * 1000;
    if (last_confirmation) {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    }
    setsockopt(socket_descriptor, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

    recvfrom(socket_descriptor, str, sizeof(unsigned char)*5,MSG_WAITALL, ( struct sockaddr *) &server_address,(unsigned int*)&len);
    _Bool success = 1;

    for (int i = 0; i < 5; i++) {
        if (str[i] != '1') {
            success = 0;
            break;
        }
    }
    if (!success) {
        return 0;
    } else {
        return 1;
    }
}

// request confirmation from server
void confirmation_request(int socket_descriptor, struct sockaddr_in server_address) {
    unsigned char buffer[BUFFER_SIZE];
    buffer[0] = 'c';
    buffer[1] = 'o';
    buffer[2] = 'n';
    buffer[3] = 'f';
    buffer[4] = ' ';
    buffer[5] = 'p';
    buffer[6] = 'l';
    buffer[7] = 's';
    for (int i = 8; i < BUFFER_SIZE; i++) { buffer[i] = '1'; }
    sendto(socket_descriptor, buffer, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &server_address,sizeof(server_address));
}

// check is packet is confirmation request - server app
_Bool packet_is_request(const unsigned char *buffer) {
    if (buffer[0]!='c' || buffer[1]!='o' || buffer[2]!='n' || buffer[3]!='f' || buffer[4]!=' ' || buffer[5]!='p' || buffer[6]!='l' || buffer[7]!='s') {
        return 0;
    }
    for (int i = 8; i < BUFFER_SIZE; i++) {
        if (buffer[i] != '1') {
            return 0;
        }
    }
    return 1;
}

// send message to client that all packets were received
void everything_received_mes(int socket_descriptor, struct sockaddr_in client_address) {
    unsigned char buffer[BUFFER_SIZE];
    buffer[0] = 'g';
    buffer[1] = 'o';
    buffer[2] = 't';
    buffer[3] = ' ';
    buffer[4] = 'a';
    buffer[5] = 'l';
    buffer[6] = 'e';
    buffer[7] = 's';
    for (int i = 8; i < BUFFER_SIZE; i++) { buffer[i] = '1'; }
    sendto(socket_descriptor, buffer, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
}

_Bool everything_received_rec(const unsigned char *buffer) {
    if (buffer[0]!='g' || buffer[1]!='o' || buffer[2]!='t' || buffer[3]!=' ' || buffer[4]!='a' || buffer[5]!='l' || buffer[6]!='e' || buffer[7]!='s') {
        return 0;
    }
    for (int i = 8; i < BUFFER_SIZE; i++) {
        if (buffer[i] != '1') {
            return 0;
        }
    }
    return 1;
}