#include "../libs/confirmations.h"
#include "../libs/txt_clrs.h"
#include "../libs/crc32/crc32.h"

#ifndef UDPTRNS3_FILE_OPERATIONS_SERVER_H
#define UDPTRNS3_FILE_OPERATIONS_SERVER_H


// receive_message
//
// - receive file from client
void receive_message(char *file_dest, int socket_descriptor, struct sockaddr_in client_address, int len, long message_length);

#endif //UDPTRNS3_FILE_OPERATIONS_SERVER_H
