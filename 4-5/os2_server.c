#include "letterstruct.h"
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <unistd.h>     /* for close() */

#define MAXPENDING 5 /* Maximum outstanding connection requests */

void DieWithError(char *errorMessage); /* Error handling function */

struct letter *HandleTCPClient(int clntSocket) {
  // receiving letter header
  struct letter header;
  char *header_caret = (char *) &header;
  int header_progress;
  int header_size = sizeof(header);

  if ((header_progress = recv(clntSocket, header_caret, header_size, 0)) < 0)
    DieWithError("recv() failed");
  header_caret += header_progress;
  while (header_progress < header_size) {
    int part_size;
    if ((part_size = recv(clntSocket, header_caret, header_size - header_progress, 0)) < 0)
      DieWithError("recv() failed");
    header_caret += part_size;
    header_progress += part_size;
  }

  // creating full letter
  struct letter *letter = (struct letter *) malloc(header_size + header.nameLen);
  memcpy(letter, &header, header_size);

  // receiving sender name
  int name_progress = 0;
  if ((name_progress = recv(clntSocket, letter->name, letter->nameLen, 0)) < 0)
    DieWithError("recv() failed");
  while (name_progress < letter->nameLen) {
    int part_size;
    if ((part_size = recv(clntSocket, &(letter->name[name_progress]), letter->nameLen - name_progress, 0)) < 0)
      DieWithError("recv() failed");
    name_progress += part_size;
  }

  return letter;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage:  %s <Server Port> <N>\n", argv[0]);
    exit(1);
  }

  unsigned short servPort = atoi(argv[1]);// server port
  int N = atoi(argv[2]);                  // number of awaited letters

  int *clientSockets = (int *) malloc(N * sizeof(int));
  struct letter **clientLetters = (struct letter **) malloc(N * sizeof(struct letter **));

  // creating a socket
  int servSock;
  if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithError("socket() failed");

  // building the address structure
  struct sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(servPort);

  // binding the socket
  if (bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    DieWithError("bind() failed");

  printf("\nServer IP address = %s. Waiting for letters...\n\n", inet_ntoa(servAddr.sin_addr));

  // listening to connections
  if (listen(servSock, MAXPENDING) < 0)
    DieWithError("listen() failed");

  int clientCount = 0;

  while (clientCount < N) {
    // waiting for connection
    struct sockaddr_in currClntAddr;
    unsigned int clntLen = sizeof(currClntAddr);
    int currClntSock = accept(servSock, (struct sockaddr *) &currClntAddr, &clntLen);
    if (currClntSock < 0)
      DieWithError("accept() failed");

    // receiving and saving letter
    printf("-----begin-connect-----\nReceiving letter %d/%d: %s\n", clientCount + 1, N, inet_ntoa(currClntAddr.sin_addr));
    clientLetters[clientCount] = HandleTCPClient(currClntSock);
    printf("Name: %s\nWealth: %d\n------end-connect------\n\n",
           clientLetters[clientCount]->name,
           clientLetters[clientCount]->wealth);
    // saving the socket
    clientSockets[clientCount] = currClntSock;
    ++clientCount;
  }
  printf("All letters received. Choosing partner...\n");
  // максимально приближенный к реальности алгоритм выбора
  int wealthiest = 0;
  for (int i = 1; i < N; ++i) {
    if (clientLetters[wealthiest]->wealth < clientLetters[i]->wealth) {
      wealthiest = i;
    }
  }
  printf("Partner (%s) chosen successfully.\n\n", clientLetters[wealthiest]->name);
  printf("Sending replies...\n");
  for (int i = 0; i < N; ++i) {
    char *reply;
    if (i == wealthiest) {
      reply = "OF COURSE!";
    } else {
      reply = "NOT TODAY.";
    }
    if (send(clientSockets[i], reply, REPLY_SIZE, 0) != REPLY_SIZE)
      DieWithError("send() failed");
    printf("\"%s\" sent to %s (%d/%d).\n", reply, clientLetters[i]->name, i, N);
  }

  printf("\nProcess finished! Terminating...\n\n");
  // cleanup
  for (int i = 0; i < N; ++i) {
    close(clientSockets[i]);
    free(clientLetters[i]);
  }
  free(clientSockets);
  free(clientLetters);
}
