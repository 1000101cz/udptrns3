

#include "main_func.h"

// initialize communication with client
long init_handshake(int socket_descriptor, struct sockaddr_in client_address, int len) {
    unsigned char init_buffer[25] = {'\0'};
    unsigned char sub_init_buffer[12] = {'\0'};
    char *ptr;
    int buf_2_position;
    long init_crc, message_length;
    unsigned long computed_init_crc;
    printf("  - receiving handshake message\n");
    while (1) {
        recvfrom(socket_descriptor, init_buffer, sizeof(unsigned char) * 25, MSG_WAITALL,(struct sockaddr *) &client_address, (unsigned int *) &len);
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
            send_success(socket_descriptor, client_address);
            break;
        } else {
            send_fail(socket_descriptor, client_address);
            fprintf(stderr, "Invalid CRC, repeating\n");
        }
    }
    printf("  - handshake successfully received\n");

    // get IP address of client (not necessary)
    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_address;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, ip_str, INET_ADDRSTRLEN );
    printf("  - source address:  %s:%d\n",ip_str,ntohs(client_address.sin_port));
    printf("  - length of file:  %ld [bytes]\n",message_length);

    return message_length;
}

// receive sha256 file hash and compare it with computed hash
_Bool termination_f(char *file_dest, int socket_descriptor, struct sockaddr_in client_address, int len) {
    // compute SHA256 hash
    printf("  - computing sha256 hash\n");
    char hash[65] = {0}; // create empty hash array
    hash_file(file_dest,hash); // fill array with hash
    printf("  - computed hash: %s\n",hash);

    // receive SHA256 hash
    printf("  - receiving sha256 hash\n");
    unsigned char buffer[BUFFER_SIZE] = {'\0'};
    recvfrom(socket_descriptor, buffer, sizeof(unsigned char)*BUFFER_SIZE,MSG_WAITALL, ( struct sockaddr *) &client_address,(unsigned int*)&len); // receive hash
    printf("  - received hash: %s\n",buffer);

    // compare received and computed hashes
    printf("  - comparing sha256 hashes\n");
    _Bool fucked = 0;
    for (int i = 0; i < 65; i++) {
        if (hash[i] != buffer[i]) {
            fprintf(stderr, "ERROR: Hashes did not match!\n"); // send Error if they differ
            fucked = 1;
            send_fail(socket_descriptor, client_address);
            break;
        }
    }

    if (!fucked) {
        print_text("  - hashes are identical\n",GREEN,0);
        send_success(socket_descriptor, client_address); // send Success if they match
    } else {
        print_text("  - hashes do not match!\n",RED,0);
    }
    return fucked;
}