#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#include "user/user.h"

// find: recursively search a directory tree for entries with a given name.
//   find <path> <name>                      -- print each matching path
//   find <path> <name> -exec cmd [args...]  -- run "cmd args <path>" per match

static int do_exec = 0;       // was -exec given?
static char **exec_args = 0;  // the cmd and its fixed args
static int exec_nargs = 0;    // how many fixed args (cmd included)

// Run "cmd args... path" as a child process and wait for it.
static void
run_exec(char *path)
{
  char *args[MAXARG];
  int i, pid;

  for (i = 0; i < exec_nargs; i++)
    args[i] = exec_args[i];
  args[exec_nargs] = path;      // append the matched path
  args[exec_nargs + 1] = 0;     // argv must be null-terminated

  if ((pid = fork()) < 0) {
    fprintf(2, "find: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    exec(args[0], args);
    fprintf(2, "find: exec %s failed\n", args[0]);
    exit(1);
  }
  wait(0);
}

// Act on a match: either run the command or print the path.
static void
report(char *path)
{
  if (do_exec)
    run_exec(path);
  else
    printf("%s\n", path);
}

static void
find(char *path, char *name)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (st.type != T_DIR) {
    // A plain file or device: nothing to descend into here.
    close(fd);
    return;
  }

  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
    fprintf(2, "find: path too long\n");
    close(fd);
    return;
  }

  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
      continue;

    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;              // build the full path to this entry

    if (stat(buf, &st) < 0) {
      fprintf(2, "find: cannot stat %s\n", buf);
      continue;
    }

    if (strcmp(de.name, name) == 0)
      report(buf);

    if (st.type == T_DIR)
      find(buf, name);         // recurse into sub-directories
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if (argc < 3) {
    fprintf(2, "usage: find <path> <name> [-exec cmd [args...]]\n");
    exit(1);
  }

  if (argc >= 4) {
    if (strcmp(argv[3], "-exec") != 0) {
      fprintf(2, "find: unexpected argument '%s'\n", argv[3]);
      exit(1);
    }
    if (argc < 5) {
      fprintf(2, "find: -exec requires a command\n");
      exit(1);
    }
    do_exec = 1;
    exec_args = &argv[4];
    exec_nargs = argc - 4;
    // Need room for the fixed args, the appended path, and the null.
    if (exec_nargs + 2 > MAXARG) {
      fprintf(2, "find: too many arguments to -exec\n");
      exit(1);
    }
  }

  find(argv[1], argv[2]);
  exit(0);
}
