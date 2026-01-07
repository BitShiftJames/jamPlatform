
#include "../platform.h"

#include <assert.h>
#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/socket.h>

/* Hours spent trying to get a window on screen
                        28
 based on: Philippe Gaultier's wayland from scratch.
 source: https://gaultier.github.io/blog/wayland_from_scratch.html */

#include "wayland/wayland_client.h"

void create_a_window(void **memory, uint32_t Width, uint32_t Height) {

  // FIXME: Query x11 or wayland.
  // Then allocate a windowState object.
  *memory = malloc(sizeof(wayland_windowState));
  wayland_windowState *windowState = ((wayland_windowState *)*memory);
  
  if (connect_wayland_display(windowState)) {
    wayland_wl_display_get_registry(windowState);
    while (1) {
      char read_buf[4096] = "";
      int64_t read_bytes = recv(windowState->fd, read_buf, sizeof(read_buf), 0);

      if (read_bytes == -1) {
        printf("Wayland closed the socket\n");
        exit(errno);
      }

      char *msg = read_buf;
      uint64_t msg_len = (uint64_t)read_bytes;

      int count;

      while (msg_len > 0) {
        wayland_listen_to_events(windowState, &msg, &msg_len);
      }

      wayland_window_set_up(windowState);

      fflush(stdout);   

    }
  }

}

void destroy_a_window(void **memory) {
  wayland_windowState *windowState = ((wayland_windowState *)*memory);

  close(windowState->fd);
  printf("And the file descriptor is gone...\n");
  windowState->fd = 0;
}

bool DirectoryExist(const char *path) {
  bool result = false;

  struct stat buf = {};

  if (stat(path, &buf) == -1) {
  } else {
    result = S_ISDIR(buf.st_mode);
  }

  return result;
}

bool CreateDirectory(const char *path) {
  bool result = false;

  if (mkdir(path, S_IRWXU | S_IRWXG) == -1) {
  } else {
    result = true;
  }

  return result;
}
