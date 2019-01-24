#include "socketor.h"
#include <signal.h>
#include <cstring>
#include <mutex>
#include <condition_variable>

void welcome(Session *s) {
  char *h = "Hello, guest! show me your faith\n";
  s->give_gift(h, strlen(h));
}

void echo(Session *s) {
  int gs = s->weight_gift();
  printf("gift size %d\n", gs);
  if (gs > 0) {
    char c[gs];
    s->open_gift(c, gs);
    s->give_gift(c, gs);
  }
}

bool exit_flag = false;
std::mutex m;
std::condition_variable cv;

void handle_sigint(int signo) {
  exit_flag = true;
  cv.notify_one();
}

int main() {
  Socketor *s = new Socketor();
  s->master_at(3637)->do_on_guest(welcome)->do_on_business(echo)->online();

  signal(SIGINT, handle_sigint);

  std::unique_lock<std::mutex> lk(m);
  cv.wait(lk, [] () { return exit_flag; });

  s->offline();

  printf("clear and exit\n");
  return 0;
}
