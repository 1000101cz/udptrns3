#ifndef UDPTRNS3_MAIN_FUNC_H
#define UDPTRNS3_MAIN_FUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../sha256/sha256.h"
#include "../crc32/crc32.h"
#include "../libs/confirmations.h"
#include "../libs/file_operations.h"
#include "../libs/txt_clrs.h"

// init_handshake
//
// - initialize communication with client
// - receive length of file and send confirmation
long init_handshake(int socket_descriptor, struct sockaddr_in client_address, int len);


// termination_f
//
// - terminate transmission
// - receive sha256 file hash and compare it with computed hash
_Bool termination_f(char *file_dest, int socket_descriptor, struct sockaddr_in client_address, int len);


#endif //UDPTRNS3_MAIN_FUNC_H
