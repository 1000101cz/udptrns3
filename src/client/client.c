
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

    struct sockaddr_in server_address, client_address;
    memset(&server_address, 0, sizeof(server_address));
    memset(&client_address, 0, sizeof(client_address));
    int len = sizeof(server_address);

    struct timeval stop, start;

    // set client port
    client_address.sin_port = htons(PORT_CLIENT);
    if (bind(socket_descriptor,(struct sockaddr *)&client_address,sizeof(client_address)) != 0) {
        fprintf(stderr, "Port %d binding error\n",PORT_CLIENT);
        exit(100);
    } else {
        printf("File dst:  %s\n",argv[2]);
        printf("Transfer:  IP_addr:%d   -> %15s:%d\n",PORT_CLIENT,argv[1],PORT_SERVER);
        for (int i = 0; i < strlen(argv[1]) + 42; i++) {
            printf("-");
        }
        printf("\n");
    }

    // fill server information
    server_address.sin_family = AF_INET;
#ifdef NETDERPER
    server_address.sin_port = htons(PORT_NETDERPER_1);
    print_text("NETDERPER ON\n",RED,1,0);
#endif
#ifndef NETDERPER
    print_text("NETDERPER OFF\n",GREEN,1,0);
    server_address.sin_port = htons(PORT_SERVER);
#endif
    server_address.sin_addr.s_addr = inet_addr(argv[1]);




    //    HANDSHAKE
    print_text("\nHANDSHAKE\n",BLUE,1,0);
    long n_o_char = init_handshake(socket_descriptor, server_address, len, argv[2]);


    //    FILE-TRANSFER
    print_text("\nFILE-TRANSFER\n",BLUE,1,0);
    gettimeofday(&start, NULL);
    printf("  - sending file\n");
    send_file(argv[2], n_o_char, socket_descriptor, server_address, len);
    gettimeofday(&stop, NULL);
    long time_taken = (stop.tv_sec - start.tv_sec)*1000 + (stop.tv_usec - start.tv_usec)/1000;
    printf("  - time taken:      %5ld  [ms]\n",time_taken);
    printf("  - transfer speed:   %.1f  [MB/s]\n",(double)(n_o_char/1000000)/(time_taken/1000.0));


    //    TERMINATION
    print_text("\nTERMINATION\n",BLUE,1,0);
    if (termination_f(argv[2], socket_descriptor, server_address, len)) {
        print_text("\nTRANSFER FAILED!\n",RED,1,0);
    } else {
        print_text("\nSUCCESS\n",GREEN,1,0);
    }


    return 0;
}
