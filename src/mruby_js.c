#include <stdio.h>
#include <string.h>

#include <emscripten/emscripten.h>

#include <mruby.h>
#include <mruby/string.h>

#define INVALID_HANDLE -1

extern void js_call(mrb_state *mrb, int handle, const char* name, mrb_value* argv, int argc);

int EMSCRIPTEN_KEEPALIVE mruby_js_argument_type(mrb_state *mrb, mrb_value* argv, int idx)
{
  return argv[idx].tt;
}

char* EMSCRIPTEN_KEEPALIVE mruby_js_get_string(mrb_state *mrb, mrb_value* argv, int idx)
{
  struct RString *s;
  /* TODO: well, let's come back to the auto-conversion later,
   * since that involves changing the mruby_js_argument_type function
   * as well.
   */
  if (mrb_type(argv[idx]) != MRB_TT_STRING) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not a string!");
  }
  s = mrb_str_ptr(argv[idx]);
  if (strlen(s->ptr) != s->len) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "String contains NULL!");
  }
  return s->ptr;
}

mrb_int EMSCRIPTEN_KEEPALIVE mruby_js_get_integer(mrb_state *mrb, mrb_value* argv, int idx)
{
  if (mrb_type(argv[idx]) != MRB_TT_FIXNUM) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not an integer!");
  }
  return mrb_fixnum(argv[idx]);
}

mrb_float EMSCRIPTEN_KEEPALIVE mruby_js_get_float(mrb_state *mrb, mrb_value* argv, int idx)
{
  if (mrb_type(argv[idx]) != MRB_TT_FLOAT) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not a float!");
  }
  return mrb_float(argv[idx]);
}

void EMSCRIPTEN_KEEPALIVE mruby_js_name_error(mrb_state *mrb)
{
  mrb_raise(mrb, E_ARGUMENT_ERROR, "Error occurs when locating the function to call!");
}

static mrb_value
mrb_js_call(mrb_state *mrb, mrb_value self)
{
  char* name = NULL;
  mrb_value *argv = NULL;
  int argc = 0;

  /* TODO: block handling */
  mrb_get_args(mrb, "z*", &name, &argv, &argc);
  if (name != NULL) {
    js_call(mrb, INVALID_HANDLE, name, argv, argc);
  }
  return self;
}

void
mrb_mruby_js_gem_init(mrb_state* mrb) {
  struct RClass *class_cextension = mrb_define_module(mrb, "MrubyJs");
  mrb_define_class_method(mrb, class_cextension, "call", mrb_js_call, ARGS_ANY());
}
