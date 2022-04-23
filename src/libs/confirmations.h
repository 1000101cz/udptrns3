#ifndef CONFIRMATIONS_H__
#define CONFIRMATIONS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT     32222
#define BUFFER_SIZE 1024
#define SUB_BUFFER_SIZE 40
#define TIMEOUT_S 1
#define TIMEOUT_MS 0
#define CRC_SIZE 4
#define MAX_SENT_REPEAT 20

#define MAX_PACKETS_AT_TIME 50

//#define GENERATE_ERRORS
#ifdef GENERATE_ERRORS
  #define DONT_SENT_CONF 900 // 1 in value (program will sleep for TIMEOUT each time)
  #define CORRUPT_PACKET 50 // 1 in value
#endif

// send_success
//   * send short success message
void send_success(int socket_descriptor, struct sockaddr_in client_address);

// send_fail
//   * send short fail message
void send_fail(int socket_descriptor, struct sockaddr_in client_address);

// get_conf
//   - receive short confirmation message
_Bool get_conf(int socket_descriptor, struct sockaddr_in server_address, int len);

// confirmation_request
//   - request confirmation after MAX_PACKETS_AT_ONCE packets were sent
void confirmation_request(int socket_descriptor, struct sockaddr_in server_address);

// packet_is_requested
//   - check if received packet is confirmation request
_Bool packet_is_request(const unsigned char *buffer);

// everything_received_mes
//   - send message to client that all packets were received
void everything_received_mes(int socket_descriptor, struct sockaddr_in client_address);

// everything_received_rec
//   - check if received message is everything_received - client app
_Bool everything_received_rec(const unsigned char *buffer);

// get_last_confirmation
//   - receive last confirmation that operation succeeded - no timeout
_Bool get_last_conf(int socket_descriptor, struct sockaddr_in server_address, int len);

#endif
