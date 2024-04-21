#include "RUDP_API.h"  // Include the header file for the RUDP API

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define TIMEOUT 1  // Timeout in seconds

// Calculate the checksum of the packet in a simple way
int checksum(RUDP *packet) {
  int sum = 0;
  for (int i = 0; i < 10 && i < 6000; i++) {
    sum += packet->data[i];
  }
  return sum;
}

// Wait for acknowledgment for a packet with sequence number 'seq_num'
int wait_for_ack(int socket, int seq_num, clock_t start_time, int timeout) {
  RUDP *packetReply = malloc(sizeof(RUDP));
  while ((double)(clock() - start_time) / CLOCKS_PER_SEC < timeout) {
    int recvLen = recvfrom(socket, packetReply, sizeof(RUDP) - 1, 0, NULL, 0);
    if (recvLen == -1) {
      free(packetReply);
      return 0;
    }
    if (packetReply->seq_num == seq_num && packetReply->flags.ACK == 1) {
      free(packetReply);
      return 1;
    }
  }
  free(packetReply);
  return 0;
}

// Send acknowledgment for a received packet
int send_ack(int socket, RUDP *packet) {
  RUDP *ack_packet = malloc(sizeof(RUDP));
  memset(ack_packet, 0, sizeof(RUDP));
  ack_packet->flags.ACK = 1;
  if (packet->flags.FIN == 1) {
    ack_packet->flags.FIN = 1;
  }
  if (packet->flags.SYN == 1) {
    ack_packet->flags.SYN = 1;
  }
  if (packet->flags.DATA == 1) {
    ack_packet->flags.DATA = 1;
  }
  ack_packet->seq_num = packet->seq_num;
  ack_packet->checksum = checksum(ack_packet);
  int sendResult = sendto(socket, ack_packet, sizeof(RUDP), 0, NULL, 0);
  if (sendResult == -1) {
    printf("send function failed" );
    free(ack_packet);
    return 0;
  }
  free(ack_packet);
  return 1;
}

// Set timeout for the socket
int set_timeout(int socket, int time) {
  // set timeout for the socket
  struct timeval timeout;
  timeout.tv_sec = time;
  timeout.tv_usec = 0;

  if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) <0) {
    return -1;
  }
  return 1;
}

int seq = 0;  // Sequence number for packets

// Create a socket for RUDP communication
int RUDP_socket() {
  // create socket
  int send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (send_socket == -1) {
    printf("failed create socket");
    return -1;
  }

  return send_socket;
}

// Connect to a remote RUDP server
int RUDP_connect(int socket, char *ip, int port) {
  // setup a timeout for the socket
  if (set_timeout(socket, TIMEOUT) == -1) {
    return -1;
  }
  // server address structure.
  struct sockaddr_in serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  int rval = inet_pton(AF_INET, (const char *)ip, &serverAddress.sin_addr);
  if (rval <= 0) {
    printf("inet faile");
    return -1;
  }

  if (connect(socket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
    printf("connect failed");
    return -1;
  }

  // send SYN message
  RUDP *packet = malloc(sizeof(RUDP));
  memset(packet, 0, sizeof(RUDP));
  packet->flags.SYN = 1;
  int total_tries = 0;
  RUDP *recv_packet = NULL;
  while (total_tries < 3) {
    int send_result = sendto(socket, packet, sizeof(RUDP), 0, NULL, 0);
    if (send_result == -1) {
      printf("send fail");
      free(packet);
      return -1;
    }
    clock_t start_time = clock();
    do {
      // receive SYN-ACK message
      recv_packet = malloc(sizeof(RUDP));
      memset(recv_packet, 0, sizeof(RUDP));
      int recv_result = recvfrom(socket, recv_packet, sizeof(RUDP), 0, NULL, 0);
      if (recv_result == -1) {
        printf("recv function fail");
        free(packet);
        free(recv_packet);
        return -1;
      }
      if (recv_packet->flags.SYN && recv_packet->flags.ACK) {
        printf("connected\n");
        free(packet);
        free(recv_packet);
        return 1;
      } else {
        printf("receive wrong packet ");
      }
    } while ((double)(clock() - start_time) / CLOCKS_PER_SEC < TIMEOUT);
    total_tries++;
  }

  printf("fail in connect to receiver");
  free(packet);
  free(recv_packet);
  return 0;
}

// Get connection for receiving data
int RUDP_get_connection(int socket, int port) {
  // Setup the server address structure.
  struct sockaddr_in serverAddress;
  memset(&serverAddress, 0, sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

  // bind the socket to the server address
  int bindResult =
      bind(socket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
  if (bindResult == -1) {
    printf("bind func failed");
    close(socket);
    return -1;
  }

  // receive SYN message
  struct sockaddr_in clientAddress;
  socklen_t clientAddressLen = sizeof(clientAddress);
  memset((char *)&clientAddress, 0, sizeof(clientAddress));

  RUDP *packet = malloc(sizeof(RUDP));
  memset(packet, 0, sizeof(RUDP));
  int recv_len = recvfrom(socket, packet, sizeof(RUDP) - 1, 0,(struct sockaddr *)&clientAddress, &clientAddressLen);

  if (recv_len == -1) {
    printf("recv from failed ");
    free(packet);
    return -1;
  }

  // make the connection for saving the client address
  if (connect(socket, (struct sockaddr *)&clientAddress, clientAddressLen) ==-1) {
    printf("connect function failed ");
    free(packet);
    return -1;
  }

  // send SYN-ACK message
  if (packet->flags.SYN == 1) {
    RUDP *packetReply = malloc(sizeof(RUDP));
    memset(packet, 0, sizeof(RUDP));
    packetReply->flags.SYN = 1;
    packetReply->flags.ACK = 1;
    int sendResult = sendto(socket, packetReply, sizeof(RUDP), 0, NULL, 0);
    if (sendResult == -1) {
      printf("send function failed ");
      free(packet);
      free(packetReply);
      return -1;
    }
    set_timeout(socket, TIMEOUT * 10);
    free(packet);
    free(packetReply);
    return 1;
  }

  return 0;
}

// Send data over RUDP connection
int RUDP_send(int socket, char *data, int data_length) {
  // calculate the number of packets needed to send the data
  int packets_num = data_length / 6000;
  // calculate the size of the last packet
  int last_packet_size = data_length % 6000;

  RUDP *packet = malloc(sizeof(RUDP));

  // send the packets
  for (int i = 0; i < packets_num; i++) {
    memset(packet, 0, sizeof(RUDP));
    packet->seq_num = i;     // set the sequence number
    packet->flags.DATA = 1;  // set the DATA flag
    // if its the last packet, set the FIN flag
    if (i == packets_num - 1 && last_packet_size == 0) {
      packet->flags.FIN = 1;
    }
    // put the data in the packet
    memcpy(packet->data, data + i * 6000, 6000);
    // set the length of the packet
    packet->length = 6000;
    // calculate the checksum of the packet
    packet->checksum = checksum(packet);
    do {  // send the packet and wait for ack
      int sendResult = sendto(socket, packet, sizeof(RUDP), 0, NULL, 0);
      if (sendResult == -1) {
        printf("send function fail ");
        return -1;
      }
      // wait for ack and retransmit if needed
    } while (wait_for_ack(socket, i, clock(), TIMEOUT) <= 0);
  }
  if (last_packet_size > 0) {
    memset(packet, 0, sizeof(RUDP));
    // change the filed of packet
    packet->seq_num = packets_num;
    packet->flags.DATA = 1;
    packet->flags.FIN = 1;
    memcpy(packet->data, data + packets_num * 6000,
           last_packet_size);
    packet->length = last_packet_size;
    packet->checksum = checksum(packet);
    do {  // send the packet and wait for ack
      int sendResult = sendto(socket, packet, sizeof(RUDP), 0, NULL, 0);
      if (sendResult == -1) {
        printf("sendto function failed ");
        free(packet);
        return -1;
      }
    } while (wait_for_ack(socket, packets_num, clock(), TIMEOUT) <= 0);
    free(packet);
  }
  return 1;
}

// Receive data over RUDP connection
int RUDP_receive(int socket, char **data, int *data_length) {
  RUDP *packet = malloc(sizeof(RUDP));
  memset(packet, 0, sizeof(RUDP));

  int recvLen = recvfrom(socket, packet, sizeof(RUDP) - 1, 0, NULL, 0);
  if (recvLen == -1) {
    printf("recv function failed ");
    free(packet);
    return -1;
  }
  // check if the packet is corrupted, and send ack
  if (checksum(packet) != packet->checksum) {
    free(packet);
    return -1;
  }
  if (send_ack(socket, packet) == -1) {
    free(packet);
    return -1;
  }
  if (packet->flags.SYN == 1) {  // connection request
    printf("received connection request\n");
    free(packet);
    return 0;
  }
  if (packet->seq_num == seq) {
    if (packet->seq_num == 0 && packet->flags.DATA == 1) {
      set_timeout(socket, TIMEOUT * 10);
    }
    if (packet->flags.FIN == 1 && packet->flags.DATA == 1) {  // last packet
      *data = malloc(packet->length);  // Allocate memory for data
      memcpy(*data, packet->data, packet->length);
      *data_length = packet->length;
      free(packet);
      seq = 0;
      set_timeout(socket, 10000000);
      return 2;
    }
    if (packet->flags.DATA == 1) {     // data packet
      *data = malloc(packet->length);  // Allocate memory for data
      memcpy(*data, packet->data, packet->length);
      *data_length = packet->length;
      free(packet);
      seq++;
      return 1;
    }
  } else if (packet->flags.DATA == 1) {
    free(packet);
    return 0;
  }
  if (packet->flags.FIN == 1) {  // close request
    free(packet);
    // send ack and wait for TIMEOUT*10 seconds to check if the sender closed
    printf("received close request\n");
    set_timeout(socket, TIMEOUT * 10);

    packet = malloc(sizeof(RUDP));
    time_t FIN_send_time = time(NULL);
    printf("waiting for close\n");
    while ((double)(time(NULL) - FIN_send_time) < TIMEOUT) {
      memset(packet, 0, sizeof(RUDP));
      recvfrom(socket, packet, sizeof(RUDP) - 1, 0, NULL, 0);
      if (packet->flags.FIN == 1) {
        if (send_ack(socket, packet) == -1) {
          free(packet);
          return -1;
        }
        FIN_send_time = time(NULL);
      }
    }
    free(packet);
    close(socket);
    return -2;
  }
  free(packet);
  return 0;
}

// Close the RUDP connection
int RUDP_close(int socket) {
  RUDP *close_packet = malloc(sizeof(RUDP));
  memset(close_packet, 0, sizeof(RUDP));
  close_packet->flags.FIN = 1;
  close_packet->seq_num = -1;
  close_packet->checksum = checksum(close_packet);
  do {
    int sendResult = sendto(socket, close_packet, sizeof(RUDP), 0, NULL, 0);
    if (sendResult == -1) {
      printf("send function failed" );
      free(close_packet);
      return -1;
    }
  } while (wait_for_ack(socket, -1, clock(), TIMEOUT) <= 0);
  close(socket);
  free(close_packet);
  return 1;
}

