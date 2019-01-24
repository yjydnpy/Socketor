#include "socketor.h"
#include <cstdio>
#include <cstring>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>  
#include <errno.h>

#define events_size 100

static void cannot_wait(int s) {
  int opts = fcntl(s, F_GETFL);
  opts = opts | O_NONBLOCK;
  fcntl(s, F_SETFL, opts);
}

static void auto_greeting(int s) {
  int val = 1;
  setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
}

static int master_prepare(int port) {
  const char *host = "0:0:0:0";
  int s = socket(AF_INET, SOCK_STREAM, 0);
  int on = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  cannot_wait(s);
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  bind(s, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
  listen(s, 10);
  return s;
}

static int meeting_guest(int master, GuestInfo *gi = NULL) {
  int try_count = 5;
  struct sockaddr addr;
  socklen_t addrlen;
  int guest = -1;
  while (try_count--) {
    guest = accept(master, (struct sockaddr *) &addr, &addrlen);
    if (guest == -1) {
      if (errno == EINTR) continue;
      if (errno == EWOULDBLOCK) break;
      else break;
    } else {
      cannot_wait(guest);
      auto_greeting(guest);
      if (gi) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        inet_ntop(AF_INET, (void*) &(s->sin_addr), gi->ip, 20);
        gi->port = ntohs(s->sin_port);
      }
      break;
    }
  }
  return guest;
}

Socketor* Socketor::master_at(int port) {
  master = master_prepare(port);
  bell = epoll_create1(0);
  printf("bell is %d\n", bell);
  struct epoll_event ev;
  ev.data.fd = master;
  ev.events |= EPOLLIN;
  epoll_ctl(bell, EPOLL_CTL_ADD, master, &ev);
  return this;
}

Socketor* Socketor::do_on_business(BusinessHandler h) {
  bh = h;
  return this;
}

Socketor* Socketor::do_on_done(DoneHandler h) {
  dh = h;
  return this;
}

void Socketor::online() {
  t = thread(&Socketor::trading, this); 
}

void Socketor::offline() {
  tired = true;
  t.join();
}

void Socketor::trading() {
  int i;
  struct epoll_event ee[events_size];
  while (!tired) {
    int c = epoll_wait(bell, ee, events_size, 1000);
    if (c) printf("wait end %d\n", c);
    for (i = 0; i < c; i++) {
      struct epoll_event *e = &ee[i];
      if (e->data.fd == master) {
        int guest = meeting_guest(master);
        Session *s = new Session(guest);
        guest_session[guest] = s;
        struct epoll_event ev;
        ev.data.fd = guest;
        ev.events |= EPOLLIN;
        epoll_ctl(bell, EPOLL_CTL_ADD, guest, &ev);
        gh(s);
      } else {
        int guest = e->data.fd;
        Session *s = guest_session[guest];
        int g = s->save_gift();
        if (g == 0) {
          printf("close by remote\n");
          if (dh) dh(s);
          see_you(guest);
        } else {
          bh(s);
        }
      }
    }
  }
}

void Socketor::see_you(int guest) {
  struct epoll_event ev;
  ev.data.fd = guest;
  ev.events |= EPOLLIN;
  epoll_ctl(bell, EPOLL_CTL_DEL, guest, &ev);
  Session *s = guest_session[guest];
  guest_session.erase(guest);
  delete s;
}


int Session::weight_gift() {
  int w;
  if (bf == bt) w = 0;
  else if (bf < bt) w = bt - bf;
  else w = bs - bf + bt;
  return w;
}

int Session::glance_gift() {
  if (weight_gift() < 4) return -1;
  int r;
  memcpy(&r, buf + bf, 4);
  return r;
}

int Session::save_gift() {
  int read_size = 0;
  while (true) {
    if (bf > bt) {
      int max = bf - bt - 1;
      if (max == 0) return -1;
      int nread = read(guest, buf + bt, max);
      if (nread == 0) return 0;
      if (nread == -1) return read_size != 0 ? read_size : -1;
      bt += nread;
      read_size += nread;
      return read_size;

    } else if (bf == bt) {
      // buf is empty
      int max = bs - bt;
      if (bt == 0) max--;
      int nread = read(guest, buf+bt, max);
      if (nread == 0) return 0;
      if (nread == -1) return read_size == 0 ? -1 : read_size;
      if (nread < max) {
        read_size += nread;
        bt += nread;
        return read_size;
      } else {
        read_size += nread;
        bt += nread;

        if (bt > bs - 1) {
          bt -= bs;
        }

      }
    } else {
      int max = bs - bt;
      if (max == 1 && bf == 0) return read_size == 0 ? -1 : read_size;
      int nread = read(guest, buf+bt, max);
      if (nread == 0) return 0;
      if (nread == -1) return read_size == 0 ? -1 : read_size;
      if (nread < max) {
        bt += nread;
        read_size += nread;
        return read_size;
      } else {
        read_size += nread;
        bt = 0;
      }
    }
  }

  return read_size;
}

int Session::open_gift(char *gift, int gz) {
  if (weight_gift() < gz) return -1;
  if (bs - bf >= gz) {
    memcpy(gift, buf + bf, gz);
    bf += gz;
  } else {
    int p1 = bs - bf;
    memcpy(gift, buf + bf, p1);
    int p2 = gz - p1;
    memcpy(gift + p1, buf, p2);
    bf = p2;
  }
  return gz;
}

int Session::give_gift(char *gb, int gz) {
  return send(guest, gb, gz, 0);
}
