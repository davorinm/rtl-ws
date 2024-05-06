#include <stddef.h>
#include <string.h>
#include <time.h>


static const struct packed_file {
  const char *name;
  const unsigned char *data;
  size_t size;
  time_t mtime;
} packed_files[] = {
  {NULL, NULL, 0, 0}
};

static int scmp(const char *a, const char *b) {
  while (*a && (*a == *b)) a++, b++;
  return *(const unsigned char *) a - *(const unsigned char *) b;
}
const char *mg_unlist(size_t no);
const char *mg_unlist(size_t no) {
  return packed_files[no].name;
}
const char *mg_unpack(const char *path, size_t *size, time_t *mtime);
const char *mg_unpack(const char *name, size_t *size, time_t *mtime) {
  const struct packed_file *p;
  for (p = packed_files; p->name != NULL; p++) {
    if (scmp(p->name, name) != 0) continue;
    if (size != NULL) *size = p->size - 1;
    if (mtime != NULL) *mtime = p->mtime;
    return (const char *) p->data;
  }
  return NULL;
}
