#include "user/user.h"
int main(int argc, char *argv[]) {
  int ping_pipe[2];
  int pong_pipe[2];
  if (pipe(ping_pipe) < 0 || pipe(pong_pipe) < 0) {
    printf("pipe failed\n");
    exit(1);
  }
  int pid = fork();
  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }
  if (pid == 0) { // child
    int b = 'p';
    int whoami = getpid();
    int res = read(ping_pipe[0], &b, sizeof(b));
    printf("%d: recieved ping %d\n", whoami, b);
    write(pong_pipe[1], &b, sizeof(b));
    exit(1);
  } else // parent
  {
    int b = 'p';
    write(ping_pipe[1], &b, sizeof(b));
    int whoami = getpid();
    int res = read(pong_pipe[0], &b, sizeof(b));
    printf("%d: recieved pong %d\n", whoami, b);
    exit(1);
  }
  exit(0);
}
