#include "file_operations.h"

static void send_mega_buffer(const unsigned char *mega_buffer, int socket_descriptor, long packets_at_total, struct sockaddr_in server_address, _Bool last_packet_sent) {
    unsigned char packet_buffer[BUFFER_SIZE] = {'\0'};
    int not_sent_packets = 0;
    for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
        for (int j = 0; j < BUFFER_SIZE; j++) {
            packet_buffer[j] = mega_buffer[j + i*BUFFER_SIZE];
        }

        // send buffer with data, CRC and packet number
#ifdef GENERATE_ERRORS
        if (i%16 != 0) {
            sendto(socket_descriptor, packet_buffer, sizeof(unsigned char) * (BUFFER_SIZE), MSG_CONFIRM,(const struct sockaddr *) &server_address, sizeof(server_address));
        } else {
            print_text("  % ERROR_GEN: ",YELLOW,0);
            print_text("discarding packet ( in send_mega_buffer() )\n",0,0);
            not_sent_packets++;
        }
#endif
#ifndef GENERATE_ERRORS
        sendto(socket_descriptor, packet_buffer, sizeof(unsigned char) * (BUFFER_SIZE), MSG_CONFIRM,(const struct sockaddr *) &server_address, sizeof(server_address));
#endif
        if (last_packet_sent && ((i+1)==packets_at_total%MAX_PACKETS_AT_TIME)) {
            break;
        }
    }

    // send confirmation request
#ifdef GENERATE_ERRORS
    print_text("  % ERROR_GEN: ",YELLOW,0);
    print_text("discarding confirmation request ( in send_mega_buffer() )\n",0,0);
#endif
#ifndef GENERATE_ERRORS
    confirmation_request(socket_descriptor,server_address);
#endif
}

// send file to server from client
void send_file(char *file_dest, long n_o_char, int socket_descriptor, struct sockaddr_in server_address, int len) {
    unsigned char sub_buffer[SUB_BUFFER_SIZE] = {'\0'};
    unsigned char data_buffer[BUFFER_SIZE - SUB_BUFFER_SIZE] = {'\0'};
    unsigned char packet_buffer[BUFFER_SIZE] = {'\0'};
    unsigned char mega_buffer[MAX_PACKETS_AT_TIME * BUFFER_SIZE] = {'\0'};

    _Bool server_missing_bools[MAX_PACKETS_AT_TIME] = {0};
    _Bool sending_file = 1;
    _Bool last_packet_sent = 0;

    long pointer = 0;
    long total_error_count = 0;
    long packets_at_total = n_o_char/(BUFFER_SIZE-SUB_BUFFER_SIZE) + 1;

    unsigned long CRC_value;

    int number_of_packets=1;

    FILE *read_file = fopen(file_dest,"rb");

    while(sending_file) {
        // create mega_buffer with all packets prepared to send at once
        for (int packet_n = 0; packet_n < MAX_PACKETS_AT_TIME; packet_n++) {
            // clear buffer
            for (int i = 0; i < BUFFER_SIZE - SUB_BUFFER_SIZE; i++) { data_buffer[i] = '\0'; }

            // fill buffer
            for (int i = 0; i < BUFFER_SIZE - SUB_BUFFER_SIZE && pointer < n_o_char; i++) {
                data_buffer[i] = getc(read_file);
                pointer++;
            }

            // create CRC and packet number sub_buffer
            CRC_value = compute_CRC_buffer(&data_buffer, BUFFER_SIZE - SUB_BUFFER_SIZE);// compute CRC
            sprintf((char *) sub_buffer, "%d %ld", number_of_packets, CRC_value);

            // connect buffers (data + CRC)
            for (int i = 0; i < BUFFER_SIZE - SUB_BUFFER_SIZE; i++) {
                packet_buffer[i] = data_buffer[i];
            }
            for (int i = BUFFER_SIZE - SUB_BUFFER_SIZE; i < BUFFER_SIZE; i++) {
                packet_buffer[i] = sub_buffer[i - (BUFFER_SIZE - SUB_BUFFER_SIZE)];
            }

            for (int i = 0; i < BUFFER_SIZE; i++) {
                mega_buffer[packet_n * BUFFER_SIZE + i] = packet_buffer[i];
            }
            if (number_of_packets >= packets_at_total) {
                last_packet_sent = 1;
                sending_file = 0;
                break;
            }
            number_of_packets++;
        }

        // send all data in mega_buffer and confirmation request
        send_mega_buffer(mega_buffer, socket_descriptor, packets_at_total, server_address, last_packet_sent);

        // receive confirmation or got_ales
        long delta = recvfrom(socket_descriptor, packet_buffer, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &server_address,(unsigned int*)&len);
        int try_num = 0;
        while (delta == -1) {
            if (try_num > MAX_SENT_REPEAT) {
                print_text("  ! MAX_SENT_REPEAT reached\n",RED,0);
                exit(100);
            }
            confirmation_request(socket_descriptor,server_address);
            delta = recvfrom(socket_descriptor, packet_buffer, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &server_address,(unsigned int*)&len);
            try_num++;
        }

        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {server_missing_bools[i] = 0;}
        int server_missing = 0;
        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
            if (packet_buffer[i] == '0') {
                server_missing++;
                server_missing_bools[i] = 1;
            }
        }

        if (server_missing > 0) {
            total_error_count = total_error_count + server_missing;
            char string[60];
            sprintf((char *) string, "  ! server is missing %d packets from last mega_buffer\n", server_missing);
            print_text(string, RED, 0);
        }

        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
            if (server_missing_bools[i]) {
                int try_number = 0;
                while (1) { // send corrupted packets again
                    for (int j = 0; j < BUFFER_SIZE; j++) {
                        packet_buffer[j] = mega_buffer[j + BUFFER_SIZE * i];
                    }
#ifdef GENERATE_ERRORS
                    if (try_number != 0) {
                        sendto(socket_descriptor, packet_buffer, sizeof(unsigned char) * (BUFFER_SIZE), MSG_CONFIRM,(const struct sockaddr *) &server_address, sizeof(server_address));
                    } else {
                        print_text("  % ERROR_GEN: ",YELLOW,0);
                        print_text("discarding message ( in send_file() )\n",0,0);
                    }
#endif
#ifndef GENERATE_ERRORS
                    sendto(socket_descriptor, packet_buffer, sizeof(unsigned char) * (BUFFER_SIZE), MSG_CONFIRM,(const struct sockaddr *) &server_address, sizeof(server_address));
#endif
                    if (get_conf(socket_descriptor, server_address, len, 0)) {
                        break;
                    }
                    try_number++;
                }
            }
        }
    }

    printf("  - data packets sent: %d \n",number_of_packets);
    printf("  - total number of corrupted packets: %ld\n", total_error_count);

    fclose(read_file);
}

// separate packet number from message buffer
long separate_number(const unsigned char *buffer) {
    unsigned char packet_number_buffer[SUB_BUFFER_SIZE/2] = {'\0'};
    char *ptr;
    for (int i = BUFFER_SIZE-SUB_BUFFER_SIZE; i < BUFFER_SIZE; i++) {
        if (buffer[i] != ' ') {
            packet_number_buffer[i-(BUFFER_SIZE-SUB_BUFFER_SIZE)] = buffer[i];
        } else {
            break;
        }
    }
    return strtol((const char *) packet_number_buffer, &ptr, 10);
}

// separate crc from message buffer
long separate_crc(const unsigned char *buffer) {
    unsigned char crc_buffer[SUB_BUFFER_SIZE/2] = {'\0'};
    char *ptr;
    for (int i = BUFFER_SIZE - SUB_BUFFER_SIZE; i < BUFFER_SIZE; i++) {
        if (buffer[i] == ' ') {
            for (int j = i; j < i+SUB_BUFFER_SIZE/2; j++) {
                crc_buffer[j-1-i] = buffer[j];
            }
            break;
        }
    }
    return strtol((const char *) crc_buffer,&ptr,10);
}

// receive file from client
void receive_message(char *file_dest, int socket_descriptor, struct sockaddr_in client_address, int len, long message_length) {
    unsigned char data_buffer[BUFFER_SIZE-SUB_BUFFER_SIZE] = {'\0'};
    unsigned char packet_buffer[BUFFER_SIZE] = {'\0'};
    unsigned char packet_buffer_confirmation[BUFFER_SIZE] = {'\0'};
    unsigned char mega_buffer[(BUFFER_SIZE - SUB_BUFFER_SIZE)*MAX_PACKETS_AT_TIME];

    _Bool received_packets[MAX_PACKETS_AT_TIME];

    long counter = 0;
    long packet_counter = 0;
    long crc_received, packet_number;
    long total_error_count = 0;
    long packets_at_total = message_length/(BUFFER_SIZE-SUB_BUFFER_SIZE)+1;

    unsigned long crc_computed;

    FILE *new_file;
    new_file = fopen(file_dest,"wb");

    int cycle = 0;
    while (1) {
        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) { received_packets[i] = 0; }

        // receive MAX_PACKETS_AT_TIME packets
        for (int i = 0; i < MAX_PACKETS_AT_TIME + 1; i++) {
            // receive packet with data number and CRC
            long lngth = recvfrom(socket_descriptor, packet_buffer, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &client_address,(unsigned int*)&len);
#ifdef NETDERPER
            client_address.sin_port = htons(PORT_NETDERPER_2);
#endif
            if (lngth == 25) {
                send_success(socket_descriptor, client_address);
                i--;
                continue;
            }
            if (packet_is_request(packet_buffer)) {
                break;
            }
            packet_counter++;

            // separate number
            packet_number = separate_number(packet_buffer);
            //printf("received packet %ld\n",packet_number);

            // separate CRC
            crc_received = separate_crc(packet_buffer);

            // separate data
            for (int j = 0; j < BUFFER_SIZE-SUB_BUFFER_SIZE; j++) {
                data_buffer[j] = packet_buffer[j];
            }

            // compute CRC
            crc_computed =  compute_CRC_buffer(&data_buffer,BUFFER_SIZE-SUB_BUFFER_SIZE);

            // check CRC and file number
            if (crc_computed == crc_received) {
                received_packets[(packet_number-1)%MAX_PACKETS_AT_TIME] = 1;
                for (int j = 0; j < BUFFER_SIZE-SUB_BUFFER_SIZE; j++) {
                    mega_buffer[((packet_number-1)%MAX_PACKETS_AT_TIME)*(BUFFER_SIZE-SUB_BUFFER_SIZE) + j] = data_buffer[j];
                }
                //break;
            } else {
                received_packets[(packet_number-1)%MAX_PACKETS_AT_TIME] = 0;
                print_text("  ! CRC do not match\n",RED,0);
            }
        }

        // send everything_received or confirmation
        int not_received = 0;
        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
            if (received_packets[i]) {
                packet_buffer_confirmation[i] = '1';
            } else {
                packet_buffer_confirmation[i] = '0';
                total_error_count++;
                not_received++;
            }
        }

        if (packet_number == packets_at_total) {
            for (int i = (int)packets_at_total % MAX_PACKETS_AT_TIME; i < MAX_PACKETS_AT_TIME; i++) {
                packet_buffer_confirmation[i] = '1';
                received_packets[i] = 1;
                total_error_count--;
                not_received--;
            }
        }

        if (not_received>0) {
            char string[60];
            sprintf(string, "  ! %d packets missing from last mega_buffer\n", not_received);
            print_text(string, RED, 0);
        }

        _Bool everything_ok = 1;
        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
            if (!received_packets[i]) {
                everything_ok = 0;
                break;
            }
        }


        int repaired_packets_num;

        if (!everything_ok) {
            repaired_packets_num = 0;
#ifdef GENERATE_ERRORS
            if (cycle != 0) {
                sendto(socket_descriptor, packet_buffer_confirmation, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
            } else {
                print_text("  % ERROR_GEN: ",YELLOW,0);
                print_text("discarding confirmation\n", 0, 0);
            }
#endif
#ifndef GENERATE_ERRORS
            sendto(socket_descriptor, packet_buffer_confirmation, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
#endif

            for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
                if (!received_packets[i]) {
                    repaired_packets_num++;
                    while (1) {
                        recvfrom(socket_descriptor, packet_buffer, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &client_address,(unsigned int*)&len);
#ifdef NETDERPER
                        client_address.sin_port = htons(PORT_NETDERPER_2);
#endif
                        if (packet_is_request(packet_buffer)) {
                            sendto(socket_descriptor, packet_buffer_confirmation, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
                            continue;
                        }
                        packet_number = separate_number(packet_buffer) - 1; // separate number
                        crc_received = separate_crc(packet_buffer); // separate CRC
                        // separate data
                        for (int j = 0; j < BUFFER_SIZE-SUB_BUFFER_SIZE; j++) { data_buffer[j] = packet_buffer[j]; }

                        crc_computed =  compute_CRC_buffer(&data_buffer,BUFFER_SIZE-SUB_BUFFER_SIZE);

                        if (crc_received == crc_computed) {
                            received_packets[packet_number%MAX_PACKETS_AT_TIME] = 1;
                            send_success(socket_descriptor, client_address);
                            if (i == packet_number%MAX_PACKETS_AT_TIME) {
                                // save to mega_buffer
                                for (int character = 0; character < BUFFER_SIZE-SUB_BUFFER_SIZE; character++) {
                                    mega_buffer[character+i*(BUFFER_SIZE-SUB_BUFFER_SIZE)] = data_buffer[character];
                                }
                            } else {
                                print_text("  ! received wrong repair packet\n",RED,0);
                            }
                            break;
                        } else {
                            send_fail(socket_descriptor, client_address);
                        }
                    }
                }
            }
        } else if (packet_number >= packets_at_total ){
            send_success(socket_descriptor, client_address);
        } else { // send confirmation that all packets were successfully received
            sendto(socket_descriptor, packet_buffer, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
        }

        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
            if (!received_packets[i]) {
                print_text("Packets are still missing!\n",RED,0);
                exit(100);
            }
        }

        if (not_received > 0) {
            print_text("  - missing packets from mega_buffer received successfully\n", 0, 0);
        }

        for (int packet = 0; packet < MAX_PACKETS_AT_TIME; packet++) {
            for (int i = 0; i < BUFFER_SIZE - SUB_BUFFER_SIZE; i++) {
                if (counter >= message_length) {
                   break;
                } // reached end of message
                fputc(mega_buffer[i+packet*(BUFFER_SIZE-SUB_BUFFER_SIZE)], new_file);
                counter++;
            }
        }
        if (counter >= message_length) {
            //everything_received_mes(socket_descriptor,client_address);
            break;
        }
        cycle++;
    }
    print_text("  + all data received\n",GREEN,0);
    printf("  - data packets received: %ld\n",packet_counter+1);
    printf("  - total number of corrupted packets: %ld\n", total_error_count);
    fclose(new_file);
}
