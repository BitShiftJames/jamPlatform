#ifndef JAMPLATFORM_H
#define JAMPLATFORM_H 

#include <cstdio>
#define roundup_4(n) (((n) + 3) & -4)

#include <cassert>
#include <cstddef>
#include <cstring>
#include <stdint.h>

void create_a_window(void **memory, uint32_t Width, uint32_t Height);
void destroy_a_window(void **memory);
bool DirectoryExist(const char *path);
bool CreateDirectory(const char *path);

inline void buf_write_u32(char *buffer, uint64_t *buffer_pos, uint64_t buffer_size,
                         uint32_t value_toWrite) {

  assert(*buffer_pos + sizeof(value_toWrite) <= buffer_size);
  assert(((size_t)buffer + *buffer_pos) % sizeof(value_toWrite) == 0);

 *(uint32_t *)(buffer + *buffer_pos) = value_toWrite;
 *buffer_pos += sizeof(value_toWrite);
}

inline void buf_write_u16(char *buffer, uint64_t *buffer_pos, uint64_t buffer_size,
                         uint16_t value_toWrite) {
  assert(*buffer_pos + sizeof(value_toWrite) <= buffer_size); // bounds check
  assert(((size_t)buffer + *buffer_pos) % sizeof(value_toWrite) == 0); // byte aligned

 *(uint16_t *)(buffer + *buffer_pos) = value_toWrite;
 *buffer_pos += sizeof(value_toWrite);
}
 

inline void buf_write_string(char *buffer, uint64_t *buffer_pos, uint64_t buffer_size,
                      char *src_buffer, uint32_t src_len) {
 assert(*buffer_pos + src_len <= buffer_size);

 buf_write_u32(buffer, buffer_pos, buffer_size, src_len);
 memcpy(buffer + *buffer_pos, src_buffer, roundup_4(src_len));
 *buffer_pos += roundup_4(src_len);
}

inline uint32_t buf_read_u32(char **buffer, uint64_t *buffer_pos) {
 assert(*buffer_pos >= sizeof(uint32_t));
 assert((size_t)*buffer % sizeof(uint32_t) == 0);

 uint32_t res = *(uint32_t *)(*buffer);
 *buffer += sizeof(res);
 *buffer_pos -= sizeof(res);

 return res;
}

inline uint16_t buf_read_u16(char **buffer, uint64_t *buffer_pos) {
 assert(*buffer_pos >= sizeof(uint16_t));
 assert((size_t)*buffer % sizeof(uint16_t) == 0);

 uint16_t res = *(uint16_t *)(*buffer);
 *buffer += sizeof(res);
 *buffer_pos -= sizeof(res);

 return res;
}

inline void buf_read_n(char **buffer, uint64_t *buffer_pos, char *dst, uint64_t n) {
  
 if (n > *buffer_pos) {
    return;
 }
 assert(*buffer_pos >= n);

 if (dst) {
  memcpy(dst, *buffer, n);
 }

 *buffer += n;
 *buffer_pos -= n;
}
#endif
