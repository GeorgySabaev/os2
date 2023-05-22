#include "protocol.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// exception? какой exception? только самоделки!
void DieWithError(char *errorMessage);

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr,
            "Usage: %s <Server IP> <Server Port>\n",
            argv[0]);
    exit(1);
  }
  int sock;                               // socket descriptor
  char *servIP = argv[1];                 // server ip (as string)
  unsigned short servPort = atoi(argv[2]);// server port
  // creating a socket
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithError("socket() failed");

  // building the address structure
  struct sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = inet_addr(servIP);
  servAddr.sin_port = htons(servPort);

  // connecting to server
  if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    DieWithError("connect() failed");

  // building the viewing request

  struct command commandHeader;
  commandHeader.type = VIEWER_CONNECT;
  commandHeader.length = 0;
  int sendLen = send(sock, &commandHeader, sizeof(commandHeader), 0);
  if (sendLen != sizeof(commandHeader))
    DieWithError("send() sent a different number of bytes than expected");

  printf("Request sent!\nWaiting for logs...\n\n");
  // receiving data
  for (;;) {
    uint32_t type;
    error_code status;
    struct send_message *message = (struct send_message *) receive_message(sock, &type, &status);
    if (status != STATUS_OK || type != SEND_MESSAGE) {
      DieWithError("error while receiving message");
    }
    printf("%s", message->message);
  }

  close(sock);
  exit(0);
}