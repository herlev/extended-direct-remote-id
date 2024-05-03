#pragma once

#include <sys/_stdint.h>
class ByteBuffer {
  // [Start,end)
  uint8_t *_start;
  uint8_t *_end;

public:
  ByteBuffer(uint8_t *start, uint8_t *end) : _start(start), _end(end) {
  }
  ByteBuffer(uint8_t *start, uint32_t length) : _start(start), _end(start + length) {
  }
  ByteBuffer sub_buffer(uint32_t offset) {
    // TODO: assert start < (or equal?) end
    return ByteBuffer(start() + offset, _end);
  }
  uint8_t &operator[](uint32_t index) {
    return *(start() + index);
  }

  uint8_t *start() {
    return _start;
  }
  uint8_t *end() {
    return _end;
  }
  uint32_t length() {
    return _end - _start;
  }
};
