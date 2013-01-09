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
        cache_object[new_id] = obj;
      }

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

    return (diff < EPSILON);
  },

  __js_fill_return_arg__deps: ['__js_add_object', '__js_is_floating_number'],
  __js_fill_return_arg: function (mrb, ret_p, val) {
    var stack = 0;
    var RETURN_HANDLERS = {
      'object': function (mrb, ret_p, val) {
        var handle = ___js_add_object(mrb, val);
        _mruby_js_set_object_handle(mrb, ret_p, handle);
      },
      'number': function (mrb, ret_p, val) {
        if (___js_is_floating_number(val)) {
          _mruby_js_set_float(mrb, ret_p, val);
        } else {
          _mruby_js_set_integer(mrb, ret_p, val);
        }
      },
      'boolean': function (mrb, ret_p, val) {
        _mruby_js_set_boolean(mrb, ret_p, (val) ? (1) : (0));
      },
      'undefined': function (mrb, ret_p, val) {
        _mruby_js_set_nil(mrb, ret_p);
      },
      'string': function (mrb, ret_p, val) {
        if (!stack) stack = Runtime.stackSave();
        var ret = Runtime.stackAlloc(val.length);
        writeArrayToMemory(val, ret);
        _mruby_js_set_string(mrb, ret_p, ret);
      }
    };

    if (ret_p) {
      var val_type = typeof val;
      if (val_type !== null) {
        RETURN_HANDLERS[val_type](mrb, ret_p, val);
      }
    }
    if (stack) Runtime.stackRestore(stack);
  },

  __js_fetch_field: function (base_object, name_p) {
    if (base_object && (typeof base_object === 'object')) {
      return base_object[Module.Pointer_stringify(name_p)];
    }
  },

  __js_call_using_new: function (func, args) {
    // This function uses "new" operator to call JavaScript functions.
    // It is implemented in the following way for two reasons:
    // 1. Function.prototype.bind only exists in ECMAScript 5
    // 2. Even if we only work with ECMAScript 5 compatible browsers,
    // my test shows that we cannot use this method to create ArrayBuffer
    // (at least in Chrome).
    // So we will use the old-fashioned way to do this:)

    switch(args.length) {
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

  js_call__deps: ['__js_fill_return_arg', '__js_fetch_field',
                  '__js_fetch_object', '__js_call_using_new'],
  js_call: function (mrb, handle, name_p, argv_p, argc, ret_p,
                    constructor_call) {
    // Supported types. Currently we are only considering
    // false, true, number and string. Those are the
    // primitive types specified in the JSON spec.
    var TYPE_HANDLERS = {
      0: function() { return false; }, // MRB_TT_FALSE
      2: function() { return true; },  // MRB_TT_TRUE
      3: _mruby_js_get_integer,        // MRB_TT_FIXNUM
      6: _mruby_js_get_float,          // MRB_TT_FLOAT
      9: function() {
        var handle = _mruby_js_get_object_handle.apply(null, arguments);
        return ___js_fetch_object(mrb, handle);
      },                        // MRB_TT_OBJECT
      17: function() {
        var str_p = _mruby_js_get_string.apply(null, arguments);
        return Module.Pointer_stringify(str_p);
      }                         // MRB_TT_STRING
    };

    var base_object = ___js_fetch_object(mrb, handle);
    var func = ___js_fetch_field(base_object, name_p);
    if (typeof func !== 'function') {
      _mruby_js_name_error(mrb);
    }

    var i = 0, args = [], type_handler;
    for (i = 0; i < argc; i++) {
      type_handler = TYPE_HANDLERS[_mruby_js_argument_type(mrb, argv_p, i)];
      args.push(type_handler(mrb, argv_p, i));
    }

    var val;
    if (constructor_call === 1) {
      val = ___js_call_using_new(func, args);
    } else {
      val = func.apply(base_object, args);
    }
    ___js_fill_return_arg(mrb, ret_p, val);
  },

  js_get_field__deps: ['__js_fill_return_arg', '__js_fetch_field',
                       '__js_fetch_object'],
  js_get_field: function (mrb, handle, field_name_p, ret_p) {
    var field = ___js_fetch_field(___js_fetch_object(mrb, handle),
                                  field_name_p);
    ___js_fill_return_arg(mrb, ret_p, field);
  },

  js_get_root_object__deps: ['__js_global_object', '__js_fill_return_arg'],
  js_get_root_object: function (mrb, ret_p) {
    // Global object must be of object type
    ___js_fill_return_arg(mrb, ret_p, ___js_global_object());
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
});
