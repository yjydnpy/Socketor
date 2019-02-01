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

static bool exit_flag = false;
static std::mutex m;
static std::condition_variable cv;

int main() {
  Socketor *sr = new Socketor();
  sr->master_at(3637)->with_mistress(4)->do_on_guest(welcome)->do_on_business(echo)->online();


  signal(SIGINT, [] (int sig) {
    exit_flag = true;
    cv.notify_one();
  });

  std::unique_lock<std::mutex> lk(m);
  cv.wait(lk, [] () { return exit_flag; });

  sr->offline();

  printf("clear and exit\n");
  return 0;
}
