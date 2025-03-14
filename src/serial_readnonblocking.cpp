#include "serial_readnonblocking.h"

char CMD[100];
char command[9][100];

void output_serial_data(const char * data){
    strcpy(CMD,data);
    //Serial.println(CMD);
    char delim[] = ",";
    char *ptr;
    int i = 0;
    if(strncmp(CMD, "cmd", 3)== 0) {
      ptr = strtok(CMD, delim);
      while (ptr != NULL) {
        strcpy(command[i],ptr);
        //Serial.println(command[i]);
        ptr = strtok(NULL, delim);
        i++;
      }
    } else {

    }

}

void read_incoming_byte (const byte raw){
  static char input_line [MAX_SERIAL_INPUT];
  static unsigned int input_pos = 0;
  switch (raw)
  {

  case '\n':   // end of text
    input_line [input_pos] = 0;  // terminating null byte

    // terminator reached! process input_line here ...
    output_serial_data (input_line);

    // reset buffer for next time
    input_pos = 0;  
    break;

  case '\r':   // discard carriage return
    break;

  default:
    // keep adding if not full ... allow for terminating null byte
    if (input_pos < (MAX_SERIAL_INPUT - 1))
      input_line [input_pos++] = raw;
    break;

  }  // end of switch
}
