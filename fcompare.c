#include "globals.h"
#include "zhsh.h"

/////////////////////////////////////////////////////////////////////////////////
// Main

int main(int argc, char *argv[]) {

  if (argc < 3) {
    printf("usage: fcompare filename1 filename2\n");
    return 0;
  }

  open_files(argv[1], argv[2]);

  zhsh_init(fp1, 1000000, 10000000, 1);

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
