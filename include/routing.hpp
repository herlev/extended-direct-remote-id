#pragma once

#include <stdio.h>
#include <sys/_stdint.h>

static uint64_t FNV1_a_hash(uint8_t buf[], uint8_t len) {
  uint64_t hash = 0xcbf29ce484222325;
  uint64_t FNV_prime = 0x100000001b3;
  for (int i = 0; i < len; i++) {
    hash ^= (uint64_t)buf[i];
    hash *= FNV_prime;
  }
  return hash;
}

struct TableElem {
  uint64_t id = 0;
  uint8_t latest_count = 0;
  bool is_valid = false;
};

static TableElem table[1000] = {};

void printTable() {
  printf("\nTable:\n");
  for (uint32_t i = 0; i < 1000; i++) {
    auto &e = table[i];
    if (e.is_valid) {
      printf("%016llX %u\n", e.id, e.latest_count);
    }
  }
  printf("\n");
}
