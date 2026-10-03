#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void memdump(char *fmt, char *data, int len);

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    printf("Example 1:\n");
    int a[2] = {61810, 2026};
    memdump("ii", (char *)a, sizeof(a));

    printf("Example 2:\n");
    memdump("S", "a string", sizeof("a string"));

    printf("Example 3:\n");
    char *s = "another";
    memdump("s", (char *)&s, sizeof(s));

    struct sss {
      char *ptr;
      int num1;
      short num2;
      char byte;
      char bytes[8];
    } example;

    example.ptr = "hello";
    example.num1 = 1819438967;
    example.num2 = 100;
    example.byte = 'z';
    strcpy(example.bytes, "xyzzy");

    printf("Example 4:\n");
    memdump("pihcS", (char *)&example, sizeof(example));

    printf("Example 5:\n");
    memdump("sccccc", (char *)&example, sizeof(example));
  } else if (argc == 2) {
    // format in argv[1], up to 512 bytes of data from standard input.
    char data[512];
    int n = 0;
    memset(data, '\0', sizeof(data));
    while (n < sizeof(data)) {
      int nn = read(0, data + n, sizeof(data) - n);
      if (nn <= 0)
        break;
      n += nn;
    }
    memdump(argv[1], data, n);
  } else {
    printf("Usage: memdump [format]\n");
    exit(1);
  }
  exit(0);
}

// Print the `len` valid bytes at `data` according to the format string `fmt`.
// Each format character consumes a fixed number of bytes from `data`
// (except 'S', which consumes whatever remains).  If a character needs more
// bytes than are left, print an error for that character and stop.
void
memdump(char *fmt, char *data, int len)
{
  int pos = 0;   // offset of the next unread byte in data
  char *f;

  for (f = fmt; *f; f++) {
    int need;    // bytes required by this format character

    switch (*f) {
    case 'i': need = 4; break;   // 32-bit integer
    case 'p': need = 8; break;   // 64-bit pointer value
    case 'h': need = 2; break;   // 16-bit integer
    case 'c': need = 1; break;   // single ASCII byte
    case 's': need = 8; break;   // 64-bit pointer to a C string
    case 'S': need = 0; break;   // remaining bytes as a C string
    default:
      printf("memdump: unknown format character '%c'\n", *f);
      return;
    }

    // Bounds check (every character except 'S', which uses what is left).
    if (*f != 'S' && pos + need > len) {
      printf("memdump: not enough data for '%c'\n", *f);
      return;
    }

    switch (*f) {
    case 'i': {
      int v;
      memmove(&v, data + pos, 4);
      printf("%d\n", v);
      pos += 4;
      break;
    }
    case 'p': {
      uint64 v;
      memmove(&v, data + pos, 8);
      printf("%lx\n", v);
      pos += 8;
      break;
    }
    case 'h': {
      short v;
      memmove(&v, data + pos, 2);
      printf("%d\n", (int)v);
      pos += 2;
      break;
    }
    case 'c':
      printf("%c\n", data[pos]);
      pos += 1;
      break;
    case 's': {
      char *p;
      memmove(&p, data + pos, 8);
      printf("%s\n", p);
      pos += 8;
      break;
    }
    case 'S': {
      int i;
      // Print until the terminating null or the end of valid data.
      for (i = pos; i < len && data[i] != '\0'; i++)
        printf("%c", data[i]);
      printf("\n");
      pos = len;
      break;
    }
    }
  }
}
