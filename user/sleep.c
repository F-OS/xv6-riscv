#include "user/user.h"
int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(2, "usage: sleep <seconds>\n");
    exit(1);
  }

  int seconds = atoi(argv[1]);
  if (seconds < 0) {
    fprintf(2, "sleep: invalid time %s\n", argv[1]);
    exit(1);
  }

  sleep(seconds * 100); // Convert seconds to milliseconds
  exit(0);
}
