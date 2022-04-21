#include "file_operations.h"

// send file to server from client
void send_file(char *file_dest, long n_o_char, int socket_descriptor, struct sockaddr_in server_address, int len) {
    long pointer = 0;
    int number_of_packets=1;
    unsigned char buffer[BUFFER_SIZE - SUB_BUFFER_SIZE] = {'\0'};
    unsigned char sub_buffer[SUB_BUFFER_SIZE] = {'\0'};
    unsigned char big_buffer[BUFFER_SIZE] = {'\0'};
    unsigned char mega_buffer[MAX_PACKETS_AT_TIME * BUFFER_SIZE] = {'\0'};
    FILE *read_file = fopen(file_dest,"rb");
    unsigned long CRC_value;
    _Bool sending_file = 1;
    long total_error_count = 0;
    _Bool last_packet_sent = 0;
    long packets_at_total = n_o_char/(BUFFER_SIZE-SUB_BUFFER_SIZE) + 1;

    while(sending_file) {

            // create mega_buffer with all packets prepared to send at once
            for (int packet_n = 0; packet_n < MAX_PACKETS_AT_TIME; packet_n++) {
                // clear buffer
                for (int i = 0; i < BUFFER_SIZE - SUB_BUFFER_SIZE; i++) { buffer[i] = '\0'; }

                // fill buffer
                for (int i = 0; i < BUFFER_SIZE - SUB_BUFFER_SIZE && pointer < n_o_char; i++) {
                    buffer[i] = getc(read_file);
                    pointer++;
                }

                // create CRC and packet number sub_buffer
                CRC_value = compute_CRC_buffer(&buffer, BUFFER_SIZE - SUB_BUFFER_SIZE);// compute CRC
                sprintf((unsigned char *) sub_buffer, "%d %ld", number_of_packets, CRC_value);

                // connect buffers (data + CRC)
                for (int i = 0; i < BUFFER_SIZE - SUB_BUFFER_SIZE; i++) {
                    big_buffer[i] = buffer[i];
                }
                for (int i = BUFFER_SIZE - SUB_BUFFER_SIZE; i < BUFFER_SIZE; i++) {
                    big_buffer[i] = sub_buffer[i - (BUFFER_SIZE - SUB_BUFFER_SIZE)];
                }

                for (int i = 0; i < BUFFER_SIZE; i++) {
                    mega_buffer[packet_n * BUFFER_SIZE + i] = big_buffer[i];
                }
                if (number_of_packets >= packets_at_total) {
                    last_packet_sent = 1;
                    break;
                }
                number_of_packets++;
            }


            // send all data in mega_packet
            for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
                for (int j = 0; j < BUFFER_SIZE; j++) {
                    big_buffer[j] = mega_buffer[j + i*BUFFER_SIZE];
                }
                // send buffer with data, CRC and packet number
                sendto(socket_descriptor, big_buffer, sizeof(unsigned char)*(BUFFER_SIZE),MSG_CONFIRM, (const struct sockaddr *) &server_address,sizeof(server_address));
                if (last_packet_sent && (i+1)==packets_at_total%MAX_PACKETS_AT_TIME) {
                    break;
                }
            }

            // send confirmation request
            confirmation_request(socket_descriptor,server_address);

            // receive confirmation or got_ales
            recvfrom(socket_descriptor, big_buffer, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &server_address,(unsigned int*)&len);
            if (everything_received_rec(big_buffer)) {
                break;
            }
            for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
                if (big_buffer[i] == '0') {
                    while (1) { // send corrupted packets again
                        for (int j = 0; j < BUFFER_SIZE; j++) {
                            big_buffer[j] = mega_buffer[j + BUFFER_SIZE * i];
                        }
                        sendto(socket_descriptor, big_buffer, sizeof(unsigned char) * (BUFFER_SIZE), MSG_CONFIRM,
                               (const struct sockaddr *) &server_address, sizeof(server_address));
                        if (get_conf(socket_descriptor, server_address, len)) {
                            break;
                        }
                    }
                }
            }
    }

    printf("Data packets sent: %d \n",number_of_packets);
    printf("Total number of corrupted packets: %ld\n", total_error_count);

    fclose(read_file);
}

long separate_number(unsigned char *buffer) {
    unsigned char packet_number_buffer[SUB_BUFFER_SIZE/2] = {'\0'};
    long packet_number;
    for (int i = BUFFER_SIZE-SUB_BUFFER_SIZE; i < BUFFER_SIZE; i++) {
        if (buffer[i] != ' ') {
            packet_number_buffer[i-(BUFFER_SIZE-SUB_BUFFER_SIZE)] = buffer[i];
        } else {
            break;
        }
    }
    packet_number = atol(packet_number_buffer);
    return packet_number;
}

long separate_crc(unsigned char *buffer) {
    unsigned char crc_buffer[SUB_BUFFER_SIZE/2] = {'\0'};
    long crc_received;
    for (int i = BUFFER_SIZE - SUB_BUFFER_SIZE; i < BUFFER_SIZE; i++) {
        if (buffer[i] == ' ') {
            for (int j = i; j < i+SUB_BUFFER_SIZE/2; j++) {
                crc_buffer[j-1-i] = buffer[j];
            }
            break;
        }
    }
    crc_received = atol(crc_buffer);
    return crc_received;
}

// receive file from client
void receive_message(char *file_dest, int socket_descriptor, struct sockaddr_in client_address, int len, long message_length) {
    unsigned char buffer[BUFFER_SIZE-SUB_BUFFER_SIZE] = {'\0'};
    unsigned char packet_number_crc[BUFFER_SIZE] = {'\0'};
    unsigned char packet_number_buffer[SUB_BUFFER_SIZE/2];
    unsigned char crc_buffer[SUB_BUFFER_SIZE/2];
    unsigned char mega_buffer[(BUFFER_SIZE - SUB_BUFFER_SIZE)*MAX_PACKETS_AT_TIME];
    _Bool received_packets[MAX_PACKETS_AT_TIME];
    long counter = 0;
    long packet_counter = 0;
    long crc_received, packet_number, crc_computed;
    long total_error_count = 0;
    FILE *new_file;
    new_file = fopen(file_dest,"wb");
    _Bool receiving_file = 1;
    long packets_at_total = message_length/(BUFFER_SIZE-SUB_BUFFER_SIZE)+1;

    while (receiving_file) {
        while (1) {
            for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) { received_packets[i] = 1; }

            // receive MAX_PACKETS_AT_TIME packets
            for (int i = 0; i < MAX_PACKETS_AT_TIME + 1; i++) {
                // receive packet with data number and CRC
                recvfrom(socket_descriptor, packet_number_crc, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &client_address,(unsigned int*)&len);
                if (packet_is_request(packet_number_crc)) {
                    break;
                }
                packet_counter++;

                // separate number
                packet_number = separate_number(packet_number_crc);

                // separate CRC
                crc_received = separate_crc(packet_number_crc);

                // clear buffers
                for (int i = 0; i < 20; i++) {
                  packet_number_buffer[i] = '\0';
                  crc_buffer[i] = '\0';
                }

                // separate data
                for (int i = 0; i < BUFFER_SIZE-SUB_BUFFER_SIZE; i++) {
                  buffer[i] = packet_number_crc[i];
                }

                // compute CRC
                crc_computed =  compute_CRC_buffer(&buffer,BUFFER_SIZE-SUB_BUFFER_SIZE);

                // check CRC and file number
                if (crc_computed == crc_received) {
                    received_packets[(packet_number-1)%MAX_PACKETS_AT_TIME] = 1;
                    for (int i = 0; i < BUFFER_SIZE-SUB_BUFFER_SIZE; i++) {
                        mega_buffer[((packet_number-1)%MAX_PACKETS_AT_TIME)*(BUFFER_SIZE-SUB_BUFFER_SIZE) + i] = buffer[i];
                    }
                    //break;
                } else {
                    received_packets[(packet_number-1)%MAX_PACKETS_AT_TIME] = 0;
                }
            }

            // send everything_received or confirmation
            for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
                if (received_packets[i]) {
                    packet_number_crc[i] = '1';
                } else {
                    packet_number_crc[i] = '0';
                }
            }
            if (packet_number == packets_at_total) {
                for (int i = packets_at_total % MAX_PACKETS_AT_TIME; i < MAX_PACKETS_AT_TIME; i++) {
                    packet_number_crc[i] = '1';
                }
            }

            _Bool everything_ok = 1;
            for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
                if (packet_number_crc[i] == '0') {
                    everything_ok = 0;
                }
            }

            if (!everything_ok) {
                sendto(socket_descriptor, packet_number_crc, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
                // TODO receive corrupted packets again
                for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
                    if (packet_number_crc[i] == '0') {
                        while (1) {
                            recvfrom(socket_descriptor, packet_number_crc, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &client_address,(unsigned int*)&len);
                            packet_number = separate_number(packet_number_crc); // separate number
                            crc_received = separate_crc(packet_number_crc); // separate CRC

                            // clear buffers
                            for (int i = 0; i < 20; i++) {
                                packet_number_buffer[i] = '\0';
                                crc_buffer[i] = '\0';
                            }

                            // separate data
                            for (int i = 0; i < BUFFER_SIZE-SUB_BUFFER_SIZE; i++) { buffer[i] = packet_number_crc[i]; }

                            crc_computed =  compute_CRC_buffer(&buffer,BUFFER_SIZE-SUB_BUFFER_SIZE);

                            if (crc_received == crc_computed) {
                                received_packets[(packet_number-1)%MAX_PACKETS_AT_TIME] = 1;
                                send_success(socket_descriptor, client_address);
                            } else {
                                send_fail(socket_descriptor, client_address);
                            }
                        }
                    }
                }

            } else if (packet_number >= packets_at_total ){
                everything_received_mes(socket_descriptor,client_address);
                receiving_file = 0;
            } else {
                sendto(socket_descriptor, packet_number_crc, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
            }


            for (int packet = 0; packet < MAX_PACKETS_AT_TIME; packet++) {
                for (int i = 0; i < BUFFER_SIZE - SUB_BUFFER_SIZE; i++) {
                    if (counter >= message_length) {
                        receiving_file = 0;
                        break;
                    } // reached end of message
                    fputc(mega_buffer[i+packet*(BUFFER_SIZE-SUB_BUFFER_SIZE)], new_file);
                    counter++;
                }
            }
            if (counter >= message_length) {
                break;
            }

        }
        if (counter >= message_length) {  // reached end of message
          receiving_file = 0;
        }
    }
    printf("Data packets received: %ld\n",packet_counter+1);
    printf("Total number of corrupted packets: %ld\n", total_error_count);
    fclose(new_file);
}
