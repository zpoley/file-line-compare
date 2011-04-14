#include "globals.h"
#include "zhsh.h"

/////////////////////////////////////////////////////////////////////////////////
// Main

int main(int argc, char *argv[]) {

  if (argc < 5) {
    printf("usage: fcompare file1 file2 hsh_buckets bloom_bits bloom_hashes\n");
    return 0;
  }

  open_files(argv[1], argv[2]);

  uint _in_hash_buckets = atoi(argv[3]);
  uint _in_bloom_bits = atoi(argv[4]);
  short _in_bloom_hashes = atoi(argv[5]);

  zhsh_init(fp1, _in_hash_buckets, _in_bloom_bits, _in_bloom_hashes);

  fpos_t fpos;
  short linelen = 0;

  // add first file to hash
  while(read_line(fp1, &fpos, freadbuff, &linelen)) {
    zhsh_set(&fpos, freadbuff, &linelen);
  }

  if (_debug) {
    print_bloom_bits();
    print_hash_buckets();
  }

  // check second file against hashed first
  while(read_line(fp2, &fpos, freadbuff, &linelen)) {
    if (!zhsh_check(freadbuff, &linelen)) {
      // print on identical lines
      printf("%s\n", freadbuff); 
    }
  }

  close_files();

  return 0;
}
