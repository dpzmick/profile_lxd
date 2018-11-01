#pragma once

#define APP_ERRORS(_) \
  _(APP_ERR_ALLOC,       "failed to allocate")\
  _(APP_ERR_INVAL,       "invalid arguments")\
  _(APP_ERR_UNIMPL,      "not implemented")\
  _(APP_ERR_THREAD_JOIN, "couldn't join a thread")\
  _(APP_ERR_OPEN,        "couldn't open file")\
  _(APP_DROP,            "dropped a message")\

enum {
  APP_SUCCESS = 0,

#define ELT(e,h) e,
  APP_ERRORS(ELT)
#undef ELT
};

char const*
app_errstr(int err);
