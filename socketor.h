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
  Socketor();
  Socketor* master_at(int port);
  Socketor* guest_to(char *addr, Session **s);
  Socketor* do_on_guest(GuestHandler h);
  Socketor* do_on_business(BusinessHandler h);
  Socketor* do_on_done(DoneHandler h);
  Socketor* with_mistress(int mc);
  void online();
  void offline();
  void trading();
  void mistress_trading(int mistress_id);
  void see_you(int guest);
  void see_you(Session *s);

  bool tired = false;
  int master = 0;
  int mistress_count = 0;
  int pos = 0;
  //int *mistress_plan;
  //int *mistress_book;
  int *master_mic = NULL; // master send message
  int *mistress_radio = NULL; // mistress recv message

  int *mistress_bell = NULL;
  thread **mistress_t = NULL;

  int bell;
  thread t;
  GuestHandler gh;
  BusinessHandler bh;
  DoneHandler dh;
  map<int, Session *> guest_session;
  map<int, int> guest_bell;
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
  int seek_mark();
}; /* struct Session */


#endif /* __SOCKETOR_H__ */
