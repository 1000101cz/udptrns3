#include "confirmations.h"

struct sockaddr_in set_port(struct sockaddr_in address, int port, _Bool wait) {
#ifdef NETDERPER
    if (wait) {
        usleep(120000); // conf request must be last message
    }
    address.sin_port = htons(port);
#endif
    return address;
}

void send_success(int socket_descriptor, struct sockaddr_in client_address) {
    unsigned char buffer[5];
    for (int i = 0; i < 5; i++) { buffer[i] = '1'; }

    client_address = set_port(client_address,PORT_NETDERPER_2, 0);

    sendto(socket_descriptor, buffer, sizeof(unsigned char)*5,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
    print_text("  - SUCCESS confirmation sent\n",GRAY,0,1);
}

void send_fail(int socket_descriptor, struct sockaddr_in client_address) {
    unsigned char buffer[5];
    for (int i = 0; i < 5; i++) { buffer[i] = '0'; }

    client_address = set_port(client_address, PORT_NETDERPER_2, 0);

    sendto(socket_descriptor, buffer, sizeof(unsigned char)*5,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
    print_text("  - FAIL confirmation sent\n",GRAY,0,1);
}

// receive confirmation that operation succeeded
_Bool get_conf(int socket_descriptor, struct sockaddr_in server_address, int len, _Bool last_confirmation) {
    unsigned char str[5] = {'\0'};

    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT_S;
    tv.tv_usec =  TIMEOUT_MS * 1000;
    setsockopt(socket_descriptor, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

    recvfrom(socket_descriptor, str, sizeof(unsigned char)*5,MSG_WAITALL, ( struct sockaddr *) &server_address,(unsigned int*)&len);

    tv.tv_sec = TIMEOUT_S;
    tv.tv_usec = TIMEOUT_MS*1000;
    setsockopt(socket_descriptor, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

    int n_o_0 = 0;
    int n_o_1 = 0;

    for (int i = 0; i < 5; i++) {
        if (str[i] == '1') {
            n_o_1++;
        } else if (str[i] == '0') {
            n_o_0++;
        }
    }
    if (n_o_0 >= 3) {
        print_text("  - received FAIL confirmation\n",GRAY,0,1);
        return 0;
    } else if (n_o_1 >= 3) {
        print_text("  - received SUCCESS confirmation\n",GRAY,0,1);
        return 1;
    } else {
        print_text("  ! did not receive confirmation (handle as FAIL)\n",RED,0,1);
        return 0;
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

    set_port(server_address, PORT_NETDERPER_1, 1);

    sendto(socket_descriptor, buffer, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &server_address,sizeof(server_address));
    print_text("  - confirmation request sent\n",GRAY,0,1);
}

// check is packet is confirmation request - server app
_Bool packet_is_request(const unsigned char *buffer) {

    int number_of_ones = 0;
    for (int i = 8; i < 30; i++) {
        if (buffer[i] == '1') {
            number_of_ones++;
        }
    }

    if (buffer[0]!='c' || buffer[1]!='o' || buffer[2]!='n' || buffer[3]!='f' || buffer[4]!=' ' || buffer[5]!='p' || buffer[6]!='l' || buffer[7]!='s' || number_of_ones < 20) {
        return 0;
    }

    print_text("  - confirmation request received\n",GRAY,0,1);
    return 1;
}