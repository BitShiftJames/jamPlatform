
#include "../platform.h"
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

void error_handling() {
  int errsv = errno;
  switch (errsv) {
    case (ENOENT): {
      // File doesn't exist.
    } break;
    default: {
      // unhandled error.
      strerror(errsv);
    } break;
  }
}

void create_a_window() {

}

bool DirectoryExist(const char *path) {
  bool result = false;

  struct stat buf = {};

  if (stat(path, &buf) == -1) {
    error_handling();
  } else {
    result = S_ISDIR(buf.st_mode);
  }

  return result;
}

bool CreateDirectory(const char *path) {
  bool result = false;

  if (mkdir(path, S_IRWXU | S_IRWXG) == -1) {
    error_handling();
  } else {
    result = true;
  }

  return false;
}
