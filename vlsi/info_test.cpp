#include "info_backend.h"
#include <mutex>
#include <condition_variable>
#include <csignal>
#include <cstdio>


static bool exit_flag = false;
static std::mutex m;
static std::condition_variable cv;

int main() {
  InfoServer info_server;
  info_server.start_work();

  signal(SIGINT, [] (int sig) {
    exit_flag = true;
    cv.notify_one();
  });
  
  std::unique_lock<std::mutex> lk(m);
  cv.wait(lk, [] () { return exit_flag; });

  info_server.stop_work();
  printf("now exit\n");
  return 0;
}
