#include "protocol.h"
#include <sys/socket.h>
#include <stdio.h>

error_code receive_data(int socketDesc, char *buffer, size_t size) {
  int buffer_progress;

  if ((buffer_progress = recv(socketDesc, buffer, size, 0)) < 0) {
    return STATUS_ERROR;
  }
  buffer += buffer_progress;
  while (buffer_progress < size) {
    int part_size;
    if ((part_size = recv(socketDesc, buffer, size - buffer_progress, 0)) <= 0) {
      return STATUS_ERROR;
    }
    buffer += part_size;
    buffer_progress += part_size;
  }
  return STATUS_OK;
}

char *receive_message(int socketDesc, uint32_t *type, error_code *status_code) {
  struct command commandHeader;
  *status_code = receive_data(socketDesc, (char *) &commandHeader, sizeof(commandHeader));
  if (*status_code != STATUS_OK) {
    return NULL;
  }
  *type = commandHeader.type;
  if (commandHeader.length == 0) {
    return NULL;
  }
  char *contents = (char *) malloc(commandHeader.length);
  *status_code = receive_data(socketDesc, contents, commandHeader.length);
  if (*status_code != STATUS_OK) {
    free(contents);
    return NULL;
  }
  return contents;
}