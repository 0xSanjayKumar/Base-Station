#ifndef SERIAL_READNONBLOCKING_H
#define SERIAL_READNONBLOCKING_H
#include <Arduino.h>
#define MAX_SERIAL_INPUT 200

void output_serial_data(const char * data);
void read_incoming_byte (const byte raw);

extern char CMD[100];
extern char command[9][100];

#endif
