
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
    printf("---------------------------------------------------\n");

    // Create socket file descriptor
    int socket_descriptor;
    if ( (socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { exit(100); }

    struct sockaddr_in server_address, client_address;
    memset(&server_address, 0, sizeof(server_address));
    memset(&client_address, 0, sizeof(client_address));
    int len = sizeof(client_address);

#ifdef NETDERPER
    print_text("NETDERPER ON\n",RED,1,0);
    client_address.sin_port = htons(PORT_NETDERPER_2);
#endif
#ifndef NETDERPER
    print_text("NETDERPER OFF\n",GREEN,1,0);
#endif

    // fill server information
    server_address.sin_family    = AF_INET; // IPv4
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT_SERVER);

    // bind socket with server address
    if ( bind(socket_descriptor, (const struct sockaddr *)&server_address,sizeof(server_address)) < 0 ) { exit(100); }



    //    HANDSHAKE
    print_text("\nHANDSHAKE\n",BLUE,1,0);
    long message_length = init_handshake(socket_descriptor, client_address, len);



    //    FILE-TRANSFER
    print_text("\nFILE-TRANSFER\n",BLUE,1,0);
    printf("  - receiving file\n");
    receive_message(file_dest, socket_descriptor, client_address, len, message_length);



    //     TERMINATION
    print_text("\nTERMINATION\n",BLUE,1,0);
    if (termination_f(file_dest, socket_descriptor, client_address, len)) {
        print_text("\nTRANSFER FAILED!\n",RED,1,0);
    } else {
        print_text("\nSUCCESS\n",GREEN,1,0);
    }

    return 0;
}
