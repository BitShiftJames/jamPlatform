
#include "wayland_client.h"

#include "../../platform.h"

#include <assert.h>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/socket.h>

bool connect_wayland_display(wayland_windowState *state) {
  const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
  uint64_t xdg_path_length = strlen(xdg_runtime_dir);

  if (xdg_runtime_dir == NULL) {
    printf("No XDG_RUNTIME_DIR\n");
    return false;
  }

  struct sockaddr_un address = {};
  address.sun_family = AF_UNIX;
  uint64_t socket_path_length = 0;
  
  if (xdg_path_length > sizeof(address.sun_path) - 1) {
    printf("The XDG_RUNTIME_PATH is larger than the socket address buffer\n");
    return false;
  }

  memcpy(address.sun_path, xdg_runtime_dir, xdg_path_length);
  socket_path_length += xdg_path_length;

  address.sun_path[socket_path_length++] = '/';

  char *wayland_display = getenv("WAYLAND_DISPLAY");
  if (wayland_display == NULL) {
    char wayland_display_default[] = "wayland-0";
    uint64_t wayland_default_len = strlen(wayland_display_default);

    memcpy(address.sun_path + socket_path_length, 
           wayland_display_default, wayland_default_len);

    socket_path_length += wayland_default_len;
  } else {
    uint64_t wayland_display_len = strlen(wayland_display);
    memcpy(address.sun_path + socket_path_length, 
           wayland_display, wayland_display_len);

    socket_path_length += wayland_display_len;
  }

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    printf("Couldn't create a socket\n");
    return false;
  }
  
  if (connect(fd, (struct sockaddr *)&address, sizeof(address))) {
    printf("Couldn't connect to the wayland socket\n");
    return false;
  }

  state->fd = fd;
  state->wl_display_id = 1;
  state->current_obj_id = 1;
  
  ClearMessageBuffer(state->message, &state->message_pos, MAX_MESSAGE_SIZE);

  return true;
}

void wayland_wl_display_get_registry(wayland_windowState *windowState) {

  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // New ID.
  windowState->wl_registry_id = ++windowState->current_obj_id;
  assert(windowState->wl_registry_id != windowState->wl_display_id);

  // Sizing.
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE + sizeof(windowState->wl_registry_id);
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header.
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_display_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_display.GET_REGISTRY);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  // Args.
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_registry_id);

  SendMessage(windowState); 

  printf("-> wl_display@%u.get_registry: wl_registry=%u\n", windowState->wl_display_id, windowState->wl_registry_id);
}

int wayland_wl_registry_bind(wayland_windowState *windowState, uint32_t name, char *interface, uint32_t interface_len, uint32_t version) {
  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // New ID.
  ++windowState->current_obj_id;
  uint32_t new_id = windowState->current_obj_id;

  // Sizing.
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE + 
                                sizeof(name) + 
                                sizeof(interface_len) + 
                                roundup_4(interface_len) + 
                                sizeof(version) + 
                                sizeof(new_id);
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header.
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_registry_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_registery.BIND);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  // Args.
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, name);
  buf_write_string(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, interface, interface_len);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, version);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, new_id);

  SendMessage(windowState);

  return new_id;
}

void wayland_xdg_wm_base_pong(wayland_windowState *windowState, uint32_t ping) {
  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // Sizing
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE + sizeof(ping); 
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header 
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->xdg_wm_base_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.xdg_wm_base.PONG);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  // Args
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, ping);

  SendMessage(windowState);
}

int wayland_wl_compositor_create_surface(wayland_windowState *windowState) {
  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // New ID.
  ++windowState->current_obj_id;
  uint32_t new_id = windowState->current_obj_id;

  // Sizing
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE + sizeof(new_id); 
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header 
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_compositor_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_compositor.CREATE_SURFACE);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  // Args
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, new_id);
  
  SendMessage(windowState);

  return new_id;
}

int wayland_xdg_wm_base_get_xdg_surface(wayland_windowState *windowState) {
  assert(windowState->xdg_wm_base_id > 0);
  assert(windowState->wl_surface_id > 0);

  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // New ID.
  ++windowState->current_obj_id;
  uint32_t new_id = windowState->current_obj_id;

  // Sizing
  uint16_t msg_announced_size = 16;
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header 
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->xdg_wm_base_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.xdg_wm_base.GET_XDG_SURFACE);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  // Args
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, new_id);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_surface_id);

  SendMessage(windowState);

  return new_id;
}

int wayland_xdg_surface_get_toplevel(wayland_windowState *windowState) {
  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // New ID.
  ++windowState->current_obj_id;
  uint32_t new_id = windowState->current_obj_id;

  // Sizing
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE + sizeof(new_id); 
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header 
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->xdg_surface_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.xdg_surface.GET_TOPLEVEL);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  // Args
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, new_id);

  SendMessage(windowState);

  return new_id;
}

void unhandled_opcode(wayland_windowState *state, uint32_t remaining_bytes, char **msg, uint64_t *msg_len,
                      uint16_t opcode, uint16_t announced_size, uint32_t object_id) {
  if (state) {
    buf_read_n(msg, msg_len, state->error_message, remaining_bytes);
  } else {
    buf_read_n(msg, msg_len, 0, remaining_bytes);
  }
  
}

void wayland_wl_surface_commit(wayland_windowState *windowState) {
  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // Sizing
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE; 
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header 
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_surface_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_surface.COMMIT);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  SendMessage(windowState);
}

void wayland_wl_shm_create_pool(wayland_windowState *windowState) {
  assert(windowState->shm_pool_size != 0);
  assert(windowState->fd != 0);

  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // New ID.
  if (windowState->wl_shm_pool_id == 0) {
    ++windowState->current_obj_id;
    windowState->wl_shm_pool_id = windowState->current_obj_id;
  }

  // Sizing.
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE+
                             sizeof(windowState->wl_shm_pool_id) +
                             sizeof(windowState->shm_pool_size);
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_shm_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_shm.WL_SHM_CREATE_POOL);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  // Args
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_shm_pool_id);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->shm_pool_size);
  assert(roundup_4(windowState->message_pos) == windowState->message_pos);

  // Send the file descriptor as ancillary data.
  // UNIX/Macros monstrosities ahead.
  char buf[CMSG_SPACE(sizeof(windowState->shm_fd))] = "";

  struct iovec io = {.iov_base = windowState->message, .iov_len = windowState->message_pos};
  struct msghdr socket_msg = {
   .msg_iov = &io,
   .msg_iovlen = 1,
   .msg_control = buf,
   .msg_controllen = sizeof(buf),
  };

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&socket_msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(windowState->shm_fd));

  *((int *)CMSG_DATA(cmsg)) = windowState->shm_fd;
  socket_msg.msg_controllen = CMSG_SPACE(sizeof(windowState->shm_fd));

  if (sendmsg(windowState->fd, &socket_msg, 0) == -1) {
    exit(errno);
  }

}

void wayland_wl_surface_frame(wayland_windowState *windowState) {

  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // New ID.
  if (windowState->frame_callback_id == 0) {
    ++windowState->current_obj_id;
    windowState->frame_callback_id = windowState->current_obj_id;
    printf("Creating a frame callback ID\n");
    printf("Creating a frame callback ID\n");
    printf("Creating a frame callback ID\n");
    printf("Creating a frame callback ID\n");
    printf("Creating a frame callback ID\n");
    printf("Creating a frame callback ID\n");
    printf("Creating a frame callback ID\n");
  } else {
    printf("Frame callback is %u\n", windowState->frame_callback_id);
  }

  // Sizing.
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE + sizeof(windowState->frame_callback_id);
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_surface_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_surface.FRAME);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  // Args.
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->frame_callback_id);
  
  SendMessage(windowState);

  printf("Asking for frame hinting\n");
}

void wayland_wl_surface_attach(wayland_windowState *windowState) {
  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  printf("CHECKPOINT\n");
  assert(windowState->message_pos == 0);

  // Sizing.
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE + sizeof(uint32_t) * 3;
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_surface_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_surface.ATTACH);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  // Args
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_buffer_id);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, 0);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, 0);

  SendMessage(windowState);
  
}

void wayland_wl_surface_damage(wayland_windowState *windowState) {
  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // Sizing.
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE + sizeof(uint32_t) * 4;
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header.
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_surface_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_surface.DAMAGE);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);
  
  // Args.
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, 0);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, 0);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->Width);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->Height);

  SendMessage(windowState);
}

int wayland_wl_shm_pool_create_buffer(wayland_windowState *windowState) {
  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // New ID.
  ++windowState->current_obj_id;
  uint32_t new_id = windowState->current_obj_id;

  // Sizing.
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE +
                                sizeof(uint32_t) * 6;
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_shm_pool_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_shm_pool.CREATE_BUFFER);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);
  // Args
  
  printf("\nWidth: %u Height: %u Stride: %u\n", windowState->Width, windowState->Height, windowState->stride);

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, new_id);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, 0);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->Width);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->Height);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->Width * COLOR_CHANNELS);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, 1);

  SendMessage(windowState);

  return new_id;
}

void wayland_wl_shm_pool_destroy(wayland_windowState *windowState) {
  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // Sizing
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE; 
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header 
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_shm_pool_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_shm_pool.DESTROY);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  SendMessage(windowState);
}

void wayland_wl_buffer_destroy(wayland_windowState *windowState) {

  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // Sizing
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE; 
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header 
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_buffer_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.wl_buffer.DESTROY);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  SendMessage(windowState);

}

void wayland_xdg_surface_ack_configure(wayland_windowState *windowState, uint32_t configure) {

  ClearMessageBuffer(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE);
  // Make sure it is cleared.
  assert(windowState->message_pos == 0);

  // Sizing.
  uint16_t msg_announced_size = WAYLAND_HEADER_SIZE + sizeof(windowState->configure_serial);
  assert(roundup_4(msg_announced_size) == msg_announced_size);

  // Header
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->xdg_surface_id);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->opcodes.xdg_surface.ACK_CONFIGURE);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  // Args.
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, configure);
  
  SendMessage(windowState);
  printf("-> xdg_surface@%u.ack_configure: configure=%u\n", windowState->xdg_surface_id, configure);
}

void wayland_window_set_up(wayland_windowState *state) {
  if (state->wl_compositor_id !=  0 &&
      state->xdg_wm_base_id != 0 &&
      state->xdg_surface_id == 0 &&
      state->wl_surface_id == 0) {
      

    state->wl_surface_id = wayland_wl_compositor_create_surface(state);
    state->xdg_surface_id = wayland_xdg_wm_base_get_xdg_surface(state);
    state->xdg_toplevel_id = wayland_xdg_surface_get_toplevel(state);
    wayland_wl_surface_commit(state);
    PrintBoundInterfaces(state);
  }

  if (state->Width != 0 &&
      state->Height != 0 &&
      state->wl_shm_id != 0 &&
      state->wl_shm_pool_id == 0) {

    int fd = memfd_create("wayland_shared_memory", MFD_CLOEXEC);

    uint32_t size = state->stride * state->Height;
    printf("Size %u\n", size);
    if (ftruncate(fd, size) == -1) {
      printf("failed to truncate memory file\n");
      exit(errno);
    }

    state->shm_pool_data = (uint8_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    state->shm_pool_size = size;
    state->shm_fd = fd;
    
    if (state->shm_pool_data != 0 &&
        state->shm_pool_size > 0) {

      wayland_wl_shm_create_pool(state);
      state->wl_buffer_id = wayland_wl_shm_pool_create_buffer(state);
      wayland_wl_shm_pool_destroy(state);

      memset(state->shm_pool_data, 0xFF0000FF, state->shm_pool_size); 

      wayland_wl_surface_attach(state);
      wayland_wl_surface_frame(state);
      wayland_wl_surface_commit(state);

    }
  }

  state->blue++;
}


void wayland_listen_to_events(wayland_windowState *state, char **msg, uint64_t *msg_len) {
  assert(*msg_len >= 0);
  
  uint32_t object_id = buf_read_u32(msg, msg_len);
  uint16_t opcode = buf_read_u16(msg, msg_len);
  uint16_t announced_size = buf_read_u16(msg, msg_len);

  printf("OBJ_ID: %u, OPCODE: %u, SIZE: %u\n", object_id, opcode, announced_size);
  assert(roundup_4(announced_size) <= announced_size);

  uint32_t header_size = sizeof(object_id) + sizeof(opcode) + sizeof(announced_size);
  assert(announced_size <= header_size + *msg_len);

  uint32_t bytes_to_read_out = announced_size - header_size;
  uint32_t bytes_read_out = *msg_len;

  if (object_id == state->wl_display_id) {
    printf("Event recieved from wl_display ");
    if (state->opcodes.wl_display.ERROR == opcode) {
      uint32_t target_object_id = buf_read_u32(msg, msg_len);
      uint32_t error_code = buf_read_u32(msg, msg_len);
      uint32_t error_length = buf_read_u32(msg, msg_len); 
      
      char buffer[4096] = "";
      buf_read_n(msg, msg_len, buffer, roundup_4(error_length));
      printf("fatel error: target_object_id@%u: error_code %u: error: %s\n",
              target_object_id, error_code, buffer);
      
      exit(errno);
    } else {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
    } 

  } else if (object_id == state->wl_registry_id) {
    printf("Event recieved from wl_registry ");

    if (state->opcodes.wl_registery.GLOBAL_EVENT == opcode) {

      uint32_t name = buf_read_u32(msg, msg_len);
      uint32_t interface_len = buf_read_u32(msg, msg_len);

      char buffer[4096] = "";
      buf_read_n(msg, msg_len, buffer, roundup_4(interface_len));

      uint32_t version_number = buf_read_u32(msg, msg_len);

      printf("\nEvent: Registry Global recieved numeric name: %u: name %s, version %u\n",
              name, buffer, version_number);

      char wl_shm_interface[] = "wl_shm";
      if (strcmp(wl_shm_interface, buffer) == 0) {
        state->wl_shm_id = wayland_wl_registry_bind(state, name, buffer, interface_len, version_number);
        printf("Action: Registry.bind@%u Interface bound %s@%u\n", state->wl_registry_id, wl_shm_interface, state->wl_shm_id);
      }

      char xdg_wm_base[] = "xdg_wm_base";
      if (strcmp(xdg_wm_base, buffer) == 0) {
        state->xdg_wm_base_id = wayland_wl_registry_bind(state, name, buffer, interface_len, version_number);
        printf("Action: Registry.bind@%u Interface bound %s@%u\n", state->wl_registry_id, xdg_wm_base, state->xdg_wm_base_id);
      }

      char wl_compositor_interface[] = "wl_compositor";
      if (strcmp(wl_compositor_interface, buffer) == 0) {
        state->wl_compositor_id = wayland_wl_registry_bind(state, name, buffer, interface_len, version_number);
        printf("Action: Registry.bind@%u Interface bound %s@%u\n", state->wl_registry_id, wl_compositor_interface, state->wl_compositor_id);

      }

      char wl_output_interface[] = "wl_ouput";
      if (strcmp(wl_compositor_interface, buffer) == 0) {
        state->wl_output_id = wayland_wl_registry_bind(state, name, buffer, interface_len, version_number);
        printf("Action: Registry.bind@%u Interface bound %s@%u\n", state->wl_registry_id, wl_output_interface, state->wl_output_id);

      }


    } else {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
    }

  } else if (object_id == state->wl_output_id) {
    if (state->opcodes.wl_output.MODE == opcode) {
      uint32_t flags = buf_read_u32(msg, msg_len);
      uint32_t width = buf_read_u32(msg, msg_len);
      uint32_t height = buf_read_u32(msg, msg_len);
      uint32_t refresh = buf_read_u32(msg, msg_len);

      printf("Screen Width: %u, Screen Height: %u, Refressh Rate: %u\n", width, height, refresh);
    } else {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
    }
  } else if (object_id == state->wl_shm_id) {
    printf("Event recieved from wl_shm \n");
    if (state->opcodes.wl_shm.WL_SHM_FORMAT_EVENT == opcode) {
      uint32_t format = buf_read_u32(msg, msg_len);
      
      printf("[FORMAT]: shared memory format %u is supported\n", format);


    } else {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
    }

  } else if (object_id == state->wl_shm_pool_id) {
    printf("Event recieved from wl_shm_pool ");
    switch (opcode) {
      default: {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
      } break;
    
    }
  } else if (object_id == state->wl_buffer_id) {
    printf("Event recieved from wl_buffer ");
    switch (opcode) {
      default: {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
      } break;
    
    }
  } else if (object_id == state->wl_compositor_id) {
    printf("Event recieved from wl_compositor ");
    switch (opcode) {
      default: {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
      } break;
    
    }
  } else if (object_id == state->wl_surface_id) {
    printf("Event recieved from wl_surface ");
    switch (opcode) {
      default: {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
      } break;
    
    }
  } else if (object_id == state->xdg_wm_base_id) {
    printf("Event recieved from xdg_wm_base ");
    if (state->opcodes.xdg_wm_base.PING_EVENT == opcode) {
      printf("PING \n");
      uint32_t ping_serial = buf_read_u32(msg, msg_len);
      wayland_xdg_wm_base_pong(state, ping_serial);
      printf("PONG \n");
      
    } else {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
    }
  } else if (object_id == state->xdg_surface_id) {
    printf("Event recieved from xdg_surface ");
    if (state->opcodes.xdg_surface.CONFIGURE_EVENT == opcode) {
      uint32_t serial = buf_read_u32(msg, msg_len);

      printf("Recieved an configure serial of %u\n", serial);

      wayland_xdg_surface_ack_configure(state, serial);
      return;
    } else {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
    }
  } else if (object_id == state->xdg_toplevel_id) {
    printf("Event recieved from xdg_toplevel ");
    if (state->opcodes.xdg_toplevel.WM_CAPABILITIES_EVENT == opcode) {
      uint32_t length = buf_read_u32(msg, msg_len);
      uint32_t array_count = length / sizeof(uint32_t);
      printf("length: %u, array_count: %u\n", length, array_count);
      for (uint32_t Index = 0; Index < array_count; Index++) {
        uint32_t array_element = buf_read_u32(msg, msg_len);

        printf("Xdg_toplevel has capability: %u\n", array_element);
      }

      if (array_count == 0) {
        printf("This xdg_toplevel object has no capabilities\n");
      }

    } else if (state->opcodes.xdg_toplevel.CONFIGURE_EVENT == opcode) {
      
      uint32_t new_width = buf_read_u32(msg, msg_len);
      uint32_t new_height = buf_read_u32(msg, msg_len);

      uint32_t length = buf_read_u32(msg, msg_len);
      uint32_t array_count = length / sizeof(uint32_t);
      
      state->Width = new_width;
      state->Height = new_height;
      state->stride = new_width * COLOR_CHANNELS;
      
      uint32_t size = state->stride * state->Height;
      printf("Width: %u, Height %u, and Size %u\n", new_width, new_height, size);
      printf("length: %u, array_count: %u\n", length, array_count);
      for (uint32_t Index = 0; Index < array_count; Index++) {
        uint32_t array_element = buf_read_u32(msg, msg_len);

        printf("Xdg_toplevel has state: %u\n", array_element);
      }

      if (array_count == 0) {
        printf("This xdg_toplevel object has no states\n");
      }

    } else {
      unhandled_opcode(state, bytes_to_read_out, msg, msg_len, opcode, announced_size, object_id);
    }
  } else if (object_id == state->frame_callback_id) {
    uint32_t current_time = buf_read_u32(msg, msg_len);
    printf("Current_time %u\n", current_time);
    wayland_wl_surface_frame(state);

    int fd = memfd_create("wayland_shared_memory", MFD_CLOEXEC);

    uint32_t size = state->stride * state->Height;
    printf("Size %u\n", size);
    if (ftruncate(fd, size) == -1) {
      printf("failed to truncate memory file\n");
      exit(errno);
    }

    munmap(state->shm_pool_data, state->shm_pool_size);

    state->shm_pool_data = (uint8_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    state->shm_pool_size = size;
    state->shm_fd = fd;
    
    if (state->shm_pool_data != 0 &&
        state->shm_pool_size > 0) {

      wayland_wl_shm_create_pool(state);
      state->wl_buffer_id = wayland_wl_shm_pool_create_buffer(state);
      wayland_wl_shm_pool_destroy(state);

      uint32_t *pixels = (uint32_t *)state->shm_pool_data;
      for (uint32_t Index = 0; Index < state->Width * state->Height; Index++) {
        uint8_t r = state->blue;
        uint8_t b = state->blue;
        uint8_t g = state->blue;
        uint8_t a = 0xff;
        pixels[Index] = a << 24 | r << 16 | g << 8 | b;
      }

      wayland_wl_surface_attach(state);
      wayland_wl_surface_damage(state);
      wayland_wl_surface_commit(state);

    }

  } else {
    printf("Unkown object id.");
  }
}

/* This is are the functions that make up my first attemp at a wayland client
 
void wayland_handle_message(wayland_windowState *state, char **msg, uint64_t *msg_len) {
  assert(*msg_len >= 8);

  uint32_t object_id = buf_read_u32(msg, msg_len);
  uint16_t opcode = buf_read_u16(msg, msg_len);
  uint16_t announced_size = buf_read_u16(msg, msg_len);
  assert(roundup_4(announced_size) <= announced_size);

  uint32_t header_size = sizeof(object_id) + sizeof(opcode) + sizeof(announced_size);

  assert(announced_size <= header_size + *msg_len);

  uint32_t delta = announced_size - header_size;
  uint64_t message_length = *msg_len;
  if (object_id == state->wl_registry &&
    opcode == wayland_wl_registry_event_global) {

    printf("Recieved an wl_registry\n");
    uint32_t name = buf_read_u32(msg, msg_len);
    uint32_t interface_len = buf_read_u32(msg, msg_len);
    uint32_t padded_interface_len = roundup_4(interface_len);

    char interface[4096] = "";
    assert(padded_interface_len <= sizeof(interface));
    buf_read_n(msg, msg_len, interface, padded_interface_len);

    assert(interface[interface_len - 1] == 0);

    uint32_t version = buf_read_u32(msg, msg_len);
    printf("<- wl_registry@%u.global: name=%u interface=%.*s version=%u\n", state->wl_registry, name, interface_len, interface, version);

    char wl_shm_interface[] = "wl_shm";
    if (strcmp(wl_shm_interface, interface) == 0) {
      state->wl_shm = wayland_wl_registry_bind(state, name, interface, interface_len, version);
      printf("wayland_shared_memory ID is %u\n", state->wl_shm);
    }

    char xdg_wm_base_interface[] = "xdg_wm_base";
    if (strcmp(xdg_wm_base_interface, interface) == 0) {
      state->xdg_wm_base = wayland_wl_registry_bind(state, name, interface, interface_len, version);
      printf("xdg_wm_bas ID is %u\n", state->xdg_wm_base);
    }

    char wl_compositor_interface[] = "wl_compositor";
    if (strcmp(wl_compositor_interface, interface) == 0) {
      state->wl_compositor = wayland_wl_registry_bind(state, name, interface, interface_len, version);
      printf("wayland_compositor ID is %u\n", state->wl_compositor);
    }

    return;
  } else if (object_id == state->wl_display && opcode == wayland_wl_display_error_event) {
    printf("Recieved an wl_display\n");
    uint32_t target_object_id = buf_read_u32(msg, msg_len);
    uint32_t code = buf_read_u32(msg, msg_len);
    char error[512] = "";
    uint32_t error_len = buf_read_u32(msg, msg_len);
    buf_read_n(msg, msg_len, error, roundup_4(error_len));

    fprintf(stderr, "fatal error: target_object_id=%u code=%u error=%s\n",
           target_object_id, code, error);
    exit(EINVAL);
  } else if (object_id == state->wl_shm &&
             opcode == wayland_shm_pool_event_format) {

    uint32_t format = buf_read_u32(msg, msg_len);
    printf("<- wl_shm: format=%#x\n", format);

    return;
  } else if (object_id == state->wl_buffer && 
             opcode == wayland_wl_buffer_event_release) {

    printf("Recieved an wl_buffer\n");

    printf("<- xdg_wl_buffer@%u.release\n", state->wl_buffer);
    return;
  } else if (object_id == state->xdg_wm_base && 
             opcode == wayland_xdg_wm_base_event_ping) {
    uint32_t ping = buf_read_u32(msg, msg_len);
    printf("Recieved an ping\n");
    printf("<- xdg_wm_base@%u.ping: ping=%u\n", state->xdg_wm_base, ping);
    wayland_xdg_wm_base_pong(state, ping);
    return;
  } else if (object_id == state->xdg_toplevel &&
             opcode == wayland_xdg_toplevel_event_configure) {
    uint32_t w = buf_read_u32(msg, msg_len);
    uint32_t h = buf_read_u32(msg, msg_len);
    uint32_t len = buf_read_u32(msg, msg_len);
    
    char buf[256] = "";
    assert(len <= sizeof(buf));
    buf_read_n(msg, msg_len, buf, len);
    printf("<- xdg_toplevel@%u.configure: w=%u h=%u states[%u]\n", state->xdg_toplevel, w, h, len);

    printf("Recieved an xdg_top_level\n");
    return;
  } else if (object_id == state->xdg_surface &&
             opcode == wayland_xdg_surface_event_configure) {

    printf("Recieved an xdg_surface\n");
    uint32_t configure = buf_read_u32(msg, msg_len);
    printf("<- xdg_surface@%u.configure: configure=%u\n", state->xdg_surface, configure);
    wayland_xdg_surface_ack_configure(state, configure);
    state->stage = STATE_SURFACE_ACKED_CONFIGURE;
    printf("CHECKPOINT !!!!\n");

    return;
  } else if (object_id == state->xdg_toplevel &&
             opcode == wayland_xdg_toplevel_setid_opcode) {

    uint32_t delta = announced_size - header_size;
    printf("announced_size: %u, delta: %u", announced_size, delta);

    char buf[4096] = "";
    buf_read_n(msg, msg_len, buf, delta);

    for (uint32_t Index = 0; Index < delta; Index++) {
      if (Index == 0) {
        printf("\n Msg: ");
      }
      printf("%02x ", buf[Index]);
      if ((Index + 1) % 8 == 0) {
        printf("\n Msg: ");
      }
    }
    printf("\n");
    
  } else if (object_id == state->xdg_toplevel && 
             opcode == wayland_xdg_toplevel_event_close) {

    printf("Recieved an xdg_toplevel\n");
    printf("<- xdg_toplevel@%u.close\n", state->xdg_toplevel);
    exit(0);
  } else {
    printf("Unhandled case!: moving past by this amount-> %u\n", delta);
    printf("object_id: %u, opt_code: %u, announced_size: %u\n", object_id, opcode, announced_size);
    buf_read_n(msg, msg_len, 0, delta);
    return;
  }

}

int wayland_wl_registry_bind(wayland_windowState *windowState, uint32_t name, char *interface, uint32_t interface_len, uint32_t version) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_registry);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, wayland_wl_registry_bind_opcode);

  uint16_t msg_announced_size =
       wayland_header_size + sizeof(name) + 
       sizeof(interface_len) + roundup_4(interface_len) + sizeof(version) + sizeof(windowState->current_obj_id);
  assert(roundup_4(msg_announced_size) == msg_announced_size);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, name);
  buf_write_string(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, interface, interface_len);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, version);

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->current_obj_id);

  assert(windowState->message_pos == roundup_4(windowState->message_pos));

  if ((int64_t)windowState->message_pos != send(windowState->fd, windowState->message, windowState->message_pos, 0)) {
    printf("Failed to send a bind message.");
    exit(errno);
  }

  printf("-> wl_registry@%u.bind: name=%u interface=%.*s version=%u\n",
          windowState->wl_registry, name, interface_len, interface, version);

  return windowState->current_obj_id++;
}

int wayland_wl_shm_create_pool(wayland_windowState *windowState) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_shm);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, wayland_wl_shm_create_pool_opcode);

  uint16_t msg_announced_size = wayland_header_size + 
                                sizeof(windowState->current_obj_id) + 
                                sizeof(windowState->shm_pool_size);
  assert(roundup_4(msg_announced_size) == msg_announced_size);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->current_obj_id);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->shm_pool_size);

  assert(roundup_4(windowState->message_pos) == windowState->message_pos);

  // Send the file descriptor as ancillary data.
  // UNIX/Macros monstrosities ahead.
  char buf[CMSG_SPACE(sizeof(windowState->shm_fd))] = "";

  struct iovec io = {.iov_base = windowState->message, .iov_len = windowState->message_pos};
  struct msghdr socket_msg = {
  .msg_iov = &io,
  .msg_iovlen = 1,
  .msg_control = buf,
  .msg_controllen = sizeof(buf),
  };

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&socket_msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(windowState->shm_fd));

  *((int *)CMSG_DATA(cmsg)) = windowState->shm_fd;
  socket_msg.msg_controllen = CMSG_SPACE(sizeof(windowState->shm_fd));

  if (sendmsg(windowState->fd, &socket_msg, 0) == -1) {
    exit(errno);
  }

  printf("-> wl_shm@%u.create_pool: wl_shm_pool=%u\n", windowState->wl_shm, windowState->current_obj_id);
  windowState->wl_shm_pool = windowState->current_obj_id;

  return windowState->current_obj_id++;

}

int wayland_wl_shm_pool_create_buffer(wayland_windowState *windowState) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_shm_pool);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, wayland_wl_shm_pool_create_buffer_opcode);
  
  uint32_t offset = 0;
  uint16_t msg_announced_size = wayland_header_size + sizeof(uint32_t)*6;
  assert(roundup_4(msg_announced_size) == msg_announced_size);
                              
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->current_obj_id); 
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, offset);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->Width);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->Height);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->stride);

  uint32_t format = wayland_format_xrgb8888;
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, format);

  assert(windowState->message_pos == roundup_4(windowState->message_pos));

  if ((int64_t)windowState->message_pos != send(windowState->fd, windowState->message, windowState->message_pos, 0)) {
    exit(errno);
  }

  printf("-> wl_shm_pool@%u.create_buffer: wl_buffer=%u\n", windowState->wl_shm_pool, windowState->current_obj_id);
  windowState->wl_buffer = windowState->current_obj_id;

  return windowState->current_obj_id++;
}

int wayland_xdg_wm_base_get_xdg_surface(wayland_windowState *windowState) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;


  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->xdg_wm_base);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, wayland_xdg_wm_base_get_xdg_surface_opcode);

  uint16_t msg_announced_size = wayland_header_size + sizeof(windowState->current_obj_id) + sizeof(windowState->wl_surface);

  assert(roundup_4(msg_announced_size) == msg_announced_size);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->current_obj_id);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_surface);


  if ((int64_t)windowState->message_pos != send(windowState->fd, windowState->message, windowState->message_pos, MSG_DONTWAIT)) {
    printf("failed to get wl_get_registry\n");
    exit(errno);
  }
  
  printf("-> xdg_wm_base@%u.get_xdg_surface: xdg_surface=%u\n", windowState->xdg_wm_base, windowState->current_obj_id);
  windowState->xdg_surface = windowState->current_obj_id;

  return windowState->current_obj_id++;
}

int wayland_xdg_surface_get_toplevel(wayland_windowState *windowState) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;


  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->xdg_surface);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, wayland_xdg_surface_get_toplevel_opcode);

  uint16_t msg_announced_size = wayland_header_size + sizeof(windowState->current_obj_id);

  assert(roundup_4(msg_announced_size) == msg_announced_size);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->current_obj_id);

  if ((int64_t)windowState->message_pos != send(windowState->fd, windowState->message, windowState->message_pos, MSG_DONTWAIT)) {
    printf("failed to get wl_get_registry\n");
    exit(errno);
  }
  
  printf("-> xdg_surface@%u.get_toplevel: xdg_toplevel=%u\n", windowState->xdg_surface, windowState->current_obj_id);
  windowState->xdg_toplevel = windowState->current_obj_id;

  return windowState->current_obj_id++;
}

void wayland_xdg_toplevel_setid(wayland_windowState *windowState, char *string) {
  if (string) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;


  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->xdg_toplevel);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, 3);

  uint32_t string_length = strlen(string);
  uint16_t msg_announced_size = wayland_header_size + sizeof(string_length) + (roundup_4(string_length));

  assert(roundup_4(msg_announced_size) == msg_announced_size);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  buf_write_string(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, string, strlen(string));

  if ((int64_t)windowState->message_pos != send(windowState->fd, windowState->message, windowState->message_pos, MSG_DONTWAIT)) {
    printf("failed to set string length\n");
    exit(errno);
  }

  }
}

int wayland_wl_compositor_create_surface(wayland_windowState *windowState) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_compositor);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, wayland_wl_compositor_create_surface_opcode);

  uint16_t msg_announced_size = wayland_header_size + sizeof(windowState->current_obj_id);

  assert(roundup_4(msg_announced_size) == msg_announced_size);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->current_obj_id);


  if ((int64_t)windowState->message_pos != send(windowState->fd, windowState->message, windowState->message_pos, MSG_DONTWAIT)) {
    printf("failed to get wl_get_registry\n");
    exit(errno);
  }
  
  printf("-> wl_compositor@%u.create_surface: wl_surface=%u\n", windowState->wl_compositor, windowState->current_obj_id);
  windowState->wl_surface = windowState->current_obj_id;

  return windowState->current_obj_id++;
}

void wayland_wl_surface_commit(wayland_windowState *windowState) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_surface);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, wayland_wl_surface_commit_opcode);

  uint16_t msg_announced_size = wayland_header_size;
  assert(roundup_4(msg_announced_size) == msg_announced_size);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  if ((int64_t)windowState->message_pos != send(windowState->fd, windowState->message, windowState->message_pos, MSG_DONTWAIT)) {
    printf("failed to get wl_get_registry\n");
    exit(errno);
  }
  
  printf("-> wl_surface@%u.commit()", windowState->wl_surface);
}

void wayland_wl_surface_attach(wayland_windowState *windowState) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_surface);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, wayland_wl_surface_attach_opcode);

  uint16_t msg_announced_size = wayland_header_size + sizeof(windowState->wl_buffer) + sizeof(uint32_t) * 3;
  assert(roundup_4(msg_announced_size) == msg_announced_size);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->wl_buffer);

  uint32_t x = 0, y = 0;
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, x);
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, y);

  if ((int64_t)windowState->message_pos != send(windowState->fd, windowState->message, windowState->message_pos, 0)) {
    exit(errno);
  }

  printf("-> wl_surface@%u.attach: wl_buffer=%u\n", windowState->wl_surface, windowState->wl_buffer);

}

void wayland_xdg_wm_base_pong(wayland_windowState *windowState, uint32_t ping) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;
  
  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->xdg_wm_base);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, wayland_xdg_wm_base_pong_opcode);
  
  uint16_t msg_announced_size = wayland_header_size + sizeof(ping);
  assert(roundup_4(msg_announced_size) == msg_announced_size);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, ping);

  if ((int64_t)windowState->message_pos != send(windowState->fd, windowState->message, windowState->message_pos, 0)) { exit(errno);
  }

  printf("-> xdg_wm_base@%u.pong: ping=%u\n", windowState->xdg_wm_base, ping);
}

void wayland_xdg_surface_ack_configure(wayland_windowState *windowState, uint32_t configure) {
  memset(windowState->message, 0, MAX_MESSAGE_SIZE);
  windowState->message_pos = 0;

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, windowState->xdg_surface);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, wayland_xdg_surface_ack_configure_opcode);
  
  uint16_t msg_announced_size = wayland_header_size + sizeof(configure);
  assert(roundup_4(msg_announced_size) == msg_announced_size);
  buf_write_u16(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, msg_announced_size);

  buf_write_u32(windowState->message, &windowState->message_pos, MAX_MESSAGE_SIZE, configure);

  if ((int64_t)windowState->message_pos != send(windowState->fd, windowState->message, windowState->message_pos, 0)) {
    exit(errno);
  }

   printf("-> xdg_surface@%u.ack_configure: configure=%u\n", windowState->xdg_surface, configure);
}

int CreateSharedMemory(uint32_t size) {
  int fd = memfd_create("wayland_shared_memory", MFD_CLOEXEC);

  if (ftruncate(fd, size) == -1) {
    printf("Failed to truncate shared memory object down to size");
    exit(errno);
  }

  return fd;
}


 */
