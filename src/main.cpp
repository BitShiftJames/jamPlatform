#include "platform.h"
#include <cstdio>

int main() {
  bool doesDirectoryExist = DirectoryExist("~/home/");
  if (doesDirectoryExist) {
    printf("Directory does exist");
  } else {
    printf("Directory does exist");
  }

  return 0;
}
