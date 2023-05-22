#include "protocol.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int *viewerSockets;
int viewerCapacity;
int viewerCount;

#define MAXPENDING 5

char string_buffer[2048];

void DieWithError(char *errorMessage);

void RemoveViewer(int i) {
  printf("Viewer removed\n");
  --viewerCount;
  if (i == viewerCount) {
    return;
  }
  viewerSockets[i] = viewerSockets[viewerCount];
}

int TestSocket(int sock) {
  int error = 0;
  socklen_t len = sizeof(error);
  int retval = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);
  if (retval != 0 || error != 0) {
    /* there was a problem getting the error code */
    fprintf(stderr, "Socket not up\n");
    return 0;
  }
  return 1;
}

void Log(char *message) {
  printf("%s", message);
  struct send_message messageHeader;
  messageHeader.messageLen = strlen(message) + 1;
  struct command commandHeader;
  commandHeader.type = SEND_MESSAGE;
  commandHeader.length = sizeof(struct send_message) + messageHeader.messageLen;

  for (int i = 0; i < viewerCount;) {
    if (TestSocket(viewerSockets[i]) == 0) {
      RemoveViewer(i);
      continue;
    }
    int sendLen = send(viewerSockets[i], (char *) &commandHeader, sizeof(commandHeader), 0);
    if (sendLen != sizeof(commandHeader)) {
      RemoveViewer(i);
      continue;
    }
    sendLen = send(viewerSockets[i], (char *) &messageHeader, sizeof(messageHeader), 0);
    if (sendLen != sizeof(messageHeader)) {
      RemoveViewer(i);
      continue;
    }
    sendLen = send(viewerSockets[i], message, messageHeader.messageLen, 0);
    if (sendLen != messageHeader.messageLen) {
      RemoveViewer(i);
      continue;
    }
    ++i;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage:  %s <Server Port> <N>\n", argv[0]);
    exit(1);
  }
  unsigned short servPort = atoi(argv[1]);// server port
  int N = atoi(argv[2]);                  // number of awaited letters

  int clientCount = 0;
  int *clientSockets = (int *) malloc(N * sizeof(int));
  struct send_letter **clientLetters = (struct send_letter **) malloc(N * sizeof(struct send_letter **));
  viewerCapacity = 16;
  viewerCount = 0;
  viewerSockets = (int *) malloc(viewerCapacity * sizeof(int));
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
  sprintf(string_buffer, "\nServer IP address = %s. Waiting for letters...\n\n", inet_ntoa(servAddr.sin_addr));
  Log(string_buffer);
  // listening to connections
  if (listen(servSock, MAXPENDING) < 0)
    DieWithError("listen() failed");
  while (clientCount < N) {
    // waiting for connection
    struct sockaddr_in currClntAddr;
    unsigned int clntLen = sizeof(currClntAddr);
    int currClntSock = accept(servSock, (struct sockaddr *) &currClntAddr, &clntLen);
    uint32_t commandType;
    error_code statusCode;
    char *buffer = receive_message(currClntSock, &commandType, &statusCode);
    if (statusCode == STATUS_ERROR) {
      continue;
    }
    switch (commandType) {
      case SEND_LETTER:
        clientLetters[clientCount] = (struct send_letter *) buffer;
        clientSockets[clientCount] = currClntSock;

        sprintf(string_buffer, "-----begin-connect-----\nReceiving letter %d/%d: %s\nName:", clientCount + 1, N, inet_ntoa(currClntAddr.sin_addr));
        Log(string_buffer);
        Log(clientLetters[clientCount]->name);
        sprintf(string_buffer, "\nWealth: %d\n------end-connect------\n\n", clientLetters[clientCount]->wealth);
        Log(string_buffer);

        ++clientCount;
        break;
      case VIEWER_CONNECT:
        if (viewerCapacity == viewerCount) {
          viewerCapacity *= 2;
          viewerSockets = (int *) realloc(viewerSockets, sizeof(int) * viewerCapacity);
        }
        viewerSockets[viewerCount] = currClntSock;
        Log("Viewer connected.\n");
        ++viewerCount;
        break;
      default:
        break;
    }
  }
  Log("All letters received. Choosing partner...\n");

  // максимально приближенный к реальности алгоритм выбора
  int wealthiest = 0;
  for (int i = 1; i < N; ++i) {
    if (clientLetters[wealthiest]->wealth < clientLetters[i]->wealth) {
      wealthiest = i;
    }
  }
  sprintf(string_buffer, "Partner (%.250s) chosen successfully.\n\n", clientLetters[wealthiest]->name);
  Log(string_buffer);
  Log("Sending replies...\n");
  for (int i = 0; i < N; ++i) {
    char *reply;
    if (i == wealthiest) {
      reply = "OF COURSE!";
    } else {
      reply = "NOT TODAY.";
    }
    if (send(clientSockets[i], reply, REPLY_SIZE, 0) != REPLY_SIZE)
      DieWithError("send() failed");
    sprintf(string_buffer, "\"%s\" sent to %.250s (%d/%d).\n", reply, clientLetters[i]->name, i + 1, N);
    Log(string_buffer);
  }

  Log("\nProcess finished! Terminating...\n\n");

  // cleanup
  for (int i = 0; i < N; ++i) {
    close(clientSockets[i]);
    free(clientLetters[i]);
  }
  free(clientSockets);
  free(clientLetters);
}