#include "store.h"

#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gpio.pb-c.h"
#include "log.h"

#define MAX_STORE_COUNT 64
#define INVALID_STORE_ID MAX_STORE_COUNT

typedef struct Store {
  EnumStoreType type;
  void *key;
  void *store;
} Store;

static bool s_initialized = false;

static Store s_stores[MAX_STORE_COUNT];

static int s_ctop_fifo;

static StoreFuncs s_func_table[ENUM_STORE_TYPE__END];

Store *prv_get_first_empty() {
  for (uint16_t i = 0; i < MAX_STORE_COUNT; i++) {
    if (s_stores[i].key == NULL && s_stores[i].store == NULL) {
      return &s_stores[i];
    }
  }
  return NULL;
}

static void prv_handle_store_update(uint8_t *buf, int64_t len) {
  MxStoreUpdate *update = mx_store_update__unpack(NULL, (size_t)len, buf);
  s_func_table[update->type].update_store(update->msg, update->mask);
  mx_store_update__free_unpacked(update, NULL);
}

// handles getting an update from python, runs as thread
static void *prv_poll_update(void *arg) {
  // read protos from stdin
  // compare using second proto as 'mask'
  // trigger gpio interrupt as necessary
  struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
  LOG_DEBUG("starting to poll\n");
  while (true) {
    int res = poll(&pfd, 1, -1);
    if (res == -1) {
      LOG_DEBUG("polling error\n");
      perror(__func__);
    } else if (res == 0) {
      continue;  // nothing to read
    } else {
      if (pfd.revents & POLLIN) {
        static uint8_t buf[4096];  // store may be bigger than this?
        int64_t len = read(STDIN_FILENO, buf, sizeof(buf));
        if (len == -1) {
          // TODO: handle read error case
        }
        LOG_DEBUG("store got buffer with len %ld\n", len);
        prv_handle_store_update(buf, len);
      } else {
        // TODO: handle POLLHUP case
      }
    }
  }
  return NULL;
}

void store_init(EnumStoreType type, StoreFuncs funcs) {
  LOG_DEBUG("initializing store\n");
  s_func_table[type] = funcs;
  if (s_initialized) {
    return;
  }

  // set up polling thread
  pthread_t poll_thread;
  pthread_create(&poll_thread, NULL, prv_poll_update, NULL);

  // set up store pool
  memset(&s_stores, 0, sizeof(s_stores));

  // open fifo
  char fifo_path[64];
  snprintf(fifo_path, sizeof(fifo_path), "/tmp/%d_ctop", getpid());
  mkfifo(fifo_path, 0666);
  s_ctop_fifo = open(fifo_path, O_WRONLY);

  s_initialized = true;
}

void store_register(EnumStoreType type, void *store, void *key) {
  // malloc a proto as a store and return a pointer to it
  Store *local_store = prv_get_first_empty();
  if (store == NULL) {
    return;
  }
  local_store->type = type;
  local_store->store = store;
  local_store->key = key;
}

void *store_get(EnumStoreType type, void *key) {
  // just linear search for it
  // if key is NULL just get first one with right type
  for (uint16_t i = 0; i < MAX_STORE_COUNT; i++) {
    if (key == NULL && s_stores[i].type == type) {
      return s_stores[i].store;
    } else if (key != NULL && s_stores[i].type == type) {
      if (s_stores[i].key == key) {
        return s_stores[i].store;
      }
    }
  }
  return NULL;
}

void store_export(EnumStoreType type, void *store, void *key) {
  // serialize to proto
  size_t packed_size = s_func_table[type].get_packed_size(store);
  uint8_t *store_buf = malloc(packed_size);
  s_func_table[type].pack(store, store_buf);

  MxStoreInfo msg = MX_STORE_INFO__INIT;
  msg.key = (uint64_t)key;
  msg.type = type;
  msg.msg.data = store_buf;
  msg.msg.len = packed_size;

  size_t export_size = mx_store_info__get_packed_size(&msg);
  uint8_t *export_buf = malloc(export_size);
  mx_store_info__pack(&msg, export_buf);

  // write proto to fifo
  ssize_t written = write(s_ctop_fifo, export_buf, export_size);
  if (written == -1) {
    // TODO: handle error
  }

  // free memory
  free(export_buf);
  free(store_buf);
}