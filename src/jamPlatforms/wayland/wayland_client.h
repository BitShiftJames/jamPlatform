#ifndef JAM_WAYLAND_CLIENT_H
#define JAM_WAYLAND_CLIENT_H

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

#define MAX_MESSAGE_SIZE 4096
#define WAYLAND_HEADER_SIZE 8
#define COLOR_CHANNELS 4

// One hundred percent wayland specific

// FIXME: Opcodes need to be generated from an xml file instead
// of hardset even if using the stable interfaces

struct WL_DISPLAY {
  enum Methods {
    SYNC=0,
    GET_REGISTRY=1,
  };

  enum Events {
    ERROR=0,
    DELETE_ID=1,
  };
};

struct WL_REGISTERY {
  enum Methods {
    BIND=0,
  };

  enum Events {
    GLOBAL_EVENT=0,
    GLOBAL_REMOVE=1,
  };
};

struct WL_SHM {

  bool RGB565_supported;
  bool RGBA4444_supported;
  bool XRGB4444_supported;
  bool XRGB8888_supported;
  bool RGBA8888_supported;

  enum Formats {
    // 16 bit color values without alpha.
	  WL_SHM_FORMAT_RGB565 = 0x36314752,

    // 16 bit color values
	  WL_SHM_FORMAT_XRGB4444 = 0x32315258,
	  WL_SHM_FORMAT_RGBA4444 = 0x32314152,

    // 32 bit color values
	  WL_SHM_FORMAT_XRGB8888 = 1,
	  WL_SHM_FORMAT_RGBA8888 = 0x34324152,
  };

  enum Methods {
    WL_SHM_CREATE_POOL=0,
    WL_SHM_RELEASE=1,
  };

  enum Events {
    WL_SHM_FORMAT_EVENT=0,
  };
};

struct WL_SHM_POOL {
  enum Methods {
    CREATE_BUFFER=0,
    DESTROY=1,
    RESIZE=2,
  };

  enum Events{
    NO_EVENTS=-1,
  };
};

struct WL_BUFFER {
  enum Methods {
    DESTORY=0,
  };

  enum Events {
    DESTORY_EVENT=0 
  };
};

struct WL_COMPOSITOR {
  enum Methods {
    CREATE_SURFACE=0,
    CREATE_REGION=1,
  };

  enum Events {
    NO_EVENTS=-1,
  };
};

struct WL_OUTPUT {
  enum Methods {
    RELEASE = 0,
  };

  enum Events {
    GEOMETRY = 0,
    MODE = 1,
    DONE = 2,
    SCALE = 3,
    NAME = 4,
    DESCRIPTION = 5,
  };
};

struct WL_SURFACE {
  enum Methods {
    DESTROY=0,
    ATTACH=1,
    DAMAGE=2,
    FRAME=3,
    SET_OPAQUE_REGION=4,
    SET_INPUT_REGION=5,
    COMMIT=6,
    SET_BUFFER_TRANSFORM=7,
    SET_BUFFER_SCALE=8,
    DAMAGE_BUFFER=9,
    OFFSET=10,
  };

  enum Events {
    ENTER=0,
    LEAVE=1,
    PREFERRED_BUFFER_SCALE=2,
    PREFERRED_BUFFER_TRANSFORM=3,
  };
};

struct XDG_WM_BASE {
  enum Methods {
    DESTROY=0,
    CREATE_POSITIONER=1,
    GET_XDG_SURFACE=2,
    PONG=3,
  };

  enum Events {
    PING_EVENT=0,
  };
};

struct XDG_SURFACE {
  enum Methods {
    DESTROY=0,
    GET_TOPLEVEL=1,
    GET_POPUP=2,
    SET_WINDOW_GEOMETRY=3,
    ACK_CONFIGURE=4,
  };

  enum Events {
    CONFIGURE_EVENT=0,
  };
};

struct XDG_TOPLEVEL {
  enum Methods {
    DESTROY=0,
    SET_PARENT=1,
    SET_TITLE=2,
    SET_APP_ID=3,
    SHOW_WINDOW_MENU=4,
    MOVE=5,
    RESIZE=6,
    SET_MAX_SIZE=7,
    SET_MIN_SIZE=8,
    SET_MAXIMIZED=9,
    UNSET_MAXIMIZED=10,
    SET_FULLSCREEN=11,
    UNSET_FULLSCREEN=12,
    SET_MINIMIZED=13,
  };

  enum Events {
    CONFIGURE_EVENT=0,
    CLOSE_EVENT=1,
    CONFIGURE_BOUNDS=2,
    WM_CAPABILITIES_EVENT=3,
  };
};

struct OP_CODES {
  WL_DISPLAY wl_display; 
  WL_REGISTERY wl_registery;
  WL_BUFFER wl_buffer;
  WL_SHM wl_shm;
  WL_SHM_POOL wl_shm_poo;
  WL_SURFACE wl_surface;
  WL_COMPOSITOR wl_compositor;
  WL_OUTPUT wl_output;
  XDG_WM_BASE xdg_wm_base;
  XDG_SURFACE xdg_surface;
  XDG_TOPLEVEL xdg_toplevel;
};

enum window_stage {
  STATE_NONE,
  STATE_PAUSED,
  STATE_SURFACE_ACKED_CONFIGURE,
  STATE_SURFACE_ATTACHED,
};

struct wayland_windowState {
  int fd;

  // avoiding static chars on stack.
  char error_message[MAX_MESSAGE_SIZE];
  char message[MAX_MESSAGE_SIZE];
  size_t message_len;
  uint64_t message_pos;

  uint32_t current_obj_id; // 1 is reserved;
  
  OP_CODES opcodes;

  window_stage stage;

  // Client object IDs;
  uint32_t wl_display_id; 
  uint32_t wl_registry_id;
  uint32_t wl_shm_id;
  uint32_t wl_shm_pool_id;
  uint32_t wl_buffer_id;
  uint32_t xdg_wm_base_id;
  uint32_t xdg_surface_id;
  uint32_t wl_compositor_id;
  uint32_t wl_surface_id;
  uint32_t xdg_toplevel_id;
  uint32_t wl_output_id;
  uint32_t callback_id;

  // Pixel Buffer Information;
  uint32_t Width;
  uint32_t Height;
  uint32_t stride;
  
  uint32_t configure_serial;
  // Shared memory object
  int shm_fd;
  uint32_t shm_pool_size;
  uint8_t *shm_pool_data;

  
  int count;
};

inline void ClearMessageBuffer(char *buf, uint64_t *size, uint64_t capacity) {
  if (size) {
    if (*size > capacity) {
      printf("Trying to clear a larger capacity then you have\n");
      exit(errno);
    }

    memset(buf, 0, *size);
    *size = 0;
  } else {
    memset(buf, 0, capacity);
  }

}

inline void SendMessage(wayland_windowState *state) {
  if ((int64_t)state->message_pos != send(state->fd, state->message, state->message_pos, MSG_DONTWAIT)) {
    printf("Error sending message.\n");
    exit(errno);
  }
  printf("Succedded in sending message.\n");

  return;
}

inline void PrintBoundInterfaces(wayland_windowState *state) {
  printf("wl_display: %u\n", state->wl_display_id);
  printf("wl_registry: %u\n", state->wl_registry_id);
  printf("wl_shm: %u\n", state->wl_shm_id);
  printf("wl_shm_pool: %u\n", state->wl_shm_pool_id);
  printf("wl_buffer: %u\n", state->wl_buffer_id);
  printf("wl_surface: %u\n", state->wl_surface_id);
  printf("wl_compositor: %u\n", state->wl_compositor_id);
  printf("wl_xdg_wm_base: %u\n", state->xdg_wm_base_id);
  printf("wl_xdg_surface: %u\n", state->xdg_surface_id);
  printf("wl_xdg_toplevel: %u\n", state->xdg_toplevel_id);

}
// Not wayland specific

// Maybe wayland specific
int CreateSharedMemory(uint32_t size);

// Definitely wayland specific

bool connect_wayland_display(wayland_windowState *state); // Returns true on a successful connection
void wayland_wl_display_get_registry(wayland_windowState *windowState);
void wayland_window_set_up(wayland_windowState *state);
void wayland_listen_to_events(wayland_windowState *state, char **msg, uint64_t *msg_len);

// Done
void wayland_xdg_wm_base_pong(wayland_windowState *windowState, uint32_t ping);
int wayland_wl_compositor_create_surface(wayland_windowState *windowState);
int wayland_wl_registry_bind(wayland_windowState *windowState, uint32_t name, char *interface, uint32_t interface_len, uint32_t version);
int wayland_xdg_wm_base_get_xdg_surface(wayland_windowState *windowState);
int wayland_xdg_surface_get_toplevel(wayland_windowState *windowState);


// Not Done
int wayland_wl_shm_create_pool(wayland_windowState *windowState);
int wayland_wl_shm_pool_create_buffer(wayland_windowState *windowState);
void wayland_xdg_surface_ack_configure(wayland_windowState *windowState, uint32_t configure);
void wayland_xdg_toplevel_setid(wayland_windowState *windowState, char *string);
void wayland_wl_surface_commit(wayland_windowState *windowState);
void wayland_wl_surface_attach(wayland_windowState *windowState);

// Not in use.
void wayland_handle_message(wayland_windowState *state, char **msg, uint64_t *msg_len);

#endif // !JAM_WAYLAND_CLIENT_H
