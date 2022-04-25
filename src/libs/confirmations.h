#ifndef CONFIRMATIONS_H__
#define CONFIRMATIONS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT_SERVER     32222
#define PORT_CLIENT     22223

//#define NETDERPER
#ifdef NETDERPER
    #define PORT_NETDERPER_1 14000
    #define PORT_NETDERPER_2 14001
#endif

#define BUFFER_SIZE 1024
#define SUB_BUFFER_SIZE 40
#define TIMEOUT_S 0
#define TIMEOUT_MS 100 // 10 without netderper
#define MAX_SENT_REPEAT 20

#define MAX_PACKETS_AT_TIME 50

//#define GENERATE_ERRORS

// send_success
//   * send short success message
void send_success(int socket_descriptor, struct sockaddr_in client_address);

// send_fail
//   * send short fail message
void send_fail(int socket_descriptor, struct sockaddr_in client_address);

// get_conf
//   - receive short confirmation message
_Bool get_conf(int socket_descriptor, struct sockaddr_in server_address, int len, _Bool last_confirmation);

// confirmation_request
//   - request confirmation after MAX_PACKETS_AT_ONCE packets were sent
void confirmation_request(int socket_descriptor, struct sockaddr_in server_address);

// packet_is_requested
//   - check if received packet is confirmation request
_Bool packet_is_request(const unsigned char *buffer);

#endif
