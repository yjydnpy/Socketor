#ifndef __INFO_SERVICE_BACKEND_H__
#define __INFO_SERVICE_BACKEND_H__

#include "vlsi.h"
#include "info_endpoint.h"
#include <map>

using vlsi::Writer;
using vlsi::Mail;
using std::map;

struct InfoServer : public Writer {
  void reply(Mail *mail) {
    switch (mail->subject) {
      case GET_INFO: 
      {
        GetInfoReq req;
        GetInfoResp resp;
        req.from_string(mail->body, mail->body_len);
        get_info(&req, &resp);
        mail->emit_reply(&resp);
        break;
      }
      case SET_INFO:
      {
        SetInfoReq req;
        SetInfoResp resp;
        req.from_string(mail->body, mail->body_len);
        set_info(&req, &resp);
        mail->emit_reply(&resp);
        break;
      }
      default:
      {
        mail->emit_null();
        break;
      }
    }
    return;
  }

  map<int, char *> infos;

  void get_info(GetInfoReq *req, GetInfoResp *resp) {
    char *info = infos[req->uid];
    memcpy(resp->val, info, 16);
    resp->status = 99;
  }
  void set_info(SetInfoReq *req, SetInfoResp *resp) {
    char *info = (char *) malloc(16);
    memcpy(info, req->val, 16);
    infos[req->uid] = info;
    resp->status = 0;
  }
};


#endif /* __INFO_SERVICE_BACKEND_H__ */
