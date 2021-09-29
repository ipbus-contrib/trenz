#ifndef CMD_BUFFER_H
#define CMD_BUFFER_H

#ifndef MAX_CMD_LENGTH
#define MAX_CMD_LENGTH 16
#endif

#ifndef MAX_N
#define MAX_N 16
#endif

uint8_t buffer[MAX_N];
char command[MAX_CMD_LENGTH];
char chan[2];
#endif
