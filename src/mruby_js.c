#include <stdio.h>
#include <string.h>

#include <emscripten/emscripten.h>

#include <mruby.h>
#include <mruby/class.h>
#include <mruby/string.h>
#include <mruby/variable.h>

#define INVALID_HANDLE -1

static struct RClass *mjs_mod;
static struct RClass *js_obj_cls;

/* bridge functions between JS side and C side */

int EMSCRIPTEN_KEEPALIVE mruby_js_argument_type(mrb_state *mrb, mrb_value* argv, int idx)
{
  return mrb_type(argv[idx]);
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

mrb_int EMSCRIPTEN_KEEPALIVE mruby_js_get_object_handle(mrb_state *mrb, mrb_value* argv, int idx)
{
  if (mrb_type(argv[idx]) != MRB_TT_OBJECT) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not an object!");
  }

  /* currently we only support passing objects of JsObject type */
  if (mrb_object(argv[idx])->c != js_obj_cls) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Object argument must be JsObject type!");
  }

  return mrb_fixnum(mrb_iv_get(mrb, argv[idx], mrb_intern(mrb, "handle")));
}

void EMSCRIPTEN_KEEPALIVE mruby_js_name_error(mrb_state *mrb)
{
  mrb_raise(mrb, E_ARGUMENT_ERROR, "Error occurs when locating the function to call!");
}

void EMSCRIPTEN_KEEPALIVE mruby_js_set_integer(mrb_state *mrb, mrb_value* arg, mrb_int val)
{
  *arg = mrb_fixnum_value(val);
}

void EMSCRIPTEN_KEEPALIVE mruby_js_set_float(mrb_state *mrb, mrb_value* arg, mrb_float val)
{
  *arg = mrb_float_value(val);
}

void EMSCRIPTEN_KEEPALIVE mruby_js_set_boolean(mrb_state *mrb, mrb_value* arg, int val)
{
  *arg = (val == 1) ? (mrb_true_value()) : (mrb_false_value());
}

void EMSCRIPTEN_KEEPALIVE mruby_js_set_nil(mrb_state *mrb, mrb_value* arg)
{
  *arg = mrb_nil_value();
}

void EMSCRIPTEN_KEEPALIVE mruby_js_set_string(mrb_state *mrb, mrb_value* arg, const char* val)
{
  *arg = mrb_str_new_cstr(mrb, val);
}

void EMSCRIPTEN_KEEPALIVE mruby_js_set_object_handle(mrb_state *mrb, mrb_value* arg, mrb_int handle)
{
  struct RObject *o;
  enum mrb_vtype ttype = MRB_INSTANCE_TT(js_obj_cls);
  mrb_value argv;

  if (ttype == 0) ttype = MRB_TT_OBJECT;
  o = (struct RObject*)mrb_obj_alloc(mrb, ttype, js_obj_cls);
  *arg = mrb_obj_value(o);
  argv = mrb_fixnum_value(handle);
  mrb_funcall_argv(mrb, *arg, mrb->init_sym, 1, &argv);
}

/* mrb functions */

/* See js/mruby_js.js file for the possible values of ret_allow_object */
extern void js_call(mrb_state *mrb, mrb_int handle, const char *name,
                    mrb_value *argv, int argc,
                    mrb_value* ret, int ret_allow_object,
                    int constructor_call);
extern void js_get_field(mrb_state *mrb, mrb_int handle, const char *field_name_p,
                         mrb_value *ret, int ret_allow_object);
extern void js_get_root_object(mrb_state *mrb, mrb_value *ret);
extern void js_release_object(mrb_state *mrb, mrb_int handle);

static mrb_value
mrb_js_get_root_object(mrb_state *mrb, mrb_value mod)
{
  mrb_value ret = mrb_nil_value();

  js_get_root_object(mrb, &ret);

  return ret;
}

static mrb_value
mrb_js_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_int handle = INVALID_HANDLE;

  mrb_get_args(mrb, "i", &handle);
  if (handle <= 0) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "No valid handle is provided!");
  }

  mrb_iv_set(mrb, self, mrb_intern(mrb, "handle"), mrb_fixnum_value(handle));

  return self;
}

static mrb_value
mrb_js_handle_get(mrb_state *mrb, mrb_value self)
{
  return mrb_iv_get(mrb, self, mrb_intern(mrb, "handle"));
}

static mrb_value
mrb_js_handle_set(mrb_state *mrb, mrb_value self)
{
  mrb_int handle = INVALID_HANDLE;
  mrb_get_args(mrb, "i", &handle);

  mrb_iv_set(mrb, self, mrb_intern(mrb, "handle"), mrb_fixnum_value(handle));

  return mrb_nil_value();
}

static mrb_value
mrb_js_call_with_option(mrb_state *mrb, mrb_value self, int return_value_option,
                        int constructor_call)
{
  char* name = NULL;
  mrb_value *argv = NULL;
  mrb_value handle, ret = mrb_nil_value();
  int argc = 0;
  mrb_int handle_val = INVALID_HANDLE;

  /* TODO: block handling */
  mrb_get_args(mrb, "z*", &name, &argv, &argc);
  if (name == NULL) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Field name not provided!");
  }

  handle = mrb_iv_get(mrb, self, mrb_intern(mrb, "handle"));
  if (mrb_nil_p(handle) || ((handle_val = mrb_fixnum(handle)) <= 0)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "This object does not contain a valid handle,"
              " maybe it is closed already?");
  }

  js_call(mrb, handle_val, name, argv, argc, &ret, return_value_option,
          constructor_call);
  return ret;
}

static mrb_value
mrb_js_get_with_option(mrb_state *mrb, mrb_value self, int return_value_option)
{
  char* name = NULL;
  mrb_value handle, ret = mrb_nil_value();
  mrb_int handle_val = INVALID_HANDLE;

  mrb_get_args(mrb, "z", &name);
  if (name == NULL) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Field name not provided!");
  }

  handle = mrb_iv_get(mrb, self, mrb_intern(mrb, "handle"));
  if (mrb_nil_p(handle) || ((handle_val = mrb_fixnum(handle)) <= 0)) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "This object does not contain a valid handle,"
              " maybe it is closed already?");
  }

  js_get_field(mrb, handle_val, name, &ret, return_value_option);
  return ret;
}

static mrb_value
mrb_js_call(mrb_state *mrb, mrb_value self)
{
  /* Both primitive values and objects are allowed */
  return mrb_js_call_with_option(mrb, self, 2, 0);
}

static mrb_value
mrb_js_call_primitive(mrb_state *mrb, mrb_value self)
{
  /* Only primitive values are allowed */
  return mrb_js_call_with_option(mrb, self, 0, 0);
}

static mrb_value
mrb_js_call_object(mrb_state *mrb, mrb_value self)
{
  /* Only objects are allowed */
  return mrb_js_call_with_option(mrb, self, 1, 0);
}

static mrb_value
mrb_js_call_constructor(mrb_state *mrb, mrb_value self)
{
  /* Only objects are allowed */
  return mrb_js_call_with_option(mrb, self, 1, 1);
}

static mrb_value
mrb_js_get(mrb_state* mrb, mrb_value self)
{
  return mrb_js_get_with_option(mrb, self, 2);
}

static mrb_value
mrb_js_get_primitive(mrb_state* mrb, mrb_value self)
{
  return mrb_js_get_with_option(mrb, self, 0);
}

static mrb_value
mrb_js_get_object(mrb_state* mrb, mrb_value self)
{
  return mrb_js_get_with_option(mrb, self, 1);
}

static mrb_value
mrb_js_close(mrb_state *mrb, mrb_value self)
{
  mrb_value handle = mrb_iv_get(mrb, self, mrb_intern(mrb, "handle"));

  js_release_object(mrb, mrb_fixnum(handle));

  mrb_iv_set(mrb, self, mrb_intern(mrb, "handle"), mrb_fixnum_value(INVALID_HANDLE));

  return mrb_nil_value();
}

void
mrb_mruby_js_gem_init(mrb_state* mrb) {
  mjs_mod = mrb_define_module(mrb, "MrubyJs");
  js_obj_cls = mrb_define_class_under(mrb, mjs_mod, "JsObject", mrb->object_class);

  mrb_define_class_method(mrb, mjs_mod, "get_root_object", mrb_js_get_root_object, ARGS_NONE());
  mrb_define_method(mrb, js_obj_cls, "initialize", mrb_js_initialize, ARGS_REQ(1));
  mrb_define_method(mrb, js_obj_cls, "handle", mrb_js_handle_get, ARGS_NONE());
  mrb_define_method(mrb, js_obj_cls, "handle=", mrb_js_handle_set, ARGS_REQ(1));
  mrb_define_method(mrb, js_obj_cls, "call", mrb_js_call, ARGS_ANY());
  mrb_define_method(mrb, js_obj_cls, "call_primitive", mrb_js_call_primitive, ARGS_ANY());
  mrb_define_method(mrb, js_obj_cls, "call_object", mrb_js_call_object, ARGS_ANY());
  mrb_define_method(mrb, js_obj_cls, "call_constructor", mrb_js_call_constructor, ARGS_ANY());
  mrb_define_method(mrb, js_obj_cls, "get", mrb_js_get, ARGS_REQ(1));
  mrb_define_method(mrb, js_obj_cls, "get_primitive", mrb_js_get_primitive, ARGS_REQ(1));
  mrb_define_method(mrb, js_obj_cls, "get_object", mrb_js_get_object, ARGS_REQ(1));
  mrb_define_method(mrb, js_obj_cls, "close", mrb_js_close, ARGS_NONE());
}
