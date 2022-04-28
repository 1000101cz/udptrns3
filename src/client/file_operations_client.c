#include "file_operations_client.h"

static void send_mega_buffer(const unsigned char *mega_buffer, int socket_descriptor, long packets_at_total, struct sockaddr_in server_address, _Bool last_packet_sent) {
    unsigned char packet_buffer[BUFFER_SIZE] = {'\0'};
    for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
        for (int j = 0; j < BUFFER_SIZE; j++) {
            packet_buffer[j] = mega_buffer[j + i*BUFFER_SIZE];
        }

        // send buffer with data, CRC and packet number
#ifdef NETDERPER
        server_address.sin_port = htons(PORT_NETDERPER_1);
#endif
        sendto(socket_descriptor, packet_buffer, sizeof(unsigned char) * (BUFFER_SIZE), MSG_CONFIRM,(const struct sockaddr *) &server_address, sizeof(server_address));
        if (last_packet_sent && ((i+1)==packets_at_total%MAX_PACKETS_AT_TIME)) {
            break;
        }
    }
    print_text("    - mega_buffer sent\n",GRAY,0,1);

    // send confirmation request
    confirmation_request(socket_descriptor,server_address);
}

// send file to server from client
void send_file(char *file_dest, long n_o_char, int socket_descriptor, struct sockaddr_in server_address, int len) {
    unsigned char sub_buffer[SUB_BUFFER_SIZE] = {'\0'};
    unsigned char data_buffer[BUFFER_SIZE - SUB_BUFFER_SIZE] = {'\0'};
    unsigned char packet_buffer[BUFFER_SIZE] = {'\0'};
    unsigned char mega_buffer[MAX_PACKETS_AT_TIME * BUFFER_SIZE] = {'\0'};

    char string[60];

    _Bool server_missing_bools[MAX_PACKETS_AT_TIME] = {0};
    _Bool sending_file = 1;
    _Bool last_packet_sent = 0;

    long pointer = 0;
    long total_error_count = 0;
    long packets_at_total = n_o_char/(BUFFER_SIZE-SUB_BUFFER_SIZE) + 1;

    unsigned long CRC_value;

    int number_of_packets=1;

    FILE *read_file = fopen(file_dest,"rb");

    int mega_buffer_number = 0;
    while(sending_file) {
        mega_buffer_number++;
        // create mega_buffer with all packets prepared to send at once
        sprintf(string, "\n  - START mega_buffer %d\n",mega_buffer_number);
        print_text(string,BLUE,0,1);
        for (int packet_n = 0; packet_n < MAX_PACKETS_AT_TIME; packet_n++) {
            // clear buffer
            for (int i = 0; i < BUFFER_SIZE - SUB_BUFFER_SIZE; i++) { data_buffer[i] = '\0'; }

            // fill buffer
            for (int i = 0; i < BUFFER_SIZE - SUB_BUFFER_SIZE && pointer < n_o_char; i++) {
                data_buffer[i] = getc(read_file);
                pointer++;
            }

            // create CRC and packet number sub_buffer
            // TODO - include Packet number to CRC
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
                print_text("    ! MAX_SENT_REPEAT reached\n",RED,0,0);
                exit(100);
            }
            confirmation_request(socket_descriptor,server_address);
            delta = recvfrom(socket_descriptor, packet_buffer, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &server_address,(unsigned int*)&len);
            try_num++;
        }

        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {server_missing_bools[i] = 1;}
        int server_missing = MAX_PACKETS_AT_TIME;
        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
            if (packet_buffer[i] == '1') {
                server_missing--;
                server_missing_bools[i] = 0;
            } else {
                sprintf(string,"    ! server missing packet %d\n",i);
                print_text(string,RED,0,1);
            }
        }

        if (server_missing > 0) {
            total_error_count = total_error_count + server_missing;
            sprintf((char *) string, "    ! server is missing %d packets from last mega_buffer\n", server_missing);
            print_text(string, RED, 0,1);
        } else {
            print_text("    - server confirmed all packets from last mega_buffer\n",GRAY,0,1);
        }

        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
            if (server_missing_bools[i]) {
                int try_number = 0;
                while (1) { // send corrupted packets again
                    for (int j = 0; j < BUFFER_SIZE; j++) {
                        packet_buffer[j] = mega_buffer[j + BUFFER_SIZE * i];
                    }
                    sprintf((char *) string, "    - sending packet %d\n", i);
                    print_text(string, GRAY, 0,1);

#ifdef NETDERPER
                    server_address.sin_port = htons(PORT_NETDERPER_1);
#endif
                    sendto(socket_descriptor, packet_buffer, sizeof(unsigned char) * (BUFFER_SIZE), MSG_CONFIRM,(const struct sockaddr *) &server_address, sizeof(server_address));
                    if (get_conf(socket_descriptor, server_address, len, 0)) {
                        break;
                    }
                    try_number++;
                }
            }
        }
        sprintf((char *) string, "  - END mega_buffer %d\n", mega_buffer_number);
        print_text(string, BLUE, 0,1);
    }

    printf("  - data packets sent: %d \n",number_of_packets);
    printf("  - total number of corrupted packets: %ld\n", total_error_count);

    fclose(read_file);
}