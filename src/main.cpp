#include "platform.h"
#include <cstdio>

int main() {
  
  void *memoryPtr = 0;
  create_a_window(&memoryPtr, 0, 0);
  destroy_a_window(&memoryPtr);

  return 0;
}
