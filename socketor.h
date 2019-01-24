#ifndef __SOCKETOR_H__
#define __SOCKETOR_H__

#include <cstdlib>
#include <cstdint>
#include <map>
#include <functional>
#include <thread>
#include <unistd.h>

using std::map;
using std::thread;

struct GuestInfo;
struct Session;
struct Socketor;

using GuestHandler = std::function<void(Session *)>;
using BusinessHandler = std::function<void(Session *)>;
using DoneHandler = std::function<void(Session *)>;

#define buf_size 4096

struct GuestInfo {
  char ip[20];
  int port;
};

struct Socketor {
  Socketor* master_at(int port);
  Socketor* guest_to() {}
  Socketor* do_on_guest(GuestHandler h);
  Socketor* do_on_business(BusinessHandler h);
  Socketor* do_on_done(DoneHandler h);
  void online();
  void offline();
  void trading();
  void see_you(int guest);

  bool tired = false;
  int master;
  int bell;
  thread t;
  GuestHandler gh;
  BusinessHandler bh;
  DoneHandler dh;
  map<int, Session *> guest_session;
}; /* struct Socketor */

struct Session {
  Session(int gs) : guest(gs) {
    buf = (char *) malloc(buf_size);
    bs = buf_size;
  }
  ~Session() {
    close(guest);
    if (buf) free(buf);
  }
  int guest;
  GuestInfo gi;
  char *buf = NULL;
  int bs = 0;
  int bf = 0;
  int bt = 0;
  int weight_gift();
  int glance_gift();
  int save_gift();
  int open_gift(char* gift, int gz);
  int give_gift(char *gb, int gz);
}; /* struct Session */


#endif /* __SOCKETOR_H__ */
