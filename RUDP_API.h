
typedef struct RUDP_flags {
  unsigned int SYN : 1;
  unsigned int ACK : 1;
  unsigned int DATA : 1;
  unsigned int FIN : 1;
} RUDP_flags;

typedef struct _RUDP {
  RUDP_flags flags;
  int seq_num;
  int checksum;
  int length;  // the length of the data
  char data[6000];
} RUDP;

/*Creating a RUDP socket the socket number if success, -1 if failed.*/
int RUDP_socket();

/* opening a connection between two sides 1 is success, 0 if failed. */
int RUDP_connect(int socket, char *ip, int port);

/* 1 is success, 0 if failed. */
int RUDP_get_connection(int socket, int port);

/*Sending data 1 is success, 0 if failed.*/
int RUDP_send(int socket, char *data, int data_length);

/*Receiving data 1 is data, 0 if nondata, 2 if last data packet, -2 if close connection, -1 is failed.*/
int RUDP_receive(int socket, char **data, int *data_length);

/*Closing the RUDP socket.*/
int RUDP_close(int socket);
	
