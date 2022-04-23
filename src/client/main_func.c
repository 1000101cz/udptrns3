#include "main_func.h"


long init_handshake(int socket_descriptor, struct sockaddr_in server_address, int len, char *file_address) {
    // send file length to server
    FILE *read_file;
    read_file = fopen(file_address,"rb");
    fseek(read_file, 0, SEEK_END);
    long n_o_char = ftell(read_file);
    fclose(read_file);
    unsigned char file_length_string[12] = {'\0'};
    unsigned char init_msg[25] = {'\0'};
    sprintf((char*)file_length_string, "%ld", n_o_char);
    unsigned long init_crc = compute_CRC_buffer(&file_length_string, 12);
    sprintf((char*)init_msg, "%ld %ld", n_o_char, init_crc);
    int try_number = 0;
    while (1) {
        sendto(socket_descriptor, init_msg, sizeof(unsigned char)*25,MSG_CONFIRM, (const struct sockaddr *) &server_address,sizeof(server_address));
        if (get_conf(socket_descriptor, server_address, len)) {
            break; // repeat if Success confirmation not received from server
        }
        if (try_number >= MAX_SENT_REPEAT) {
            fprintf(stderr,"Handshake failed, cannot connect to server.\nTerminating..\n");
            exit(100);
        }
        try_number++;
    }
    printf("Connection established...\n\nLength of file: %ld\n",n_o_char);
    return n_o_char;
}


void termination_f(char *file_address, int socket_descriptor, struct sockaddr_in server_address, int len) {
    // send SHA256 hash of file to server
    char hash[65] = {0}; // create empty array
    hash_file(file_address,hash); // fill array with file hash
    unsigned char str[BUFFER_SIZE] = {'\0'};
    sprintf((char*)str, "%s", hash); // fill str with file hash
    sendto(socket_descriptor, str, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &server_address,sizeof(server_address)); // send file hash to server

    // wait for SHA256 hash confirmation
    if (get_last_conf(socket_descriptor, server_address, len)) {
        printf("\nSuccess\n");
        close(socket_descriptor);
    } else {
        fprintf(stderr,"Transfer failed - SHA256 hashes do not match!\n");
        close(socket_descriptor);
        exit(100);
    }
}