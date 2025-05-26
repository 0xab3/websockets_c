#include "libs/sha1.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
__always_inline char *read_entire_file(const char *filename,
                                       size_t *filesize_out) {
  FILE *file = fopen(filename, "rb"); // Open in binary mode
  assert(file != NULL);
  fseek(file, 0, SEEK_END);
  ssize_t filesize = ftell(file);
  rewind(file);
  assert(filesize != 0);

  char *buffer = malloc((size_t)filesize + 1);
  assert(buffer != 0);

  size_t read_size = fread(buffer, 1, (size_t)filesize, file);
  fclose(file);
  assert(read_size == (size_t)filesize);

  if (filesize_out) {
    *filesize_out = (size_t)filesize;
  }

  return buffer;
}

int main(int argc, char **argv) {
  assert(argc >= 2);
  const char *filename = argv[1];
  size_t file_size = 0;
  const char *contents = read_entire_file(filename, &file_size);
  assert(contents != NULL);
  sha1Digest digest = sha1_digest((const uint8_t *)contents, file_size);
  sha1_print_digest(&digest);
}
