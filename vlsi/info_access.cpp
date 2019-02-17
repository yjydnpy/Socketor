#include "info_endpoint.h"
#include <cstdio>
#include <thread>

static void client_run(InfoPoster *info_poster) {
  for (int i = 0; i < 1; i++) {
    GetInfoReq req2;
    req2.uid = 1;
    GetInfoResp resp2;
    info_poster->get_info(&req2, &resp2);
    printf("get info resp status is %d\n", resp2.status);
    printf("get info resp val is %s\n", resp2.val);
  }
};

int main() {

  InfoPoster info_poster("127.0.0.1:8999");

  SetInfoReq req;
  req.uid = 1;
  const char *name = "apple";
  strncpy(req.val, name, strlen(name) + 1);
  SetInfoResp resp;
  resp.status = 10;
  info_poster.set_info(&req, &resp);
  printf("set info resp status is %d\n", resp.status);
  
  std::thread threads[1];
  int i;
  for (i = 0; i < 1; i++) {
    threads[i] = std::thread(client_run, &info_poster);
  }
  printf("thread ok\n");
  for (i = 0; i < 1; i++) {
    if (threads[i].joinable()) {
      threads[i].join();
    }
  }

  printf("thread join ok\n");
  /*
  for (int i = 0; i < 10000; i++) {
    printf("i is %d\n", i);
    GetInfoReq req2;
    req2.uid = 1;
    GetInfoResp resp2;
    info_poster.get_info(&req2, &resp2);
    printf("get info resp status is %d\n", resp2.status);
    printf("get info resp val is %s\n", resp2.val);
  }
  */
  printf("now exit ....\n");

  info_poster.stop();
  return 0;
}
