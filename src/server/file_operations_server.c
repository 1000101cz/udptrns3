#include "file_operations_server.h"



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

    char string[80];

    _Bool received_packets[MAX_PACKETS_AT_TIME];

    long counter = 0;
    long packet_counter = 0;
    long crc_received, packet_number;
    long total_error_count = 0;
    long packets_at_total = message_length/(BUFFER_SIZE-SUB_BUFFER_SIZE)+1;

    unsigned long crc_computed;

    FILE *new_file;
    new_file = fopen(file_dest,"wb");

    int highest_received = -1;
    int highest_received_last = -1;

    int cycle = 0;
    int mega_buffer_number = 0;
    while (1) {
        mega_buffer_number++;
        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) { received_packets[i] = 0; }

        // receive MAX_PACKETS_AT_TIME packets
        sprintf(string, "\n  - START mega_buffer %d\n",mega_buffer_number);
        print_text(string,BLUE,0,1);
        for (int i = 0; i < MAX_PACKETS_AT_TIME + 1; i++) {
            // receive packet with data number and CRC
            long lngth = recvfrom(socket_descriptor, packet_buffer, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &client_address,(unsigned int*)&len);
#ifdef NETDERPER
            client_address.sin_port = htons(PORT_NETDERPER_2);
#endif
            if (lngth == 25) {
                print_text("  - didn't receive data or request...\n",GRAY,0,1);
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
            //printf("    - received packet %ld | highest received last %d\n",packet_number,highest_received_last);

            if (i==0 && packet_number == highest_received_last+1 && highest_received_last%MAX_PACKETS_AT_TIME != 0) {    // + _last
                send_success(socket_descriptor, client_address);
                i--;
                continue;
            }


            if(packet_number < highest_received_last) {
                print_text("strange... (packet from previous mega_buffer)\n",RED,0,0);
                send_success(socket_descriptor, client_address);
            }

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
                if (packet_number > highest_received) {
                    highest_received = (int)packet_number;
                }
                received_packets[(packet_number-1)%MAX_PACKETS_AT_TIME] = 1;
                for (int j = 0; j < BUFFER_SIZE-SUB_BUFFER_SIZE; j++) {
                    mega_buffer[((packet_number-1)%MAX_PACKETS_AT_TIME)*(BUFFER_SIZE-SUB_BUFFER_SIZE) + j] = data_buffer[j];
                }
                //break;
            } else {
                received_packets[(packet_number-1)%MAX_PACKETS_AT_TIME] = 0;
                print_text("  ! CRC do not match\n",RED,0,1);
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

        if (highest_received > (packets_at_total-packets_at_total%MAX_PACKETS_AT_TIME)) {
            print_text("    - confirming not existing packages\n",GRAY,0,1);

            for (int i = (int)packets_at_total % MAX_PACKETS_AT_TIME; i < MAX_PACKETS_AT_TIME; i++) {
                packet_buffer_confirmation[i] = '1';
                received_packets[i] = 1;
                total_error_count--;
                not_received--;
            }
        } else {
            print_text("    - not last mega_buffer",GRAY,0,1);
            sprintf(string," ( highest packet: %d | total:%ld )\n",highest_received, packets_at_total);
            print_text(string,GRAY,0,1);
        }

        if (not_received>0) {
            sprintf(string, "    ! %d packets missing from last mega_buffer\n", not_received);
            print_text(string, RED, 0,1);
            for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
                if (!received_packets[i]) {
                    sprintf(string, "    - missing packet %d\n", i);
                    print_text(string, RED, 0,1);
                }
            }
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
            sendto(socket_descriptor, packet_buffer_confirmation, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
            print_text("    - mega_buffer confirmation sent (missing some packets)\n",GRAY,0,1);

            for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
                if (!received_packets[i]) {
                    repaired_packets_num++;
                    while (1) {
                        sprintf(string, "    - waiting for packet %d\n", i);
                        print_text(string, GRAY, 0,1);

                        recvfrom(socket_descriptor, packet_buffer, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &client_address,(unsigned int*)&len);
#ifdef NETDERPER
                        client_address.sin_port = htons(PORT_NETDERPER_2);
#endif
                        if (packet_is_request(packet_buffer)) {
                            print_text("    - resending confirmation\n",GRAY,0,1);
                            sendto(socket_descriptor, packet_buffer_confirmation, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
                            continue;
                        }
                        packet_number = separate_number(packet_buffer) - 1; // separate number
                        sprintf(string,"    - received packet %ld | i %d\n",packet_number,i);
                        print_text(string,0,0,1);
                        crc_received = separate_crc(packet_buffer); // separate CRC
                        // separate data
                        for (int j = 0; j < BUFFER_SIZE-SUB_BUFFER_SIZE; j++) { data_buffer[j] = packet_buffer[j]; }

                        crc_computed =  compute_CRC_buffer(&data_buffer,BUFFER_SIZE-SUB_BUFFER_SIZE);

                        if (crc_received == crc_computed) {
                            received_packets[packet_number%MAX_PACKETS_AT_TIME] = 1;
                            if (i == packet_number%MAX_PACKETS_AT_TIME) {
                                send_success(socket_descriptor, client_address);
                                if (packet_number > highest_received) {
                                    highest_received = (int)packet_number;
                                }
                                // save to mega_buffer
                                for (int character = 0; character < BUFFER_SIZE-SUB_BUFFER_SIZE; character++) {
                                    mega_buffer[character+i*(BUFFER_SIZE-SUB_BUFFER_SIZE)] = data_buffer[character];
                                }
                            } else {
                                print_text("    ! received wrong repair packet\n",RED,0,1);
                                if (packet_number%MAX_PACKETS_AT_TIME < i) {
                                    send_success(socket_descriptor, client_address);
                                }
                                continue;
                            }
                            break;
                        } else {
                            send_fail(socket_descriptor, client_address);
                        }
                    }
                }
            }
        } else { // send confirmation that all packets were successfully received
            sendto(socket_descriptor, packet_buffer_confirmation, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
            print_text("    - packet confirmation sent (all packets successfully received)\n",GRAY,0,1);
        }

        highest_received_last = highest_received;

        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
            if (!received_packets[i]) {
                print_text("Packets are still missing!\n",RED,0,0);
                exit(100);
            }
        }

        if (not_received > 0) {
            print_text("    - missing packets from mega_buffer received successfully\n", GRAY, 0,1);
        }
        sprintf(string, "  - END mega_buffer %d\n",mega_buffer_number);
        print_text(string,BLUE,0,1);

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
            break;
        }
        cycle++;
    }

    print_text("  + all data received\n",GREEN,0,0);

    sprintf(string,"  - data packets received: %ld\n",packet_counter);
    print_text(string,0,0,0);

    sprintf(string,"  - total number of corrupted packets: %ld\n", total_error_count);
    print_text(string,0,0,0);

    fclose(new_file);
}