#ifndef __INFO_SERVICE_ENDPOINT_H__
#define __INFO_SERVICE_ENDPOINT_H__

#include "vlsi.h"
#include "stevedore.h"

using vlsi::Container;
using vlsi::Poster;

enum INFO_SERVICE_MAIL_SUBJECT {
  GET_INFO = 1,
  SET_INFO = 2
};

struct GetInfoReq : public Container {
  int uid;
  virtual void from_string(char *c, int len = 0) {
    P(c).p(&uid);
  }
  virtual int to_string(char *c, int len) {
    return S(c).s(&uid)->finish();
  }
};

struct GetInfoResp : public Container {
  int status;
  char val[16];
  virtual void from_string(char *c, int len = 0) {
    P(c).p(&status)->p_array(val, 16);
  }
  virtual int to_string(char *c, int len) {
    return S(c).s(&status)->s_array(val, 16)->finish();
  }
};

struct SetInfoReq : public Container {
  int uid;
  char val[16];
  virtual void from_string(char *c, int len = 0) {
    P(c).p(&uid)->p_array(val, 16);
  }
  virtual int to_string(char *c, int len) {
    return S(c).s(&uid)->s_array(val, 16)->finish();
  }
};

struct SetInfoResp : public Container {
  int status;
  virtual void from_string(char *c, int len = 0) {
    P(c).p(&status);
  }
  virtual int to_string(char *c, int len) {
    return S(c).s(&status)->finish();
  }
};

struct InfoPoster : public Poster {
  InfoPoster(char *addr) : Poster(addr) {
  }

  int get_info(Container *req, Container *resp) {
    return post_mail(GET_INFO, req, resp);
  }

  int set_info(Container *req, Container *resp) {
    return post_mail(SET_INFO, req, resp);
  }

};

#endif /* __INFO_SERVICE_ENDPOINT_H__ */
