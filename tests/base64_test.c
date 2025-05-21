// stolen from:
// https://github.com/ReneNyffenegger/cpp-base64/blob/master/test.cpp
#include "../libs/base64.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_ENCODED_LEN 1024

int main(void) {
  int all_tests_passed = 1;
  base64EncodeSettings settings = {.padding = BASE64_ENCODE_PADDING};

  // Test 1: Basic test string
  const char *orig = "this shit shall work or i die";

  char encoded[MAX_ENCODED_LEN] = {0};
  size_t encoded_len = 0;

  if (base64_encode((const uint8_t *)orig, strlen(orig), encoded,
                    MAX_ENCODED_LEN, &encoded_len,
                    settings) != BASE64_STATUS_SUCCESS) {
    printf("Test 1: Encoding failed\n");
    all_tests_passed = 0;
  }

  const char *expected = "dGhpcyBzaGl0IHNoYWxsIHdvcmsgb3IgaSBkaWU=";
  if (strcmp(encoded, expected) != 0) {
    printf("Test 1: Encoded output mismatch\nExpected: %s\nGot: %s\n", expected,
           encoded);
    all_tests_passed = 0;
  }

  // Test 2: "abc"
  const char *input2 = "abc";
  const char *expected2 = "YWJj";
  memset(encoded, 0, sizeof(encoded));
  encoded_len = 0;
  if (base64_encode((const uint8_t *)input2, strlen(input2), encoded,
                    MAX_ENCODED_LEN, &encoded_len,
                    settings) != BASE64_STATUS_SUCCESS ||
      strcmp(encoded, expected2) != 0) {
    printf("Test 2: Failed encoding 'abc'\nExpected: %s\nGot: %s\n", expected2,
           encoded);
    all_tests_passed = 0;
  }

  // Test 3: "abcd"
  const char *input3 = "abcd";
  const char *expected3 = "YWJjZA==";
  memset(encoded, 0, sizeof(encoded));
  encoded_len = 0;
  if (base64_encode((const uint8_t *)input3, strlen(input3), encoded,
                    MAX_ENCODED_LEN, &encoded_len,
                    settings) != BASE64_STATUS_SUCCESS ||
      strcmp(encoded, expected3) != 0) {
    printf("Test 3: Failed encoding 'abcd'\nExpected: %s\nGot: %s\n", expected3,
           encoded);
    all_tests_passed = 0;
  }

  // Test 4: "abcde"
  const char *input4 = "abcde";
  const char *expected4 = "YWJjZGU=";
  memset(encoded, 0, sizeof(encoded));
  encoded_len = 0;
  if (base64_encode((const uint8_t *)input4, strlen(input4), encoded,
                    MAX_ENCODED_LEN, &encoded_len,
                    settings) != BASE64_STATUS_SUCCESS ||
      strcmp(encoded, expected4) != 0) {
    printf("Test 4: Failed encoding 'abcde'\nExpected: %s\nGot: %s\n",
           expected4, encoded);
    all_tests_passed = 0;
  }

  // Test 5: Input exactly 17 bytes
  const char *input5 = "aaaaaaaaaaaaaaaaa"; // 17 'a's
  const char *expected5 = "YWFhYWFhYWFhYWFhYWFhYWE=";
  memset(encoded, 0, sizeof(encoded));
  encoded_len = 0;
  if (base64_encode((const uint8_t *)input5, strlen(input5), encoded,
                    MAX_ENCODED_LEN, &encoded_len,
                    settings) != BASE64_STATUS_SUCCESS ||
      strcmp(encoded, expected5) != 0) {
    printf("Test 5: Failed encoding 17-byte input\nExpected: %s\nGot: %s\n",
           expected5, encoded);
    all_tests_passed = 0;
  }

  return all_tests_passed ? 0 : 1;
}
