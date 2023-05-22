#include <stdlib.h>
#include <stdint.h>

struct command {
  uint32_t length;
  uint32_t type;
};

#define SEND_LETTER 1
#define VIEWER_CONNECT 2
#define SEND_MESSAGE 3

typedef unsigned char error_code;

struct send_letter {
  int wealth;
  int nameLen;
  char name[0];
};

struct send_message {
  int messageLen;
  char message[0];
};

#define STATUS_OK 0
#define STATUS_ERROR 1

char *receive_message(int socketDesc, uint32_t *type, unsigned char *status_code);

#define MAX_NAME_LENGTH 250
#define REPLY_SIZE 10