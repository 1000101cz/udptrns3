


#include "main_func.h"



int main(int argc, char *argv[]) {

    // ERROR messages
    if (argc < 2) {
        fprintf(stderr,"ERROR: destination for new file required!\n");
        return 100;
    }
    char *file_dest = argv[1];
    if ( access(file_dest, F_OK) == 0 ) {
        fprintf(stderr,"ERROR: File exists!\n");
        return 100;
    }
    printf("File: %s\n",argv[1]);

    // Create socket file descriptor
    int socket_descriptor;
    if ( (socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { exit(100); }

    struct sockaddr_in server_address, client_address;
    memset(&server_address, 0, sizeof(server_address));
    memset(&client_address, 0, sizeof(client_address));
    int len = sizeof(client_address);

    // fill server information
    server_address.sin_family    = AF_INET; // IPv4
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // bind socket with server address
    if ( bind(socket_descriptor, (const struct sockaddr *)&server_address,sizeof(server_address)) < 0 ) { exit(100); }



    //    HANDSHAKE
    long message_length = init_handshake(socket_descriptor, client_address, len);



    //    FILE-TRANSFER
    receive_message(file_dest, socket_descriptor, client_address, len, message_length);



    //     TERMINATION
    _Bool op_exit = termination_f(file_dest, socket_descriptor, client_address, len);



    return op_exit;
}
