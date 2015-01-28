#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <emscripten/emscripten.h>

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/proc.h>
#include <mruby/string.h>
#include <mruby/value.h>
#include <mruby/variable.h>

#define INVALID_HANDLE -1

/* JS functions */
extern void js_invoke(mrb_state *mrb, mrb_value *this_value,
                      mrb_int func_handle,
                      mrb_value *argv, int argc,
                      mrb_value *ret, int type);
extern void js_get_field(mrb_state *mrb, mrb_value *obj_p,
                         mrb_value *field_p, mrb_value *ret);
extern void js_set_field(mrb_state *mrb, mrb_value *obj_p,
                         mrb_value *field_p, mrb_value *val_p);
extern void js_get_root_object(mrb_state *mrb, mrb_value *ret);
extern void js_release_object(mrb_state *mrb, mrb_int handle);
extern void js_create_array(mrb_state *mrb, const mrb_value *arr_p,
                            int len, mrb_value *ret);
extern void js_create_empty_object(mrb_state *mrb, mrb_value *ret);

static struct RClass *mjs_mod;
static struct RClass *js_obj_cls;
static struct RClass *js_func_cls;
static struct RClass *js_array_cls;

/* Object handle is stored as RData in mruby to leverage auto-dfree calls,
 * in other words, we are simulating finalizers here.
 */
static void
mruby_js_object_handle_free(mrb_state *mrb, void *p)
{
  mrb_int *handle = (mrb_int*) p;
  if (handle) {
    js_release_object(mrb, *handle);
  }
  free(p);
}

static const struct mrb_data_type mruby_js_object_handle_type ={
  "mruby_js_object_handle", mruby_js_object_handle_free
};

/* Gets the object handle value from a JsObject, it is put here since
 * mruby_js_set_object_handle uses this function.
 */
static mrb_int
mruby_js_get_object_handle_value(mrb_state *mrb, mrb_value js_obj)
{
  mrb_value value_handle;
  mrb_int *handle_p = NULL;

  value_handle = mrb_iv_get(mrb, js_obj, mrb_intern_cstr(mrb, "handle"));
  Data_Get_Struct(mrb, value_handle, &mruby_js_object_handle_type, handle_p);
  if (handle_p == NULL) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Cannot get handle value!");
  }

  return *handle_p;
}

/* bridge functions between JS side and C side */
int mruby_js_argument_type(mrb_state *mrb, mrb_value *argv, int idx)
{
  enum mrb_vtype t = mrb_type(argv[idx]);
  switch (t) {
    case MRB_TT_FALSE:
      if (mrb_nil_p(argv[idx])) {
        return 6;
      } else {
        return 0;
      }
    case MRB_TT_TRUE:
      return 1;
    case MRB_TT_FIXNUM:
      return 2;
    case MRB_TT_FLOAT:
      return 3;
    case MRB_TT_OBJECT:
      return 4;
    case MRB_TT_STRING:
      return 5;
    case MRB_TT_PROC:
      /* 6 is used by nil */
      return 7;
    case MRB_TT_ARRAY:
      return 8;
    case MRB_TT_HASH:
      return 9;
    case MRB_TT_SYMBOL:
      return 10;
    default:
      mrb_raisef(mrb, E_ARGUMENT_ERROR,
                 "Given type %d is not supported in JavaScript!\n", t);
  }
  /* This is not reachable */
  return -1;
}

mrb_int mruby_js_get_string_len(mrb_state *mrb, mrb_value *argv, int idx)
{
  struct RString *s;
  /* This comes in pair with mruby_js_get_string_ptr, skip type check here */
  s = mrb_str_ptr(argv[idx]);
  return RSTR_LEN(s);
}

char* mruby_js_get_string_ptr(mrb_state *mrb, mrb_value *argv, int idx)
{
  struct RString *s;
  if (mrb_type(argv[idx]) != MRB_TT_STRING) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not a string!");
  }
  s = mrb_str_ptr(argv[idx]);
  return RSTR_PTR(s);
}

mrb_int mruby_js_get_integer(mrb_state *mrb, mrb_value *argv, int idx)
{
  if (mrb_type(argv[idx]) != MRB_TT_FIXNUM) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not an integer!");
  }
  return mrb_fixnum(argv[idx]);
}

mrb_float mruby_js_get_float(mrb_state *mrb, mrb_value *argv, int idx)
{
  if (mrb_type(argv[idx]) != MRB_TT_FLOAT) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not a float!");
  }
  return mrb_float(argv[idx]);
}

mrb_int mruby_js_get_object_handle(mrb_state *mrb, mrb_value *argv, int idx)
{
  if (mrb_type(argv[idx]) != MRB_TT_OBJECT) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not an object!");
  }

  return mruby_js_get_object_handle_value(mrb, argv[idx]);
}

struct RProc* mruby_js_get_proc(mrb_state *mrb, mrb_value *argv, int idx)
{
  if (mrb_type(argv[idx]) != MRB_TT_PROC) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not a proc!");
  }

  /* save proc in a global array to avoid GC */
  mrb_funcall_argv(mrb, mrb_obj_value(mjs_mod),
                   mrb_intern_cstr(mrb, "add_proc"),
                   1, &argv[idx]);

  return mrb_proc_ptr(argv[idx]);
}

mrb_int mruby_js_get_array_handle(mrb_state *mrb, mrb_value *argv, int idx)
{
  mrb_value js_array;

  if (mrb_type(argv[idx]) != MRB_TT_ARRAY) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not an array!");
  }

  js_array = mrb_funcall_argv(mrb, argv[idx], mrb_intern_cstr(mrb, "toJsArray"),
                              0, NULL);

  return mruby_js_get_object_handle(mrb, &js_array, 0);
}

mrb_int mruby_js_get_hash_handle(mrb_state *mrb, mrb_value *argv, int idx)
{
  mrb_value js_object;

  if (mrb_type(argv[idx]) != MRB_TT_HASH) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not a hash!");
  }

  js_object = mrb_funcall_argv(mrb, argv[idx], mrb_intern_cstr(mrb, "toJsObject"),
                               0, NULL);

  return mruby_js_get_object_handle(mrb, &js_object, 0);
}

void mruby_js_convert_symbol_to_string(mrb_state *mrb, mrb_value *argv, int idx)
{
  mrb_value str;

  if (mrb_type(argv[idx]) != MRB_TT_SYMBOL) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Given argument is not a symbol!");
  }

  str = mrb_funcall_argv(mrb, argv[idx], mrb_intern_cstr(mrb, "to_s"),
                         0, NULL);
  if (mrb_type(str) != MRB_TT_STRING) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Failed to convert symbol to string!");
  }

  argv[idx] = str;
}

/* Invoke helper functions */
mrb_value* mruby_js_invoke_alloc_argv(mrb_state *mrb, int argc)
{
  return (mrb_value*) malloc(argc * sizeof(mrb_value));
}

mrb_value* mruby_js_invoke_fetch_argp(mrb_state *mrb, mrb_value* argv, int idx)
{
  return &argv[idx];
}

void mruby_js_invoke_release_argv(mrb_state *mrb, mrb_value* argv)
{
  free(argv);
}

/* invoke proc from js side */
void mruby_js_invoke_proc(mrb_state *mrb, struct RProc *proc,
                          int argc, mrb_value *argv, mrb_value* retp)
{
  mrb_value p = mrb_obj_value(proc);
  mrb_value ret = mrb_nil_value();

  ret = mrb_yield_argv(mrb, p, argc, argv);
  if (mrb->exc) {
    mrb_p(mrb, mrb_obj_value(mrb->exc));
  }

  if (retp) {
    *retp = ret;
  }

  /* Only for statistics and GC purpose */
  mrb_funcall_argv(mrb, mrb_obj_value(mjs_mod),
                   mrb_intern_cstr(mrb, "call_proc"),
                   1, &p);
}

void mruby_js_name_error(mrb_state *mrb)
{
  mrb_raise(mrb, E_ARGUMENT_ERROR, "Error occurs when locating the function to call!");
}

void mruby_js_set_integer(mrb_state *mrb, mrb_value *arg, mrb_int val)
{
  *arg = mrb_fixnum_value(val);
}

void mruby_js_set_float(mrb_state *mrb, mrb_value *arg, mrb_float val)
{
  *arg = mrb_float_value(mrb, val);
}

void mruby_js_set_boolean(mrb_state *mrb, mrb_value *arg, int val)
{
  *arg = (val == 1) ? (mrb_true_value()) : (mrb_false_value());
}

void mruby_js_set_nil(mrb_state *mrb, mrb_value *arg)
{
  *arg = mrb_nil_value();
}

void mruby_js_set_string(mrb_state *mrb, mrb_value *arg, const char *val)
{
  *arg = mrb_str_new_cstr(mrb, val);
}

void mruby_js_set_object_handle(mrb_state *mrb, mrb_value *arg, mrb_int handle)
{
  mrb_value argv = mrb_fixnum_value(handle);
  *arg = mrb_obj_new(mrb, js_obj_cls, 1, &argv);
}

void mruby_js_set_array_handle(mrb_state *mrb, mrb_value *arg, mrb_int handle)
{
  mrb_value argv = mrb_fixnum_value(handle);
  *arg = mrb_obj_new(mrb, js_array_cls, 1, &argv);
}

void mruby_js_set_function_handle(mrb_state *mrb, mrb_value *arg,
                                  mrb_int handle, mrb_value *parent)
{
  mrb_value argv[2];
  argv[0] = mrb_fixnum_value(handle);
  if (parent != NULL) {
    argv[1] = *parent;
  } else {
    argv[1] = mrb_nil_value();
  }

  *arg = mrb_obj_new(mrb, js_func_cls, 2, argv);
}

/* mrb functions */

static mrb_value
mrb_js_get_root_object(mrb_state *mrb, mrb_value mod)
{
  mrb_sym root_sym = mrb_intern_cstr(mrb, "ROOT_OBJECT");
  mrb_value ret = mrb_iv_get(mrb, mod, root_sym);
  if (!mrb_nil_p(ret)) {
    return ret;
  }

  js_get_root_object(mrb, &ret);
  if (!mrb_nil_p(ret)) {
    /* Cache root object to ensure singleton */
    mrb_iv_set(mrb, mod, root_sym, ret);
  }

  return ret;
}

static mrb_value
mrb_js_obj_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_int handle = INVALID_HANDLE;
  mrb_int *handle_p;

  mrb_get_args(mrb, "i", &handle);
  if (handle <= 0) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "No valid handle is provided!");
  }

  handle_p = (mrb_int*) malloc(sizeof(mrb_int));
  if (handle_p == NULL) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "Cannot allocate memory!");
  }
  *handle_p = handle;
  mrb_iv_set(mrb, self, mrb_intern_cstr(mrb, "handle"), mrb_obj_value(
      Data_Wrap_Struct(mrb, mrb->object_class,
                       &mruby_js_object_handle_type, (void*) handle_p)));

  return self;
}

static mrb_value
mrb_js_obj_handle(mrb_state *mrb, mrb_value self)
{
  return mrb_fixnum_value(mruby_js_get_object_handle_value(mrb, self));
}

static mrb_value
mrb_js_obj_get(mrb_state *mrb, mrb_value self)
{
  mrb_value field;
  mrb_value ret = mrb_nil_value();

  mrb_get_args(mrb, "o", &field);

  js_get_field(mrb, &self, &field, &ret);
  return ret;
}

static mrb_value
mrb_js_obj_set(mrb_state *mrb, mrb_value self)
{
  mrb_value field, val;

  mrb_get_args(mrb, "oo", &field, &val);

  js_set_field(mrb, &self, &field, &val);
  return mrb_nil_value();
}

static mrb_value
mrb_js_obj_create(mrb_state *mrb, mrb_value self)
{
  mrb_value ret = mrb_nil_value();

  js_create_empty_object(mrb, &ret);
  return ret;
}

static mrb_value
mrb_js_array_create(mrb_state *mrb, mrb_value self)
{
  mrb_value ret = mrb_nil_value();

  js_create_array(mrb, NULL, -1, &ret);
  return ret;
}

/*
 * Valid invoke types:
 * 0 - Normal function call
 * 1 - New call
 * 2 - Normal call specifying this value
 */
static mrb_value
mrb_js_func_invoke_internal(mrb_state *mrb, mrb_value func)
{
  mrb_value *argv = NULL;
  mrb_value ret = mrb_nil_value();
  mrb_value this_value;
  int argc = 0, type = -1;

  /* TODO: proc handling */
  mrb_get_args(mrb, "i*", &type, &argv, &argc);

  if (type == 2) {
    /* call with this object */
    this_value = argv[0];

    argv++;
    argc--;
  } else {
    /* this object is parent object by default */
     this_value = mrb_funcall_argv(mrb, func, mrb_intern_cstr(mrb, "parent_object"),
                                   0, NULL);
  }

  js_invoke(mrb, &this_value,
            mruby_js_get_object_handle_value(mrb, func),
            argv, argc, &ret, type);
  return ret;
}

static mrb_value
mrb_array_tojs(mrb_state *mrb, mrb_value arr)
{
  mrb_value ret = mrb_nil_value();

  js_create_array(mrb, RARRAY_PTR(arr), RARRAY_LEN(arr), &ret);
  return ret;
}

void
mrb_mruby_js_gem_init(mrb_state *mrb) {
  mjs_mod = mrb_define_module(mrb, "MrubyJs");
  mrb_define_class_method(mrb, mjs_mod, "get_root_object", mrb_js_get_root_object, ARGS_NONE());
  mrb_define_class_method(mrb, mjs_mod, "window", mrb_js_get_root_object, ARGS_NONE());
  mrb_define_class_method(mrb, mjs_mod, "global", mrb_js_get_root_object, ARGS_NONE());

  js_obj_cls = mrb_define_class_under(mrb, mjs_mod, "JsObject", mrb->object_class);
  mrb_define_method(mrb, js_obj_cls, "initialize", mrb_js_obj_initialize, ARGS_REQ(1));
  /* only for testing use */
  mrb_define_method(mrb, js_obj_cls, "handle", mrb_js_obj_handle, ARGS_NONE());
  mrb_define_method(mrb, js_obj_cls, "get", mrb_js_obj_get, ARGS_REQ(1));
  mrb_define_method(mrb, js_obj_cls, "set", mrb_js_obj_set, ARGS_REQ(2));
  mrb_define_method(mrb, js_obj_cls, "[]", mrb_js_obj_get, ARGS_REQ(1));
  mrb_define_method(mrb, js_obj_cls, "[]=", mrb_js_obj_set, ARGS_REQ(2));
  mrb_define_class_method(mrb, js_obj_cls, "create", mrb_js_obj_create, ARGS_NONE());

  js_func_cls = mrb_define_class_under(mrb, mjs_mod, "JsFunction", js_obj_cls);
  mrb_define_method(mrb, js_func_cls, "invoke_internal", mrb_js_func_invoke_internal, ARGS_ANY());

  js_array_cls = mrb_define_class_under(mrb, mjs_mod, "JsArray", js_obj_cls);
  mrb_define_class_method(mrb, js_array_cls, "create", mrb_js_array_create,
                          ARGS_NONE());

  mrb_define_method(mrb, mrb->array_class, "toJsArray", mrb_array_tojs, ARGS_NONE());
}

void
mrb_mruby_js_gem_final(mrb_state *mrb) {
}
