#ifndef __EXTRA_INT_H__
#define __EXTRA_INT_H__
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

uint16_t to_le16(uint16_t value);

uint32_t to_be32(uint32_t value);
uint64_t to_be64(uint64_t value);
uint32_t to_le32(uint32_t value);
uint64_t to_le64(uint64_t value);

void write_be32(uint8_t *data, uint32_t value);
void write_be64(uint8_t *data, uint64_t value);
uint32_t read_be32(const uint8_t *data);
void uint32_array_to_be_bytes(const uint32_t *input, size_t len,
                              uint8_t *output);
void uint16_array_to_be_bytes(const uint16_t *input, size_t len,
                              uint8_t *output);
size_t log2i(size_t number);
uint16_t to_le16(uint16_t value) {
  return (uint16_t)((value >> 8) | (value << 8));
}
uint32_t to_be32(uint32_t value) {
  return ((value & 0xFF000000) >> 24) | ((value & 0x00FF0000) >> 8) |
         ((value & 0x0000FF00) << 8) | ((value & 0x000000FF) << 24);
}
uint64_t to_be64(uint64_t value) {
  return ((value & 0xFF00000000000000ULL) >> 56) |
         ((value & 0x00FF000000000000ULL) >> 40) |
         ((value & 0x0000FF0000000000ULL) >> 24) |
         ((value & 0x000000FF00000000ULL) >> 8) |
         ((value & 0x00000000FF000000ULL) << 8) |
         ((value & 0x0000000000FF0000ULL) << 24) |
         ((value & 0x000000000000FF00ULL) << 40) |
         ((value & 0x00000000000000FFULL) << 56);
}
uint32_t to_le32(uint32_t value) {
  return ((value & 0x000000FFU) << 24) | ((value & 0x0000FF00U) << 8) |
         ((value & 0x00FF0000U) >> 8) | ((value & 0xFF000000U) >> 24);
}

uint64_t to_le64(uint64_t value) {
  return ((value & 0x00000000000000FFULL) << 56) |
         ((value & 0x000000000000FF00ULL) << 40) |
         ((value & 0x0000000000FF0000ULL) << 24) |
         ((value & 0x00000000FF000000ULL) << 8) |
         ((value & 0x000000FF00000000ULL) >> 8) |
         ((value & 0x0000FF0000000000ULL) >> 24) |
         ((value & 0x00FF000000000000ULL) >> 40) |
         ((value & 0xFF00000000000000ULL) >> 56);
}

uint32_t read_be32(const uint8_t *data) {
  return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
         ((uint32_t)data[2] << 8) | ((uint32_t)data[3]);
}
void write_be32(uint8_t *data, uint32_t value) {
  data[0] = (uint8_t)((value >> 24) & 0xFF);
  data[1] = (uint8_t)((value >> 16) & 0xFF);
  data[2] = (uint8_t)((value >> 8) & 0xFF);
  data[3] = (uint8_t)(value & 0xFF);
  return;
}
void write_be64(uint8_t *data, uint64_t value) {
  data[0] = (uint8_t)((value >> 56) & 0xFF);
  data[1] = (uint8_t)((value >> 48) & 0xFF);
  data[2] = (uint8_t)((value >> 40) & 0xFF);
  data[3] = (uint8_t)((value >> 32) & 0xFF);
  data[4] = (uint8_t)((value >> 24) & 0xFF);
  data[5] = (uint8_t)((value >> 16) & 0xFF);
  data[6] = (uint8_t)((value >> 8) & 0xFF);
  data[7] = (uint8_t)(value & 0xFF);
}

void uint32_array_to_be_bytes(const uint32_t *input, size_t len,
                              uint8_t *output) {
  uint8_t temp_output[4] = {0};
  for (size_t i = 0; i < len; i++) {
    temp_output[0] = (uint8_t)((input[i] >> 24) & 0xFF);
    temp_output[1] = (uint8_t)((input[i] >> 16) & 0xFF);
    temp_output[2] = (uint8_t)((input[i] >> 8) & 0xFF);
    temp_output[3] = (uint8_t)(input[i] & 0xFF);
    memcpy(output + i * 4, temp_output, 4);
  }
}
void uint16_array_to_be_bytes(const uint16_t *input, size_t len,
                              uint8_t *output) {
  uint8_t temp_output[2] = {0};
  for (size_t i = 0; i < len; i++) {
    temp_output[0] = (uint8_t)((input[i] >> 8) & 0xFF);
    temp_output[1] = (uint8_t)(input[i] & 0xFF);
    memcpy(output + i * 2, temp_output, 2);
  }
}
// type conversion is ass so implementing this shit
size_t log2i(size_t number) {
  size_t bits = 0;
  while (number != 0) {
    number = number >> 1;
    bits++;
  }
  return bits;
}
#endif
