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
  if (argc != 5) {
    fprintf(stderr,
            "Usage: %s <Server IP> <Server Port> <Name> <Wealth>\n",
            argv[0]);
    exit(1);
  }
  int sock;                               // socket descriptor
  char *servIP = argv[1];                 // server ip (as string)
  unsigned short servPort = atoi(argv[2]);// server port
  char *name = argv[3];                   // name of the letter sender
  int wealth = atoi(argv[4]);             // wealth of the letter sender

  // creating a socket
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithError("socket() failed");

  // building the address structure
  struct sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = inet_addr(servIP);
  servAddr.sin_port = htons(servPort);

  // connect to server
  if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    DieWithError("connect() failed");

  int nameLen = strlen(name) + 1;

  // building the letter
  // wealth + name length + name

  struct command commandHeader;
  commandHeader.type = SEND_LETTER;
  commandHeader.length = sizeof(struct send_letter) + nameLen;
  struct send_letter letterHeader;

  letterHeader.wealth = wealth;
  letterHeader.nameLen = nameLen;

  int sendLen = send(sock, &commandHeader, sizeof(commandHeader), 0);
  if (sendLen != sizeof(commandHeader))
    DieWithError("send() sent a different number of bytes than expected");
  sendLen = send(sock, &letterHeader, sizeof(letterHeader), 0);
  if (sendLen != sizeof(letterHeader))
    DieWithError("send() sent a different number of bytes than expected");
  sendLen = send(sock, name, nameLen, 0);
  if (sendLen != nameLen)
    DieWithError("send() sent a different number of bytes than expected");

  printf("\nName: %s\nWealth: %d\n\n", name, wealth);

  printf("Letter sent!\n\nWaiting for reply...\n");

  // receive and display a reply
  int bytesRcvd, totalBytesRcvd = 0;
  char replyBuffer[REPLY_SIZE];
  printf("Reply received: \"");
  while (totalBytesRcvd < REPLY_SIZE) {
    if ((bytesRcvd = recv(sock, replyBuffer, REPLY_SIZE, 0)) <= 0)
      DieWithError("recv() failed or connection closed prematurely");
    totalBytesRcvd += bytesRcvd;
    replyBuffer[bytesRcvd] = '\0';
    printf("%s", replyBuffer);
  }
  printf("\".\n");

  close(sock);
  exit(0);
}