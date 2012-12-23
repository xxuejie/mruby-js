#include <mruby.h>
#include <stdio.h>

extern void js_func();

static mrb_value
mrb_call_js(mrb_state *mrb, mrb_value self)
{
  js_func();
  return self;
}

void
mrb_mruby_js_gem_init(mrb_state* mrb) {
  struct RClass *class_cextension = mrb_define_module(mrb, "MrubyJs");
  mrb_define_class_method(mrb, class_cextension, "call_js", mrb_call_js, ARGS_NONE());
}
