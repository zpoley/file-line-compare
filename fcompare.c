#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/////////////////////////////////////////////////////////////////////////////////
// Global Defines

#define false 0x00
#define true 0x01

typedef char bool;
typedef unsigned int uint;

// hash bucket linked list node.
typedef struct HBNode {
  fpos_t fpos;
  short linelen;
  uint bloomhash; // k combined bloom hashes
  struct HBNode *next;
} HBNode;

// hash bucket linked list head node.
typedef struct HBHeadNode {
  short size; // ll size for comparisons
  HBNode this;
} HBHeadNode;

// file line read buffer
#define FREAD_BUFF_SIZE 100000
char freadbuff[FREAD_BUFF_SIZE] = {}; 

// bloom filter num buckets
#define BLOOM_SIZE 1500000  // 2000000
char bloom_bits[BLOOM_SIZE / 8] = { 0 };

// hash table num buckets
#define HASH_SIZE BLOOM_SIZE / 5 
HBHeadNode* hash_buckets[HASH_SIZE] = { 0 };

bool _debug  = false;

/////////////////////////////////////////////////////////////////////////////////
// Helper methods

// debug to stdout on _debug = true
void logdebug(char* str, ...) {
  if (_debug) {
    va_list args;
    va_start(args,str);
    vprintf(str,args);
    va_end(args);
  }
}

// open and return file ptr
FILE* open_file(char* filename) {
  FILE* fp = fopen(filename, "r");
  return fp;
}

// close file ptr
void close_file(FILE* fp) {
  fclose(fp);
  return;
}

// debug bloom filter bits
void print_bloom_bits() {
  int i = 0, j = 0;
  printf("\nbloom bits:\n");
  for(; (i < ((BLOOM_SIZE / 8))); i++) {
    short and1 = 1;
    for(j = 0; (j < 8); j++) {
      printf("%d", ((bloom_bits[i] & (and1 << j)) > 0));
    }
  }
  printf("\n\n");
}

// debug hash table buckets
void print_hash_buckets() {
  short i = 0;
  for(i = 0; (i < HASH_SIZE); i++) {
    printf("b: %d", i);
    if (hash_buckets[i]) {
      HBNode* curr = &hash_buckets[i]->this;
      do {
        printf(" %ld", curr->fpos);
        curr = curr->next;
      }
      while(curr);
    }
    else { printf(" ___"); }
    printf("\n");
  }
}

// count file lines (as optimization to block init and manage memory
//  (currently unused)
uint count_file_lines(FILE* fp) {
  uint lines = 0;
  do { 
    char c = getc(fp);
    if (c == EOF)
      break;
    else if (c == '\n')
      lines++;
  } while(true);
  rewind(fp);
  return lines;
}

// add a new hash bucket node
void insert_hbnode(HBHeadNode* head, HBNode* chain, HBNode* bbn) {
  HBNode* curr = chain;

  while(curr->next && (bbn->fpos > curr->fpos)) {
    curr = curr->next;
  }
  if (curr->next) {
    bbn->next = curr->next;
    curr->next = bbn;
  }
  else {
    curr->next = bbn;
  }

  head->size++;
}

// set 4 bloom bits identified by quad_hash indices
void set_bloom_bits(uint* quad_hash) {
  short i = 0; 
  for(; (i < 4); i++) { 
    uint h = quad_hash[i];
    char and1 = 1;
    bloom_bits[(h / 8)] |= (and1 << (h % 8));
  }
}

// test quad_hash against bloom_bits
//  return true if all 4 bits are set, false otherwise 
bool find_in_bloom_bits(uint* quad_hash) {
  short i = 0; 
  for(; (i < 4); i++) { 
    uint h = quad_hash[i];
    char and1 = 1;
    if (!(bloom_bits[(h / 8)] & (and1 << (h % 8)))) {
      return false;
    }
  }
  return true;
}

// final step, memcmp (linelens match)
bool compare_line_to_buff(
  FILE*fp, fpos_t* new_fpos, 
  short linelen, char* buff) {

  fpos_t curr_fpos;
  fgetpos(fp, &curr_fpos);

  // seek to file location of start of string
  fseek(fp, new_fpos, SEEK_SET);

  char filebuff[FREAD_BUFF_SIZE] = { 0 };

  // read the line from the file
  fread((void*) filebuff, sizeof(char), linelen, fp);

  short i = 0;
  for(; (i < linelen); i++) {
    if (buff[i] != filebuff[i]) {
      // return false as soon as one wrong byte found
      return false;
    }
  }

  return true;
}

// return true if exact match is found in hash table
bool find_in_hash_table(
    FILE* fp, char* buff, short linelen, uint* quad_hash, uint ht_hash) {

  rewind(fp);
  short i = 0;
  uint bb_intersection = 0;
  for(;(i < 4); i++) { bb_intersection |= quad_hash[i]; }

  // run through hash bucket chain
  if (hash_buckets[ht_hash]) {
    HBNode* curr = &hash_buckets[ht_hash]->this;
    do {
      // if both linelen and bloom intersection match, 
      //  only then attempt full string comparison
      if ((curr->linelen == linelen) && (curr->bloomhash == bb_intersection)) {
        // highly probably match, full str compare from file
        if (compare_line_to_buff(fp, curr->fpos, curr->linelen, buff)) {
          // only if compare is true, otherwise test more bucket nodes
          return true;
        }
      }
      curr = curr->next;
    }
    while(curr);
  }
  return false;
}

// add line info to hash table
void hash_line_info(
    fpos_t* fpos, short linelen, uint* quad_hash, uint ht_hash) {

  short i = 0;
  set_bloom_bits(quad_hash);
  uint bb_intersection = 0;

  for(;(i < 4); i++) { bb_intersection |= quad_hash[i]; }

  if (hash_buckets[ht_hash]) {
    // append to hash bucket
    HBNode *hbn =  (HBNode*)malloc(sizeof(HBNode));
    memcpy(&hbn->fpos, fpos, sizeof(fpos_t));
    hbn->linelen = linelen;
    hbn->bloomhash = bb_intersection;
    hbn->next = NULL;
    insert_hbnode(hash_buckets[ht_hash], &hash_buckets[ht_hash]->this, hbn);
  }
  else {
    // create new head bucket node
    HBHeadNode* hbh = hash_buckets[ht_hash] = (HBHeadNode*)malloc(sizeof(HBHeadNode));
    hbh->size = 1;
    memcpy(&hbh->this.fpos, fpos, sizeof(fpos_t));
    hbh->this.linelen = linelen;
    hbh->this.bloomhash = bb_intersection;
    hbh->this.next = NULL;
    hash_buckets[ht_hash] = hbh;
  }
  return;
}

// read a line from file and generate bloom quad_hash and hash table hash
bool read_and_hash_line(
    FILE*fp, fpos_t* fpos, char* buff, 
    short* linelen, uint* quad_hash, uint* ht_hash) {

  bool ret = true;
  char c;
  short i = 0, index = 0;

  // init buff
  memset(buff, FREAD_BUFF_SIZE, 0);
  fgetpos(fp, fpos);

  for(; (i < 4); i++) { quad_hash[i] = 0; }
  *ht_hash = 0;

  do { 
    c = getc(fp);

    if (c == EOF) {
      buff[index] = 0;
      ret = false;
      break;
    }
    if (c == '\n') {
      buff[index] = 0;
      break;
    }

    // incr bloom hash functions
    for(i = 0; (i < 4); i++) { 
      quad_hash[i] += ((((c * 7) + index + i) * (index + 1 + i) + i) << (i + c + 1)) + (i + c + 1) ;
    }
    // incr hash table function
    *ht_hash += ((((c * 7) + index + 5) * (index + 1 + 5) + 5) << (5 + c + 1)) + (5 + c + 1) ;

    buff[index++] = c;
  } while(true);

  // save linelen read
  *linelen = index;

  // final hash calc
  for(i = 0; (i < 4); i++) { quad_hash[i] %= BLOOM_SIZE; }
  *ht_hash %= HASH_SIZE;

  return ret;
}

/////////////////////////////////////////////////////////////////////////////////
// Main

// execute
int main(int argc, char *argv[]) {

  if (argc < 3) {
    printf("usage: fcompare filename1 filename2\n");
    return 0;
  }

  char *filename1 = argv[1];
  char *filename2 = argv[2];

  FILE* fp1 = open_file(filename1);
  FILE* fp2 = open_file(filename2);

  fpos_t* fpos = (fpos_t*) malloc(sizeof(fpos_t));

  uint quad_hash[4] = { 0 }; // bloom bits 
  uint ht_hash = 0; // hash table bucket index
  short linelen = 0;

  logdebug("file lines %d\n", count_file_lines(fp1));
  logdebug("sizeof: %ld\n", sizeof(fpos_t));

  while(
      read_and_hash_line(
        fp1, fpos, freadbuff, &linelen, quad_hash, &ht_hash)) {

    hash_line_info(fpos, linelen, quad_hash, ht_hash);
  }

  if (_debug) {
    print_bloom_bits();
    print_hash_buckets();
  }

  while(
      read_and_hash_line(
        fp2, fpos, freadbuff, &linelen, quad_hash, &ht_hash)) {

    // 1). check bloom filter
    if (!find_in_bloom_bits(quad_hash)) {
      printf("%s\n", freadbuff); 
    }
    else {
      // 2). check hash table
      if (!find_in_hash_table(fp1, freadbuff, linelen, quad_hash, ht_hash)) {
        printf("%s\n", freadbuff);
      }
    }
  }

  close_file(fp1); 
  close_file(fp2); 

  return 0;
}
