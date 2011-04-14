#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/////////////////////////////////////////////////////////////////////////////////
// Globals

#define false 0x00
#define true 0x01

typedef char bool;
typedef unsigned int uint;

// file line read buffer
#define DEFAULT_FREAD_BUFF_SIZE 100000
static char freadbuff[DEFAULT_FREAD_BUFF_SIZE] = { 0 };

bool _debug  = false;

FILE* fp1 = NULL;
FILE* fp2 = NULL;


/////////////////////////////////////////////////////////////////////////////////
// Helpers

// debug to stdout on _debug = true
void logdebug(char* str, ...) {
  if (_debug) {
    va_list args;
    va_start(args,str);
    vprintf(str,args);
    va_end(args);
  }
}

/*
 * read_line : 
 *  read a line from FILE*fp and store the file position in fpos,
 *  the line in buff, and the length of the line in linelen.
 */
bool read_line(FILE*fp, fpos_t* fpos, char* buff, short* linelen) {

  bool ret = true;
  char c;
  short index = 0;

  memset(buff, DEFAULT_FREAD_BUFF_SIZE, 0);
  fgetpos(fp, fpos);

  do { 
    c = getc(fp);

    if (c == EOF) {
      buff[index] = 0;
      ret = false;
      break;
    }
    else if (c == '\n') {
      buff[index] = 0;
      break;
    }

    buff[index++] = c;

  } while(true);

  *linelen = index;

  return ret;
}

/*
 * open_files : 
 *  attempt to open two files specified by filename2 and filename2.
 *  assert on errr.
 */
void open_files(char* filename1, char* filename2) {

  fp1 = fopen(filename1, "r");
  assert(fp1 != NULL);
  fp2 = fopen(filename2, "r");
  assert(fp2 != NULL);

  return;
}

/*
 * close_files : 
 */
void close_files() {
  fclose(fp1); 
  fclose(fp2); 
}

#endif GLOBALS_H
