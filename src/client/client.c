
#include "main_func.h"

int main(int argc, char *argv[]) {

    // ERROR messages
    if (argc < 3) {
        fprintf(stderr, "Enter arguments! (address and file destination)\n");
        return 100;
    }
    if ( access(argv[2], F_OK) != 0 ) {
        fprintf(stderr, "ERROR: File %s dont exist!\n", argv[2]);
        return 100;
    }

    // Create socket file descriptor
    int socket_descriptor;
    if ( (socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        fprintf(stderr, "ERROR: Socket file descriptor failed\n");
        return 100;
    }

    printf("File: %s\n",argv[2]);
    printf("Dest: %s:%d\n",argv[1],PORT_SERVER);
    printf("----------------------\n");

    struct sockaddr_in server_address, client_address;
    memset(&server_address, 0, sizeof(server_address));
    memset(&client_address, 0, sizeof(client_address));
    int len = sizeof(server_address);

    struct timeval stop, start;

    // set client port
    client_address.sin_port = htons(PORT_CLIENT);
    bind(socket_descriptor,(struct sockaddr *)&client_address,sizeof(client_address));

    // fill server information
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_SERVER);
    server_address.sin_addr.s_addr = inet_addr(argv[1]);




    //    HANDSHAKE
    printf("\nHANDSHAKE >>>>>>>>>>>>>>>>\n");
    long n_o_char = init_handshake(socket_descriptor, server_address, len, argv[2]);


    //    FILE-TRANSFER
    printf("\nFILE-TRANSFER >>>>>>>>>>>>>>>>\n");
    gettimeofday(&start, NULL);
    send_file(argv[2], n_o_char, socket_descriptor, server_address, len);
    gettimeofday(&stop, NULL);
    long time_taken = (stop.tv_sec - start.tv_sec)*1000 + (stop.tv_usec - start.tv_usec)/1000;
    double time_taken_s = time_taken/1000.0;
    printf("Time taken: %.3f s\n",time_taken_s);
    printf("Transfer speed: %.1f MB/s\n\n",(double)(n_o_char/1000000)/time_taken_s);


    //    TERMINATION
    printf("\nTERMINATION >>>>>>>>>>>>>>>>\n");
    termination_f(argv[2], socket_descriptor, server_address, len);


    return 0;
}
