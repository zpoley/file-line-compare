#ifndef ZHSH_H
#define ZHSH_H

#include "globals.h"

/////////////////////////////////////////////////////////////////////////////////
// zhsh types and vars

// hash bucket linked list node.
typedef struct HBNode {
  fpos_t fpos;    // file position pointer
  short linelen;  // file line length
  uint bloomhash; // k combined bloom hashes
  struct HBNode *next;
} HBNode;

// cached file ptr
FILE *hash_fp = NULL;

// bloom filter
static bool use_bloom = true;
static uint num_bloom_bytes;
static char* bloom_bytes;
static short num_bloom_hashes;
static uint* bloom_hashes = NULL;

// hash table
static uint num_hash_buckets;
HBNode** hash_buckets;


/////////////////////////////////////////////////////////////////////////////////
// zhsh hash table interface

/*
 * zhsh_init : 
 *  initialize the hash given a FILE* (for fpos refs), number of hash buckets, 
 *  number of bloom bits (0 = no bloom filter), and a number of bloom
 *  filter hash functions
 */
bool zhsh_init(
    FILE* fp, uint num_buckets, uint num_bloom_bits, short num_bloomhash);

/*
 * zhsh_set : 
 *  store the reference to a line from a file in the hash
 */
bool zhsh_set(fpos_t* fpos, char *buff, short* linelen);

/*
 * zhsh_check : 
 *  test for the presense of a line in the hash 
 */
bool zhsh_check(char *buff, short* linelen);


/////////////////////////////////////////////////////////////////////////////////
// Helpers

// debug bloom filter bits
void print_bloom_bits() {
  int i = 0, j = 0;
  printf("\nbloom bits:\n");
  for(; (i < (num_bloom_bytes)); i++) {
    short and1 = 1;
    for(j = 0; (j < 8); j++) {
      printf("%d", ((bloom_bytes[i] & (and1 << j)) > 0));
    }
  }
  printf("\n\n");
}

// debug hash table buckets
void print_hash_buckets() {
  uint i = 0;
  for(i = 0; (i < num_hash_buckets); i++) {
    printf("b: %d", i);
    if (hash_buckets[i]) {
      HBNode* curr = hash_buckets[i];
      do {
        printf(" %ld", (long) curr->fpos);
        curr = curr->next;
      }
      while(curr);
    }
    else { printf(" ___"); }
    printf("\n");
  }
}

// set num_bloom_hashes bloom bits identified by hashes indices
void set_bloom(uint* hashes) {
  short i = 0; 
  for(; (i < num_bloom_hashes); i++) { 
    uint h = hashes[i];
    bloom_bytes[(h / 8)] |= (1 << (h % 8));
  }
}

// test quad_hash against bloom_bits
//  return true if all 4 bits are set, false otherwise 
bool check_bloom(uint* hashes) {
  short i = 0; 
  for(; (i < num_bloom_hashes); i++) { 
    uint h = hashes[i];
    if (!(bloom_bytes[(h / 8)] & (1 << (h % 8)))) {
      return false;
    }
  }
  return true;
}

// compute hash and bloom bits from buffer
void compute_hashes(char* buff, short linelen, 
    uint* ht_hash, uint* hashes) {

  uint i = 0, j = 0;

  if (use_bloom) {
    // reset hashes values
    for(j = 0; (j < num_bloom_hashes); j++) { hashes[j] = 0; }
  }

  // generate hash values for buff
  for(; (i < linelen); i++) {
    if (use_bloom) {
      // incr bloom hash functions
      for(j = 0; (j < num_bloom_hashes); j++) { 
        hashes[j] += ((((buff[i] * 7) + i + j) * (i + 1 + j) + j) << (j + buff[i] + 1)) + (j + buff[i] + 1) ;
      }
    }
    // incr hash table function
    *ht_hash += ((((buff[i] * 7) + i + 5) * (i + 1 + 5) + 5) << (5 + buff[i] + 1)) + (5 + buff[i] + 1) ;
  }

  // complete hash calcs
  if (use_bloom) {
    for(j = 0; (j < num_bloom_hashes); j++) { hashes[j] %= (num_bloom_bytes * 8); }
  }
  *ht_hash %= num_hash_buckets;

  return; 
}

bool compare_line_to_buff(
  fpos_t* new_fpos, short linelen, char* buff) {

  fpos_t curr_fpos;
  fgetpos(hash_fp, &curr_fpos);

  // seek to file location of start of string
  fseek(hash_fp, *new_fpos - curr_fpos, SEEK_CUR);

  char filebuff[DEFAULT_FREAD_BUFF_SIZE] = { 0 };

  // read the line from the file
  fread((void*) filebuff, sizeof(char), linelen, hash_fp);

  return (memcmp(buff, filebuff, (size_t) linelen) == 0);
}


/////////////////////////////////////////////////////////////////////////////////
// zhsh hash table implementation

bool zhsh_init(
    FILE* fp, uint _num_hash_buckets, uint _num_bloom_bits, short _num_bloom_hashes) {

  hash_fp = fp;

  num_hash_buckets = _num_hash_buckets;
  num_bloom_bytes = _num_bloom_bits / 8;
  num_bloom_hashes = _num_bloom_hashes;
  if ((num_bloom_bytes > 0) &&  (num_bloom_hashes > 0)) {
    bloom_hashes = malloc(sizeof(uint*) * num_bloom_hashes);
    bloom_bytes = malloc(sizeof(char) * num_bloom_bytes);
    assert(bloom_bytes != NULL);
  }
  else {
    use_bloom = false;
  }

  hash_buckets = malloc(sizeof(HBNode*) * (num_hash_buckets));
  assert(hash_buckets != NULL);

  return true;
}

bool zhsh_set(fpos_t* fpos, char *buff, short* linelen) {
  short i = 0;
  uint ht_hash = 0;

  compute_hashes(buff, *linelen, &ht_hash, bloom_hashes);

  if (use_bloom) {
    set_bloom(bloom_hashes);
  }

  uint bb_dj = 0;

  for(;(i < num_bloom_hashes); i++) { bb_dj |= bloom_hashes[i]; }

  // create new hash bucket node
  HBNode *hbn =  (HBNode*)malloc(sizeof(HBNode));
  memcpy(&hbn->fpos, fpos, sizeof(fpos_t));
  hbn->linelen = *linelen;
  hbn->bloomhash = bb_dj;
  hbn->next = NULL;

  if (hash_buckets[ht_hash]) {
    // append to hash bucket
    HBNode* curr = hash_buckets[ht_hash];

    while(curr->next && (hbn->fpos > curr->fpos)) {
      curr = curr->next;
    }
    if (curr->next) {
      hbn->next = curr->next;
      curr->next = hbn;
    }
    else {
      curr->next = hbn;
    }
  }
  else {
    // create hash bucket
    hash_buckets[ht_hash] = hbn;
  }
  return true;
}

bool zhsh_check(char *buff, short* linelen) {

  uint ht_hash = 0;
  compute_hashes(buff, *linelen, &ht_hash, bloom_hashes);

  if (use_bloom) {
    if (!check_bloom(bloom_hashes)) {
      return false;
    }
  }

  rewind(hash_fp);
  short i = 0;
  uint bb_dj = 0;
  for(;(i < num_bloom_hashes); i++) { bb_dj |= bloom_hashes[i]; }

  // run through hash bucket chain
  if (hash_buckets[ht_hash]) {
    HBNode* curr = hash_buckets[ht_hash];
    do {
      // if both linelen and bloom intersection match, 
      //  only then attempt full string comparison
      if ((curr->linelen == *linelen) && (curr->bloomhash == bb_dj)) {
        if (compare_line_to_buff((fpos_t*)&curr->fpos, curr->linelen, buff)) {
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

#endif
