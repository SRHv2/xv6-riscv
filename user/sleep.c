#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// sleep: pause the calling process for a given number of timer ticks.
// Usage: sleep <ticks>
//   <ticks> must be a non-negative integer.

int
main(int argc, char *argv[])
{
  char *p;
  int ticks;

  // Require exactly one argument (the tick count).
  if (argc != 2) {
    fprintf(2, "usage: sleep <ticks>\n");
    exit(1);
  }

  // Validate that the argument is a non-negative integer, since atoi()
  // silently returns 0 for non-numeric input and would hide the error.
  for (p = argv[1]; *p; p++) {
    if (*p < '0' || *p > '9') {
      fprintf(2, "sleep: '%s' is not a non-negative integer\n", argv[1]);
      exit(1);
    }
  }

  ticks = atoi(argv[1]);

  // pause() returns -1 if the process was killed while sleeping.
  if (pause(ticks) < 0) {
    fprintf(2, "sleep: pause failed\n");
    exit(1);
  }

  exit(0);
}
