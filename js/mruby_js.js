mergeInto(LibraryManager.library, {
  __js_global_object: function () {
    return (typeof window === 'object') ? (window) : (global);
  },

  __js_fetch_object__deps: ['__js_global_object'],
  __js_fetch_object: function (mrb, handle) {
    var obj = ___js_global_object()["MRB_JS_OBJ_CACHE"];
    if (obj && (typeof obj === 'object') &&
        (typeof handle === 'number') && (handle > 0)) {
      return obj[handle];
    }
  },

  __js_add_object__deps: ['__js_global_object'],
  __js_add_object: function (mrb, obj) {
    var global_object = ___js_global_object();
    global_object["MRB_JS_OBJ_CACHE"] = global_object["MRB_JS_OBJ_CACHE"] ||
      {"_mruby_js_next_id": 1, "_mruby_js_recycled_ids": []};
    var cache_object = global_object["MRB_JS_OBJ_CACHE"];
    var object_handle = -1;

    if (!("_mruby_js_id" in obj)) {
      // create new cache
      var new_id;
      if (cache_object["_mruby_js_recycled_ids"].length > 0) {
        // use recycled ids
        new_id = cache_object["_mruby_js_recycled_ids"].pop();
      } else {
        new_id = cache_object["_mruby_js_next_id"];

        cache_object["_mruby_js_next_id"] = new_id + 1;
      }
      cache_object[new_id] = obj;

      obj["_mruby_js_id"] = new_id;
      obj["_mruby_js_count"] = 1;

      object_handle = new_id;
    } else {
      // existing cache, only updates count
      obj["_mruby_js_count"] = obj["_mruby_js_count"] + 1;

      object_handle = obj["_mruby_js_id"];
    }
    return object_handle;
  },

  __js_is_floating_number: function (val) {
    var fixed = Math.round(val);
    var diff = Math.abs(fixed - val);
    var EPSILON = 1e-5;

    return (diff >= EPSILON);
  },

  __js_is_array: function (val) {
    return (typeof val !== 'undefined' &&
            val && val.constructor === Array);
  },

  __js_fill_return_arg__deps: ['__js_add_object', '__js_is_floating_number',
                               '__js_is_array'],
  __js_fill_return_arg: function (mrb, ret_p, val, parent_p) {
    var stack = 0;
    var RETURN_HANDLERS = {
      'object': function () {
        var handle = ___js_add_object(mrb, val);
        if (___js_is_array(val)) {
          _mruby_js_set_array_handle(mrb, ret_p, handle);
        } else {
          _mruby_js_set_object_handle(mrb, ret_p, handle);
        }
      },
      'function': function () {
        var handle = ___js_add_object(mrb, val);
        _mruby_js_set_function_handle(mrb, ret_p, handle, parent_p);
      },
      'number': function () {
        if (___js_is_floating_number(val)) {
          _mruby_js_set_float(mrb, ret_p, val);
        } else {
          _mruby_js_set_integer(mrb, ret_p, val);
        }
      },
      'boolean': function () {
        _mruby_js_set_boolean(mrb, ret_p, (val) ? (1) : (0));
      },
      'undefined': function () {
        _mruby_js_set_nil(mrb, ret_p);
      },
      'string': function () {
        if (!stack) stack = Runtime.stackSave();
        var ret = Runtime.stackAlloc(val.length + 1);
        writeStringToMemory(val, ret);
        _mruby_js_set_string(mrb, ret_p, ret);
      }
    };

    if (ret_p) {
      var val_type = typeof val;
      if (val_type !== null) {
        RETURN_HANDLERS[val_type]();
      }
    }
    if (stack) Runtime.stackRestore(stack);
  },

  __js_invoke_using_new: function (func, args) {
    // This function uses "new" operator to call JavaScript functions.
    // It is implemented in the following way for two reasons:
    // 1. Function.prototype.bind only exists in ECMAScript 5
    // 2. Even if we only work with ECMAScript 5 compatible browsers,
    // my test shows that we cannot use this method to create ArrayBuffer
    // (at least in Chrome).
    // So we will use the old-fashioned way to do this:)

    switch(args.length) {
      case 0:
        return new func();
      case 1:
        return new func(args[0]);
      case 2:
        return new func(args[0], args[1]);
      case 3:
        return new func(args[0], args[1], args[2]);
      case 4:
        return new func(args[0], args[1], args[2], args[3]);
      case 5:
        return new func(args[0], args[1], args[2], args[3], args[4]);
      case 6:
        return new func(args[0], args[1], args[2], args[3], args[4], args[5]);
      case 7:
        return new func(args[0], args[1], args[2], args[3], args[4], args[5],
                        args[6]);
      case 8:
        return new func(args[0], args[1], args[2], args[3], args[4], args[5],
                        args[6], args[7]);
      case 9:
        return new func(args[0], args[1], args[2], args[3], args[4], args[5],
                        args[6], args[7], args[8]);
      case 10:
        return new func(args[0], args[1], args[2], args[3], args[4], args[5],
                        args[6], args[7], args[8], args[9]);
      default:
        assert(false, "We do not support that many arguments now-_-");
    }
  },

  __js_fetch_argument__deps: ['__js_fetch_object', '__js_fill_return_arg'],
  __js_fetch_argument: function(mrb, argv_p, idx) {
    var TYPE_HANDLERS = {
      0: function() { return false; }, // MRB_TT_FALSE
      1: function() { return true; },  // MRB_TT_TRUE
      2: _mruby_js_get_integer,        // MRB_TT_FIXNUM
      3: _mruby_js_get_float,          // MRB_TT_FLOAT
      4: function() {
        var handle = _mruby_js_get_object_handle.apply(null, arguments);
        return ___js_fetch_object(mrb, handle);
      },                        // MRB_TT_OBJECT
      5: function() {
        var str_len = _mruby_js_get_string_len.apply(null, arguments);
        var str_p = _mruby_js_get_string_ptr.apply(null, arguments);
        return Module['Pointer_stringify'](str_p, str_len);
      },                        // MRB_TT_STRING
      6: function() { return undefined; }, // nil value
      7: function() {
        var proc = _mruby_js_get_proc.apply(null, arguments);
        return function() {
          // Callback arguments
          var cargc = arguments.length;
          var cargv = 0;
          if (cargc > 0) {
            var i;
            cargv = _mruby_js_invoke_alloc_argv(mrb, cargc);
            for (i = 0; i < cargc; i++) {
              ___js_fill_return_arg(mrb,
                                    _mruby_js_invoke_fetch_argp(mrb, cargv, i),
                                    arguments[i], 0);
            }
          }

          _mruby_js_invoke_proc(mrb, proc, cargc, cargv);
          if (cargc > 0) {
            _mruby_js_invoke_release_argv(mrb, cargv);
          }
        };
      },                        // MRB_TT_PROC
      8: function() {
        var handle = _mruby_js_get_array_handle.apply(null, arguments);
        return ___js_fetch_object(mrb, handle);
      },                        // MRB_TT_ARRAY
      9: function() {
        var handle = _mruby_js_get_hash_handle.apply(null, arguments);
        return ___js_fetch_object(mrb, handle);
      },                        // MRB_TT_HASH
      10: function() {
        _mruby_js_convert_symbol_to_string.apply(null, arguments);
        return TYPE_HANDLERS[5].apply(null, arguments);
      }                         // MRB_TT_SYMBOL
    };

    var handler = TYPE_HANDLERS[_mruby_js_argument_type(mrb, argv_p, idx)];
    return handler(mrb, argv_p, idx);
  },

  js_invoke__deps: ['__js_fill_return_arg',
                  '__js_fetch_object', '__js_invoke_using_new',
                   '__js_fetch_argument', '__js_global_object'],
  js_invoke: function (mrb, this_value_p,
                       func_handle,
                       argv_p, argc,
                       ret_p, type) {
    var func = ___js_fetch_object(mrb, func_handle);
    if (typeof func !== 'function') {
      _mruby_js_name_error(mrb);
    }

    var this_value = ___js_fetch_argument(mrb, this_value_p, 0);
    if (type !== 2) {
      if (this_value === ___js_global_object()) {
        // ECMAScript 5 compatible calling convention
        this_value = undefined;
      }
    }

    var i = 0, args = [], type_handler;
    for (i = 0; i < argc; i++) {
      args.push(___js_fetch_argument(mrb, argv_p, i));
    }

    var val;
    if (type === 1) {
      val = ___js_invoke_using_new(func, args);
    } else {
      val = func.apply(this_value, args);
    }

    // Returned value does not have a parent
    ___js_fill_return_arg(mrb, ret_p, val, 0);
  },

  js_get_field__deps: ['__js_fill_return_arg', '__js_fetch_object',
                       '__js_fetch_argument'],
  js_get_field: function (mrb, obj_p, field_p, ret_p) {
    var handle = _mruby_js_get_object_handle(mrb, obj_p, 0);
    var obj = ___js_fetch_object(mrb, handle);
    var val = obj[___js_fetch_argument(mrb, field_p, 0)];
    ___js_fill_return_arg(mrb, ret_p, val, obj_p);
  },

  js_set_field__deps: ['__js_fetch_object', '__js_fetch_argument'],
  js_set_field: function (mrb, obj_p, field_p, val_p) {
    var handle = _mruby_js_get_object_handle(mrb, obj_p, 0);
    var obj = ___js_fetch_object(mrb, handle);
    var field = ___js_fetch_argument(mrb, field_p, 0);
    var val = ___js_fetch_argument(mrb, val_p, 0);
    obj[field] = val;
  },

  js_get_root_object__deps: ['__js_global_object', '__js_fill_return_arg'],
  js_get_root_object: function (mrb, ret_p) {
    // Global object must be of object type, and has no parent.
    ___js_fill_return_arg(mrb, ret_p, ___js_global_object(), 0);
  },

  js_release_object__deps: ['__js_global_object'],
  js_release_object: function (mrb, handle) {
    var cache_object = ___js_global_object()["MRB_JS_OBJ_CACHE"];
    if (cache_object) {
      var rel_object = cache_object[handle];
      if (rel_object && ("_mruby_js_id" in rel_object)) {
        rel_object["_mruby_js_count"] = rel_object["_mruby_js_count"] - 1;
        if (rel_object["_mruby_js_count"] === 0) {
          // reference count reaches 0, release object
          var next_id = cache_object["_mruby_js_next_id"];

          delete cache_object[handle];
          if (handle === (next_id - 1)) {
            cache_object["_mruby_js_next_id"] = next_id - 1;
          } else {
            cache_object["_mruby_js_recycled_ids"].push(handle);
          }

          delete rel_object["_mruby_js_id"];
          delete rel_object["_mruby_js_count"];

          // Reset the next id when we have all recycled ids. I wonder
          // if a slice loop which can recycle partial ids is needed here.
          if (cache_object["_mruby_js_recycled_ids"].length ===
              (cache_object["_mruby_js_next_id"] - 1)) {
            cache_object["_mruby_js_next_id"] = 1;
            cache_object["_mruby_js_recycled_ids"] = [];
          }
        }
      }
    }
  },

  js_create_array__deps: ['__js_fetch_argument', '__js_fill_return_arg'],
  js_create_array: function(mrb, arr_p, len, ret_p) {
    var ret = [], i;
    if ((arr_p !== 0) && (len !== -1)) {
      for (i = 0; i < len; i++) {
        ret.push(___js_fetch_argument(mrb, arr_p, i));
      }
    }
    ___js_fill_return_arg(mrb, ret_p, ret, 0);
  },

  js_create_empty_object__deps: ['__js_fill_return_arg'],
  js_create_empty_object: function(mrb, ret_p) {
    ___js_fill_return_arg(mrb, ret_p, {}, 0);
  }
});
