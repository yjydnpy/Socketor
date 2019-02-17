#ifndef __MY_VLSI_H__
#define __MY_VLSI_H__

#include "socketor.h"
#include "simple_queue.h"
#include <map>
#include <queue>
#include <unordered_map>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <functional>
#include <cassert>
#include <atomic>

using namespace std::placeholders;    
using std::map;
using std::unordered_map;
using std::queue;
using std::atomic_int;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using std::condition_variable;

/*
 * Mail format: len id subject body
 */

namespace vlsi {

struct Container;
struct Mail;
struct Poster;
struct Writer;


struct Container {
  virtual void from_string(char *c, int len = 0) = 0;
  virtual int to_string(char *c, int len) = 0;
};

struct Poster {
  char dest_addr[24];
  Socketor *sr = NULL;
  Session *s = NULL;
  Poster(char *addr) {
    strncpy(dest_addr, addr, 24);
    id_position.store(0);
    sr = new Socketor;
    sr->guest_to(dest_addr, &s)->do_on_business(bind(&Poster::handle_reply, this, _1));
    sr->online();
  }

  void stop() {
    sr->see_you(s);
    sr->offline();
  }

  atomic_int id_position;

  mutex cvm;
  queue<condition_variable *> cq;
  mutex rm;
  unordered_map<int, char *> replys;
  unordered_map<int, condition_variable *> cvs;

  int apply_mail_id() {
    int ret = id_position++;
    if (ret >= 100000000) {
      id_position.store(0);
    }
    return ret;
  }

  condition_variable* apply_cv(int mail_id) {
    lock_guard<mutex> l(cvm);
    condition_variable *cv;
    if (!cq.empty()) {
      cv = cq.front();
      cq.pop();
    } else {
      cv = new condition_variable;
    }
    cvs[mail_id] = cv;
    return cv;
  }

  condition_variable* cv_of(int mail_id) {
    lock_guard<mutex> l(cvm);
    condition_variable *cv = cvs[mail_id];
    cvs.erase(mail_id);
    cq.push(cv);
    return cv;
  }

  char* reply_of(int mail_id) {
    lock_guard<mutex> l(rm);
    char *ret = replys[mail_id];
    replys.erase(mail_id);
    return ret;
  }

  bool has_reply(int mail_id) {
    lock_guard<mutex> l(rm);
    return replys.find(mail_id) != replys.end();
  }

  void handle_reply(Session *s) {
    while (true) {
      int expected = s->glance_gift();
      if (expected <= 0) {
        break;
      }
      if (expected <= s->weight_gift()) {
        char *room = (char *) malloc(expected);
        s->open_gift(room, expected);
        int mail_id = *((int *)(room + 4));
        //lock_guard<mutex> l(rm);
        rm.lock();
        replys[mail_id] = room;
        rm.unlock();
        cv_of(mail_id)->notify_one();
      }
    }
  }

  int post_mail(int mail_subject, Container *req, Container *resp) {
    int mail_id = apply_mail_id();
    auto cv = apply_cv(mail_id);
    delivery_mail(mail_id, mail_subject, req);
    mutex tm;
    unique_lock<mutex> l(tm);
    cv->wait_for(l, std::chrono::seconds(1), [this, &mail_id] () { return this->has_reply(mail_id); });
    //cv->wait(l, [this, &mail_id] () { return this->has_reply(mail_id); });
    if (has_reply(mail_id)) {
      char *opaque = reply_of(mail_id);
      resp->from_string(opaque + 12);
      free(opaque);
      return 0;
    } else {
      return 1;
    }
  }

  void delivery_mail(int mail_id, int mail_subject, Container *ct) {
    char buf[1024];
    memcpy(buf + 4, &mail_id, 4);
    memcpy(buf + 8, &mail_subject, 4);
    int ct_len = ct->to_string(buf + 12, 1024);
    int len = ct_len + 12;
    memcpy(buf, &len, 4);
    s->give_gift(buf, len); 
  }

}; /* struct Poster */


//struct Courier { };

//enum MAIL_SUBJECT;

struct Mail {
  Session *s;
  int id;
  int subject;
  char *body;
  int body_len;
  Mail(Session *fs, char *c, int len) : s(fs) {
    memcpy(&id, c + 4, 4);
    memcpy(&subject, c + 8, 4);
    body = c + 12;
    body_len = len - 12;
  }

  ~Mail() {
    free(body - 12);
  }

  void emit_reply(Container *ct) {
    char buf[1024];
    memcpy(buf + 4, &id, 4);
    memcpy(buf + 8, &subject, 4);
    int resp_len = ct->to_string(buf + 12, 1024);
    int len = resp_len + 12;
    memcpy(buf, &len, 4);
    s->give_gift(buf, len);
  }

  void emit_null() {
    char buf[12];
    *((int *)buf) = 12;
    memcpy(buf + 4, &id, 4);
    memcpy(buf + 8, &subject, 4);
    s->give_gift(buf, 12);
  }
}; /* struct Mail */


struct Writer {
  bool stop = false;
  simple_queue<Mail *> mails;
  int ghoster_count = 1;
  Socketor *sr = NULL;
  thread ghoster_thread;
  void start_work() {
    sr = new Socketor();
    sr->do_on_business(std::bind(&Writer::fetch_mail, this, _1))->master_at(8999)->online();
    ghoster_thread = thread(&Writer::ghoster_work, this);
    //for (int i = 0; i < ghoster_count; i++) {
      //ghoster_thread = thread(ghoster_work);
    //}
  }

  void fetch_mail(Session *s) {
    while (true) {
      int expected = s->glance_gift();
      if (expected <= 0) break;
      printf("Fetch mail expected is %d\n", expected);
      if (expected <= s->weight_gift()) {
        char *room = (char *) malloc(expected);
        int n = s->open_gift(room, expected);
        assert(n == expected);
        Mail *m = new Mail(s, room, expected);
        printf("Receive a new mail %d \n", m->id);
        mails.push(m);
      }
    }
  }

  void ghoster_work() {
    while (true) {
      Mail *mail = NULL;
      int ret = mails.pop_or_timeout(&mail, 1);
      if (mail) {
        reply(mail);
        delete mail;
      }
      if (stop && mails.empty()) break;
    }
  }


  virtual void reply(Mail *mail) = 0;

  void stop_work() {
    sr->offline();
    stop = true;
    ghoster_thread.join();
  }
  //try_read_mail();
  //void reply_for();
}; /* struct Writer */

} /* namespace vlsi */


#endif /* __MY_VLSI_H__ */
