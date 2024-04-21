#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "RUDP_API.h"

//sizeof 2 MB= 1024*1024*2= 2097152

char *util_generate_random_data(unsigned int size) {
  char *buffer = NULL;
  // Argument check.
  if (size == 0) return NULL;
  buffer = (char *)calloc(size, sizeof(char));
  // Error checking.
  if (buffer == NULL) return NULL;
  // Randomize the seed of the random number generator.
  srand(time(NULL));
  for (unsigned int i = 0; i < size; i++)
    *(buffer + i) = ((unsigned int)rand() % 255) + 1;
  return buffer;
}

int main(int argc, char *argv[]) {
  // Parse the command line arguments
  char *ip;
  int port;
   if (argc != 5 ||  (strcmp(argv[1], "-ip") != 0 && strcmp(argv[3], "-p") != 0)) {
    printf("Invalid arguments pass to sender");
    return 0;
  }
  ip = argv[2];
  port = atoi(argv[4]);

  // Generate the file data
  char *file_data = util_generate_random_data(2097152);

  // Create a RUDP socket between the sender and the receiver
  int sock = RUDP_socket();
  if (sock == -1) {
    return -1;
  }
  if (RUDP_connect(sock, ip, port) <1) {
    return -1;
  }
  if (sock == -1) {
    return 1;
  }

  int c;
  do {
    printf("Send file\n");
    if (RUDP_send(sock, file_data, 2097152) < 0) {
      printf("Error when try to send the file\n");
      RUDP_close(sock);
      return 1;
    }
    printf("Do you want to send the file again? choose 1 or 0: ");
    scanf("%d", &c);
  } while (c == 1);

  printf("Closing the connection\n");
  RUDP_close(sock);
  printf("Connection closed\n");
  free(file_data);
  return 0;
}











	
