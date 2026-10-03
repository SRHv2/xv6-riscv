#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// sixfive: print every number in the input that is a multiple of 5 or 6.
// A "number" is a maximal run of decimal digits whose neighbours (on both
// sides) are separator characters or the start/end of the input.  Digits
// touching any other character (e.g. the 6 in "xv6") are not numbers.

static char *seps = " -\r\t\n./,";

// Read fd one byte at a time and print the qualifying numbers.
static void
sixfive(int fd)
{
  char c;
  int r;
  int innum = 0;    // currently inside a run of digits?
  int valid = 0;    // was the current run preceded by a separator/boundary?
  int prevsep = 1;  // was the previous char a separator? (start = implicit sep)
  int n = 0;        // value of the current run

  while ((r = read(fd, &c, 1)) == 1) {
    if (c >= '0' && c <= '9') {
      if (!innum) {         // start of a new digit run
        innum = 1;
        n = 0;
        valid = prevsep;    // left boundary ok only if preceded by a separator
      }
      n = n * 10 + (c - '0');
      prevsep = 0;          // a digit is not a separator
    } else {
      int issep = (strchr(seps, c) != 0);
      if (innum) {
        // The run ends here; it is a number only if the right neighbour
        // is also a separator.
        if (valid && issep && (n % 5 == 0 || n % 6 == 0))
          printf("%d\n", n);
        innum = 0;
      }
      prevsep = issep;
    }
  }

  // End of input is an implicit separator, so a trailing run can qualify.
  if (innum && valid && (n % 5 == 0 || n % 6 == 0))
    printf("%d\n", n);

  if (r < 0)
    fprintf(2, "sixfive: read error\n");
}

int
main(int argc, char *argv[])
{
  int i, fd;

  if (argc <= 1) {
    sixfive(0);          // no files: read standard input
    exit(0);
  }

  for (i = 1; i < argc; i++) {
    if ((fd = open(argv[i], O_RDONLY)) < 0) {
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      continue;
    }
    sixfive(fd);
    close(fd);
  }
  exit(0);
}
