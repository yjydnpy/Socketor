#include "socketor.h"
#include <signal.h>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <functional>
#include <map>

using namespace std::placeholders;    

struct SHS;
struct WebCache;
struct Client;
struct Req;

#define FATAL_ERROR -1
#define NOT_COMPLETE 0

#define GET_METHOD 1
#define POST_METHOD 2

struct ProtocolBuffer {
  int method;
  char *uri;
  int uri_len;
  char *param;
  int param_len;
  char *body;
  int body_len;
};

static int parse_http_requset(char *buf, int size, ProtocolBuffer *p) {
  //assert(size > 4);
  int c = 0;

  if (*(buf + 3) == ' ') {
    p->method = GET_METHOD;
    c = 4;
  } else if (*(buf + 4) == ' ') {
    p->method = POST_METHOD;
    c = 5;
  } else {
    return FATAL_ERROR;
  }

  assert(*(buf + c) == '/');

  p->uri = buf + c;
  while (*(buf + c) != ' ') {
    c++;
    if (c >= size) 
      return NOT_COMPLETE;
  }
  p->uri_len = buf + c - p->uri;

  while (++c) {
    if (c >= size) return NOT_COMPLETE;
    // "Content-Length: 10\r\n"
    if (*(buf + c) == 'C' && *(buf + c + 8) == 'L' and *(buf + c + 14) == ':') {
      c += 16;
      p->body_len = strtol(buf + c, NULL, 0);
    }
    if (*(buf + c) == '\r' && *(buf + c + 1) == '\n'
        && *(buf + c + 2) == '\r' && *(buf + c + 3) == '\n') {
      c += 3;
      if (p->method == GET_METHOD) {
        assert(c + 1 == size);
        return c + 1;
      } else {
        assert(size - c - 1 == p->body_len);
        p->body = buf + c + 1;
        return size;
      }
    }
  }

  return NOT_COMPLETE;
}

#define ROOM_SPACE 4094

struct Client {
  Client(Session *sn) : s(sn) {
  }
  ~Client() {
  }
  void fill_room() {
    int gs = s->weight_gift();
    int cc = ROOM_SPACE - filling_size;
    if (cc < gs) {
      printf("fatal error cc[%d], gs[%d]\n", cc, gs);
      gs = cc;
    }
    s->open_gift(room + filling_size, gs);
    filling_size += gs;
  }
  void send(char *c, int size) {
    s->give_gift(c, size);
  }

  Session *s;
  ProtocolBuffer pb;
  char room[ROOM_SPACE];
  int filling_size = 0;
};

//void* get_header_filed(void);

enum HttpStatus {
  HTTP_OK = 0,
  HTTP_BAD_REQUEST,
  HTTP_NOT_FOUND,
  HTTP_INTERNAL_ERROR
};

static const char* http_status_line[] = {
  "HTTP/1.1 200 OK\r\n",
  "HTTP/1.1 400 Bad Request\r\n",
  "HTTP/1.1 404 Not Found\r\n",
  "HTTP/1.1 500 Internal Server Error\r\n"
};

struct Req {
  Req(Client *cp, ProtocolBuffer *p) : c(cp), pb(p) {}
  ~Req() {
    if (rep) delete rep;
  }
  Client *c;
  ProtocolBuffer *pb;

  struct Resp {
    Resp(Client *cp) : tc(cp) {}
    Client *tc;
    char content[1024];
    int content_size = 0;
    char header[128];
    int header_size = 0;
    HttpStatus status = HTTP_OK;

    Resp* with_content(char *mb, int size) {
      memcpy(content, mb,  size);
      content_size += size;
      return this;
    }
    Resp* with_header(char *mb, int size) {
      memcpy(header, mb, size);
      header_size += size;
      return this;
    }
    Resp* with_status(HttpStatus code) {
      status = code;
      return this;
    }
    void emit() {
      const char *mb = http_status_line[status];
      int ms = strlen(mb);
      tc->send((char *)mb, ms);
      int sn = snprintf(header + header_size, 50, "Content-Length: %d\r\n\r\n", content_size);
      header_size += sn;
      tc->send(header, header_size);
      tc->send(content, content_size);
    }
  };

  Resp *rep = NULL;

  Resp* resp() {
    if (!rep) rep = new Resp(c);
    return rep;
  }

};

/* SHS: smallest http system */
struct SHS {
  Socketor *sr = NULL;
  map<Session*, Client*> clients;

  virtual void create_client(Session *s);
  virtual void try_deal(Session *s);
  virtual void give_up_client(Client *c);

  virtual void handle_req(Req *r) = 0;

};

void SHS::create_client(Session *s) {
  Client *c = new Client(s);
  clients[s] = c;
}
void SHS::give_up_client(Client *c) {
  clients.erase(c->s);
  sr->see_you(c->s);
  delete c;
}
/*void SHS::client_give_up(Client *c) {
}*/

void SHS::try_deal(Session *s) {
  Client *c = clients[s];
  c->fill_room();
  //c->s->open_gift(c-room, init
  int np = parse_http_requset(c->room, c->filling_size, &c->pb);
  if (np > 0) {
    c->filling_size -= np;
    assert(c->filling_size == 0);
    Req r(c, &c->pb);
    handle_req(&r);
    //give_up_client(c);
  } else if (np == 0) {
    printf("no complete\n");
  } else {
    give_up_client(c);
  }
};

static void println(const char *sb, int len) {
  write(0, sb, len);
  write(0, "\n", 1);
}

struct WebCache : public SHS {
  WebCache() {
    sr = new Socketor();
  }
  void startup() {
    sr->master_at(8089)->do_on_guest(std::bind(&WebCache::create_client, this, _1))
                       ->do_on_business(std::bind(&WebCache::try_deal, this, _1))
                       ->online();
  }
  void stop() {
    sr->offline();
  }
  void handle_req(Req *r) {
    //printf("in handle req\n");
    ProtocolBuffer *p = r->pb;
    printf("req method is %d\n", p->method);
    if (p->uri) {
      println(p->uri, p->uri_len);
    }
    if (p->body) {
      println(p->body, p->body_len);
    }
    char *raw = "HTTP/1.1 200 OK\r\n"
         "Connection: close\r\n"
         "Content-Length: 4\r\n"
         "\r\n"
         "body";
    //r->resp_with(raw, strlen(raw));
    char *h = "Connection: Keep-Alive\r\n";
    char *ct = "You Success!\n";

    if (memcmp(p->uri, "/query", 6) == 0) {
      int query_id = 0;
      int c = 6;
      bool found = false;
      while (c++ < p->uri_len) {
        if (*(p->uri + c) == 'i' && *(p->uri + c + 1) == 'd' && *(p->uri + c + 2) == '=') {
          c += 3;
          found = true;
          break;
        }
      }
      char *query_result = "Not supply id";
      if (found) {
        query_id = strtol(p->uri + c, NULL, 0);
        query_result = "Unkonwn id";
        if (cache.find(query_id) != cache.end()) {
          query_result = cache[query_id];
        }
      }
      r->resp()->with_status(HttpStatus::HTTP_OK)->with_header(h, strlen(h) )
               ->with_content(query_result, strlen(query_result))->emit();
    } else if (memcmp(p->uri, "/store", 6) == 0) {
      int id = 0;
      char *value = NULL;
      int c = 6;
      bool found = false;
      while (c++ < p->uri_len) {
        if (*(p->uri + c) == 'i' && *(p->uri + c + 1) == 'd' && *(p->uri + c + 2) == '=') {
          c += 3;
          found = true;
          break;
        }
      }
      char *store_result = "Not supply id";
      if (found) {
        id = strtol(p->uri + c, NULL, 0);
        value = (char *) malloc(p->body_len + 1);
        memcpy(value, p->body, p->body_len);
        *(value + p->body_len) = '\0';
        cache[id] = value;
        store_result = "OK";
      }
      r->resp()->with_status(HttpStatus::HTTP_OK)->with_header(h, strlen(h) )
               ->with_content(store_result, strlen(store_result))->emit();

    } else {
      char *result = "Unsupported path";
      r->resp()->with_status(HttpStatus::HTTP_NOT_FOUND)->with_header(h, strlen(h) )
               ->with_content(result, strlen(result))->emit();
    }
  }
  map<int, char *> cache;
};

static bool exit_flag = false;
static std::mutex m;
static std::condition_variable cv;

int main() {
  WebCache wc;
  wc.startup();

  signal(SIGINT, [] (int sig) {
    exit_flag = true;
    cv.notify_one();
  });

  std::unique_lock<std::mutex> lk(m);
  cv.wait(lk, [] () { return exit_flag; });

  wc.stop();
  return 0;
}

