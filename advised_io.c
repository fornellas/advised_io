#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>

static void* dlopen_handle = NULL;

static ssize_t (*libc_read_symbol)(int fd, void *buf, size_t count) = NULL;
static ssize_t (*libc_write_symbol)(int fd, const void *buf, size_t count) = NULL;

void load_original_symbols() {
  // Already loaded
  if(dlopen_handle != NULL) {
    return;
  }
  // dlopen
  dlerror();
  dlopen_handle = dlopen("libc.so.6", RTLD_LAZY);
  char *dlerror_str = NULL;
  if(dlopen_handle == NULL) {
    dlerror_str = dlerror();
    if(NULL == dlerror_str)
      dlerror_str = "Unknown error";
    fprintf(stderr, "Unable to dlopen(\"libc.so.6\"): %s\n", dlerror_str);
    abort();
  }
  // read
  dlerror();
  libc_read_symbol = dlsym(dlopen_handle, "read");
  if((dlerror_str = dlerror()) != NULL) {
    fprintf(stderr, "Unable to find symbol read(): %s\n", dlerror_str);
    abort();
  }
  // write
  dlerror();
  libc_write_symbol = dlsym(dlopen_handle, "write");
  if((dlerror_str = dlerror()) != NULL) {
    fprintf(stderr, "Unable to find symbol write(): %s\n", dlerror_str);
    abort();
  }
}

void advise_dontuse(int fd) {
  int advise_error = posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
  if(advise_error != 0) {
    char * error_str;
    switch(advise_error){
      case EBADF:
        error_str = "The fd argument was not a valid file descriptor.";
        break;
      case EINVAL:
        error_str = "An invalid value was specified for advice.";
        break;
      case ESPIPE:
        // error_str = "The  specified file descriptor refers to a pipe or FIFO.  (Linux actually returns EINVAL in this case.)";
        // break;
        return;
      default:
        error_str = "Unknown error";
    }
    fprintf(stderr, "Failed to posix_fadvise() '%d': %s\n", fd, error_str);
  }
}

ssize_t read(int fd, void *buf, size_t count) {
  load_original_symbols();
  // fprintf(stderr, "read(%d, ...)\n", fd);fflush(stderr);
  advise_dontuse(fd);
  return (*libc_read_symbol)(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
  load_original_symbols();
  // fprintf(stderr, "write(%d, ...)\n", fd);fflush(stderr);
  advise_dontuse(fd);
  return (*libc_write_symbol)(fd, (void *)buf, count);
}