#include "../libs/confirmations.h"
#include "../libs/txt_clrs.h"
#include "../crc32/crc32.h"

#ifndef UDPTRNS3_FILE_OPERATIONS_CLIENT_H
#define UDPTRNS3_FILE_OPERATIONS_CLIENT_H


// send_file
//
// - send file from client to server
void send_file(char *file_dest, long n_o_char, int socket_descriptor, struct sockaddr_in server_address, int len);

#endif //UDPTRNS3_FILE_OPERATIONS_CLIENT_H
