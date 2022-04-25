

#include "main_func.h"

// initialize communication with client
long init_handshake(int socket_descriptor, struct sockaddr_in client_address, int len) {
    unsigned char init_buffer[BUFFER_SIZE] = {'\0'};
    unsigned char sub_init_buffer[12] = {'\0'};
    char *ptr;
    int buf_2_position;
    long init_crc, message_length;
    unsigned long computed_init_crc;
    print_text("  - receiving handshake message\n",0,0);
    int try_number = 0;
    while (1) {
        recvfrom(socket_descriptor, init_buffer, sizeof(unsigned char) * 25, MSG_WAITALL,(struct sockaddr *) &client_address, (unsigned int *) &len);
#ifdef NETDERPER
        client_address.sin_port = htons(PORT_NETDERPER_2);
#endif
        buf_2_position = 0;
        for (int i = 0; i < 12; i++) {
            if (init_buffer[i] != ' ') {
                sub_init_buffer[i] = init_buffer[i];
            } else {
                message_length = strtol((const char *)sub_init_buffer,&ptr,10);
                computed_init_crc = compute_CRC_buffer(&sub_init_buffer, 12);
                for (int j = i + 1; j < 25; j++) {
                    if (init_buffer[j] != '\0') {
                        sub_init_buffer[buf_2_position] = init_buffer[j];
                        buf_2_position++;
                    } else {
                        sub_init_buffer[buf_2_position] = '\0';
                        init_crc = strtol((const char *)sub_init_buffer,&ptr,10);
                        break;
                    }
                }
                break;
            }
        }

        if (computed_init_crc == init_crc) {
#ifdef GENERATE_ERRORS
            if (try_number != 0) {  // Don't send first confirmation
                send_success(socket_descriptor, client_address);
            } else {
                print_text("  % ERROR_GEN: ",YELLOW,0);
                print_text("discarding confirmation ( in init_handshake() )\n",0,0);
            }
#endif
#ifndef GENERATE_ERRORS
            send_success(socket_descriptor, client_address);
#endif
            break;
        } else {
            send_fail(socket_descriptor, client_address);
            print_text("  ! invalid CRC\n",RED,0);
        }
        try_number++;
    }
    print_text("  - handshake successfully received\n",GREEN,0);

    // get IP address of client (not necessary)
    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_address;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, ip_str, INET_ADDRSTRLEN );

    char string[100];
    sprintf(string,"  - source address:  %s:%d\n",ip_str,ntohs(client_address.sin_port));
    print_text(string,0,0);
    sprintf(string,"  - length of file:  %ld [kB]\n",message_length/1000);
    print_text(string,0,0);

    return message_length;
}

// receive sha256 file hash and compare it with computed hash
_Bool termination_f(char *file_dest, int socket_descriptor, struct sockaddr_in client_address, int len) {
    // compute SHA256 hash
    print_text("  - computing sha256 hash\n",0,0);
    char hash[65] = {0}; // create empty hash array
    hash_file(file_dest,hash); // fill array with hash

    char string1[90];
    sprintf(string1,"  - computed hash: %s\n",hash);
    printf(string1,0,0);

    // receive SHA256 hash
    print_text("  - receiving sha256 hash\n",0,0);
    unsigned char buffer[BUFFER_SIZE] = {'\0'};
    recvfrom(socket_descriptor, buffer, sizeof(unsigned char)*BUFFER_SIZE,MSG_WAITALL, ( struct sockaddr *) &client_address,(unsigned int*)&len); // receive hash
    while (1) {
        if (packet_is_request(buffer)) {
            print_text("  ! received request, expected hash\n", RED, 0);
            send_success(socket_descriptor, client_address);
            recvfrom(socket_descriptor, buffer, sizeof(unsigned char)*BUFFER_SIZE,MSG_WAITALL, ( struct sockaddr *) &client_address,(unsigned int*)&len); // receive hash
        } else {
            break;
        }
    }
    char string[1050];
    sprintf(string,"  - received hash: %s\n",buffer);
    print_text(string,0,0);

    // compare received and computed hashes
    _Bool fucked = 0;
    for (int i = 0; i < 65; i++) {
        if (hash[i] != buffer[i]) {
            print_text("  - hashes do not match!\n",RED,0);
            fucked = 1;
            send_fail(socket_descriptor, client_address);
            break;
        }
    }

    if (!fucked) {
        print_text("  + hashes are identical\n",GREEN,0);
        send_success(socket_descriptor, client_address); // send Success if they match
    }
    return fucked;
}