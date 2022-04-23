#ifndef UDPTRNS3_MAIN_FUNC_H
#define UDPTRNS3_MAIN_FUNC_H


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../sha256/sha256.h"
#include "../crc32/crc32.h"
#include "../libs/confirmations.h"
#include "../libs/file_operations.h"
#include "../libs/txt_clrs.h"


// init_handshake
//
// - initialize communication with server
// - send length of file and receive confirmation
long init_handshake(int socket_descriptor, struct sockaddr_in server_address, int len, char *file_address);


// termination_f
//
// - terminate transmission
// - send sha256 file hash and receive confirmation
_Bool termination_f(char *file_address, int socket_descriptor, struct sockaddr_in server_address, int len);



#endif //UDPTRNS3_MAIN_FUNC_H
