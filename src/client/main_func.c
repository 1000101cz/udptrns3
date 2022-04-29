#include "main_func.h"


long init_handshake(int socket_descriptor, struct sockaddr_in server_address, int len, char *file_address) {
    print_text("  - getting size of file\n",0,0,1);
    FILE *read_file;
    read_file = fopen(file_address,"rb");
    fseek(read_file, 0, SEEK_END);
    long n_o_char = ftell(read_file);
    fclose(read_file);

    print_text("  - computing crc of handshake message\n",0,0,1);
    unsigned char file_length_string[12] = {'\0'};
    unsigned char init_msg[25] = {'\0'};
    sprintf((char*)file_length_string, "%ld", n_o_char);
    unsigned long init_crc = compute_CRC_buffer(&file_length_string, 12);
    sprintf((char*)init_msg, "%ld %ld", n_o_char, init_crc);
    print_text("  - sending handshake message to server\n",0,0,1);
    int try_number = 0;
    while (1) {
        sendto(socket_descriptor, init_msg, sizeof(unsigned char) * 25, MSG_CONFIRM,(const struct sockaddr *) &server_address, sizeof(server_address));
        if (get_conf(socket_descriptor, server_address, len, 0)) {
            break; // repeat if Success confirmation not received from server
        }
        else {
            print_text("  - repeating handshake\n",RED,0,1);
        }
        if (try_number >= MAX_SENT_REPEAT) {
            print_text("Handshake failed, cannot connect to server.\nTerminating..\n",RED,0,0);
            exit(100);
        }
        try_number++;
    }
    print_text("  + handshake message successfully received by server\n",GREEN,0,0);
    return n_o_char;
}


_Bool termination_f(char *file_address, int socket_descriptor, struct sockaddr_in server_address, int len) {
    // compute SHA256 hash
    print_text("  - computing sha256 hash\n",0,0,1);
    char hash[HASH_BUFFER_SIZE] = {0}; // create empty array
    hash_file(file_address,hash); // fill array with file hash
    char string[90];
    sprintf(string,"  - computed hash: %s\n",hash);
    print_text(string,0,0,0);

    // send SHA256 hash
    for (int i = 0; i < 5; i++) {
        print_text("  - sending sha256 hash\n", 0, 0, 1);
        sendto(socket_descriptor, hash, sizeof(unsigned char) * HASH_BUFFER_SIZE, MSG_CONFIRM,(const struct sockaddr *) &server_address, sizeof(server_address)); // send file hash to server
    }

    // wait for SHA256 hash confirmation
    if (get_conf(socket_descriptor, server_address, len, 1)) {
        print_text("  + received success from server\n",GREEN,0,0);
        close(socket_descriptor);
        return 0;
    } else {
        print_text("  - received fail from server!\n",RED,0,0);
        close(socket_descriptor);
        return 1;
    }
}