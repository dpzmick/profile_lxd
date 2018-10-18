#pragma once

#define APP_ERRORS(_) \
  _(APP_ERR_ALLOC,  "failed to allocate")\
  _(APP_ERR_UNIMPL, "not implemented")

enum {
  APP_SUCCESS = 0,

#define ELT(e,h) e,
  APP_ERRORS(ELT)
#undef ELT
};

char const*
app_errstr(int err);
