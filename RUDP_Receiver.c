#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "RUDP_API.h"

int main(int argc, char* argv[]) {
  // parse the command line arguments
  if (argc != 3 || strcmp(argv[1], "-p") != 0) {
    printf("faild- wrong argument pass to reciver");
  }
  int port = atoi(argv[2]);
  printf("RUDP Receiver \n");
  int sock= RUDP_socket();
  if (sock== -1) {
    printf("failed create socket:(\n");
    return -1;
  }
  printf("socket created:)\n");

  int val = RUDP_get_connection(sock, port);
  if (val == 0) {
    printf("Could not get connection\n");
    return -1;
  }
  printf("Connection established\n");
  FILE* file = fopen("my_file", "w+");
  if (file == NULL) {
    printf("failed opening  my_file file!\n");
    return 1;
  }
  fprintf(file, "\n\n  The statistic is \n");
  double avr_time = 0;
  double avr_speed = 0;
  clock_t start, end;

  char* date_received = NULL;
  int data_length = 0;
  char total_date[2097152] = {0};  // 2MB
  val = 0;
  int run = 1;

  start = clock();
  end = clock();

  do {
    val = RUDP_receive(sock, &date_received, &data_length);

    if (val == -2) {  
      break;
    }
    if (val == -1) {
      printf("faild receive data:(\n");
      return -1;
    }

    if (val == 1 && start < end) {
      start = clock();
    }
    
    if (val == 1) {
      strcat(total_date, date_received);
    }
   
    if (val == 2) {
      strcat(total_date, date_received);
      end = clock();
      double pass_time = ((double)(end - start)) / CLOCKS_PER_SEC;
      avr_time += pass_time;
      double speed = 2 / pass_time;
      avr_speed += speed;
      fprintf(file, "Run %d- Time=%f S , Speed=%f MB/S\n", run, pass_time, speed);
      memset(total_date, 0, sizeof(total_date));
      run++;
    }

  } while (val >= 0); 

  printf("connection closed\n");
  fprintf(file, "\nAverage time: %f S\n", avr_time / (run - 1));
  fprintf(file, "Average speed: %f MB/S\n", avr_speed / (run - 1));

  rewind(file);
  char print_buffer[100];
  while (fgets(print_buffer, 100, file) != NULL) {
    printf("%s", print_buffer);
  }

  // Close the file
  fclose(file);
  remove("my_file");

  return 0;
}
	

