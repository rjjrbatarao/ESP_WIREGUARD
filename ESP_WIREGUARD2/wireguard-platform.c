#include "wireguard-platform.h"

#include <stdlib.h>
#include "crypto.h"
#include "lwip/sys.h"
#if defined ESP32
#define RANDOM_REG32 *((volatile uint32_t *)(0x3FF75144))
#else
#define RANDOM_REG32 *((volatile uint32_t *)(0x3FF20E44))
#endif


// This file contains a sample Wireguard platform integration

// DO NOT USE THIS FUNCTION - IMPLEMENT A BETTER RANDOM BYTE GENERATOR IN YOUR IMPLEMENTATION
void wireguard_random_bytes(void *bytes, size_t size) {
  int x;
  uint8_t *out = (uint8_t *)bytes;
  for (x = 0; x < size; x++) {
    out[x] = (int)RANDOM_REG32 % 0xFF;
  }
}

uint32_t wireguard_sys_now() {
  // Default to the LwIP system time
  return sys_now();
}

// CHANGE THIS TO GET THE ACTUAL UNIX TIMESTMP IN MILLIS - HANDSHAKES WILL FAIL IF THIS DOESN'T INCREASE EACH TIME CALLED
void wireguard_tai64n_now(uint8_t *output) {
  // See https://cr.yp.to/libtai/tai64.html
  // 64 bit seconds from 1970 = 8 bytes
  // 32 bit nano seconds from current second

  uint64_t _millis = sys_now();

  // Split into seconds offset + nanos
  uint64_t seconds = 0x400000000000000aULL + (_millis / 1000);
  uint32_t nanos = (_millis % 1000) * 1000;
  U64TO8_BIG(output + 0, seconds);
  U32TO8_BIG(output + 8, nanos);
}

bool wireguard_is_under_load() {
  return false;
}
