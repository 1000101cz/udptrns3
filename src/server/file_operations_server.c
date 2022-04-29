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
void receive_file(char *file_dest, int socket_descriptor, struct sockaddr_in client_address, int len, long message_length) {
    unsigned char sub_buffer[SUB_BUFFER_SIZE] = {'\0'};
    unsigned char data_buffer[BUFFER_SIZE-SUB_BUFFER_SIZE] = {'\0'};
    unsigned char packet_buffer[BUFFER_SIZE] = {'\0'};
    unsigned char data_number_buffer[BUFFER_SIZE - SUB_BUFFER_SIZE/2] = {'\0'};
    unsigned char packet_buffer_confirmation[BUFFER_SIZE] = {'\0'};
    unsigned char mega_buffer[(BUFFER_SIZE - SUB_BUFFER_SIZE)*MAX_PACKETS_AT_TIME];

    char string[80];

    _Bool received_packets[MAX_PACKETS_AT_TIME];

    long counter = 0;
    long packet_counter = 0;
    long crc_received;
    long total_error_count = 0;
    long packets_at_total = message_length/(BUFFER_SIZE-SUB_BUFFER_SIZE)+1;

    unsigned long crc_computed;

    FILE *new_file;
    new_file = fopen(file_dest,"wb");

    int highest_received = -1;
    int actual_min;
    int actual_max;
    int packet_number;

    int cycle = 0;
    int mega_buffer_number = 0;
    while (1) {
        mega_buffer_number++;
        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) { received_packets[i] = 0; }

        // receive MAX_PACKETS_AT_TIME packets
        sprintf(string, "\n  - START mega_buffer %d\n",mega_buffer_number);
        print_text(string,BLUE,0,1);

        actual_min = cycle*MAX_PACKETS_AT_TIME+1;
        actual_max = (cycle+1)*MAX_PACKETS_AT_TIME;
        printf("    - expecting packets in range <%d ; %d>\n",actual_min,actual_max);

        // receive mega_buffer
        for (int i = 0; i < MAX_PACKETS_AT_TIME + 1; i++) {

            for (int x = 0; x < BUFFER_SIZE; x++) { packet_buffer[x] = '\0'; }
            for (int x = 0; x < SUB_BUFFER_SIZE; x++) { sub_buffer[x] = '\0'; }
            for (int x = 0; x < BUFFER_SIZE-SUB_BUFFER_SIZE/2; x++) { data_number_buffer[x] = '\0'; }

            // receive packet with data number and CRC
            long length = recvfrom(socket_descriptor, packet_buffer, sizeof(unsigned char)*(BUFFER_SIZE),MSG_WAITALL, ( struct sockaddr *) &client_address,(unsigned int*)&len);
#ifdef NETDERPER
            client_address.sin_port = htons(PORT_NETDERPER_2);
#endif
            printf("    - received message length: %ld\n",length);
            if (length == 25) {
                print_text("  - didn't receive data or request...\n",GRAY,0,1);
                send_success(socket_descriptor, client_address);
                i--;
                continue;
            }
            if (packet_is_request(packet_buffer)) {
                print_text("  - received request, breaking mega_buffer cycle\n",0,0,1);
                break;
            }
            packet_counter++;

            // separate number
            packet_number = (int)separate_number(packet_buffer);


            if (packet_number < actual_min || packet_number > actual_max) {   // <
                print_text("    ! (packet from different mega_buffer)\n",RED,0,0);
                send_success(socket_descriptor, client_address);
            }

            // separate CRC
            crc_received = separate_crc(packet_buffer);

            // separate data
            for (int j = 0; j < BUFFER_SIZE-SUB_BUFFER_SIZE; j++) {
                data_buffer[j] = packet_buffer[j];
                data_number_buffer[j] = data_buffer[j];  // new
            }

            // compute CRC
            // TODO - include Packet number to CRC
            // old
            //crc_computed =  compute_CRC_buffer(&data_buffer,BUFFER_SIZE-SUB_BUFFER_SIZE);
            // new
            sprintf((char*)sub_buffer,"%d",packet_number);
            for (int k = BUFFER_SIZE-SUB_BUFFER_SIZE; k < BUFFER_SIZE-SUB_BUFFER_SIZE/2;k++) {
                data_number_buffer[k] = sub_buffer[k-(BUFFER_SIZE-SUB_BUFFER_SIZE)];
            }
            crc_computed =  compute_CRC_buffer(&data_number_buffer,BUFFER_SIZE-SUB_BUFFER_SIZE/2);

            // check CRC and file number
            if (crc_computed == crc_received) {
                printf("    - received packet %d\n",packet_number);
                received_packets[(packet_number-1)%MAX_PACKETS_AT_TIME] = 1;
                if (packet_number > highest_received) {
                    highest_received = (int)packet_number;
                }
                for (int j = 0; j < BUFFER_SIZE-SUB_BUFFER_SIZE; j++) {
                    mega_buffer[((packet_number-1)%MAX_PACKETS_AT_TIME)*(BUFFER_SIZE-SUB_BUFFER_SIZE) + j] = data_buffer[j];
                }
            } else {
                print_text("    ! CRC do not match\n",RED,0,1);
            }
        }
        print_text("  - end receiving standard packets\n",0,0,1);

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
            sprintf(string, "    ! %d packets missing from mega_buffer\n", not_received);
            print_text(string, RED, 0,1);
            for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
                if (!received_packets[i]) {
                    sprintf(string, "    - missing packet %d\n", i+cycle*MAX_PACKETS_AT_TIME);
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

        // receive missing packets
        if (!everything_ok) {
            repaired_packets_num = 0;
            sendto(socket_descriptor, packet_buffer_confirmation, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
            print_text("    - mega_buffer confirmation sent (missing some packets)\n",GRAY,0,1);
            print_text("\n    - receiving repair packets\n",BLUE,0,1);
            for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
                if (!received_packets[i]) {
                    repaired_packets_num++;
                    while (1) {
                        sprintf(string, "    - waiting for packet %d\n", i+cycle*MAX_PACKETS_AT_TIME);
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
                        packet_number = (int)separate_number(packet_buffer) - 1; // separate number
                        sprintf(string,"    - received packet %d\n",packet_number);
                        print_text(string,0,0,1);
                        crc_received = separate_crc(packet_buffer); // separate CRC
                        for (int x = 0; x < BUFFER_SIZE-SUB_BUFFER_SIZE/2; x++) { data_number_buffer[x] = '\0'; }
                        // separate data
                        for (int j = 0; j < BUFFER_SIZE-SUB_BUFFER_SIZE; j++) {
                            data_buffer[j] = packet_buffer[j];
                            data_number_buffer[j] = data_buffer[j];
                        }

                        // old
                        //crc_computed =  compute_CRC_buffer(&data_buffer,BUFFER_SIZE-SUB_BUFFER_SIZE);
                        // new
                        for (int x = 0; x < SUB_BUFFER_SIZE; x++) { sub_buffer[x] = '\0'; }
                        sprintf((char*)sub_buffer,"%d",packet_number+1);
                        for (int x = BUFFER_SIZE-SUB_BUFFER_SIZE; x < BUFFER_SIZE+SUB_BUFFER_SIZE/2; x++) {
                            data_number_buffer[x] = sub_buffer[x-(BUFFER_SIZE-SUB_BUFFER_SIZE)];
                        }
                        crc_computed =  compute_CRC_buffer(&data_number_buffer,BUFFER_SIZE-SUB_BUFFER_SIZE/2);

                        if (crc_received == crc_computed) {
                            received_packets[packet_number%MAX_PACKETS_AT_TIME] = 1;
                            if (i+cycle*MAX_PACKETS_AT_TIME == packet_number) {
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
                                } else if (packet_number%MAX_PACKETS_AT_TIME > highest_received) { // previous confirmation mistake
                                    send_success(socket_descriptor, client_address);
                                    if (packet_number > highest_received) {
                                        highest_received = (int)packet_number;
                                    }
                                    // save to mega_buffer
                                    for (int character = 0; character < BUFFER_SIZE-SUB_BUFFER_SIZE; character++) {
                                        mega_buffer[character+i*(BUFFER_SIZE-SUB_BUFFER_SIZE)] = data_buffer[character];
                                    }
                                }
                                continue;
                            }
                            break;
                        } else {
                            print_text("    ! crc do not match",RED,0,1);
                            printf(" (computed: %ld vs received: %ld)\n",crc_computed,crc_received);
                            send_fail(socket_descriptor, client_address);
                        }
                    }
                }
            }
        } else { // send confirmation that all packets were successfully received
            sendto(socket_descriptor, packet_buffer_confirmation, sizeof(unsigned char)*BUFFER_SIZE,MSG_CONFIRM, (const struct sockaddr *) &client_address,sizeof(client_address));
            print_text("    - packet confirmation sent (all packets successfully received)\n",GRAY,0,1);
        }


        for (int i = 0; i < MAX_PACKETS_AT_TIME; i++) {
            if (!received_packets[i]) {
                print_text("Packets are still missing!\n",RED,0,0);
                exit(100);
            }
        }

        if (not_received > 0) {
            print_text("    - missing packets from mega_buffer received successfully\n\n", BLUE, 0,1);
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