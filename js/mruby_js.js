mergeInto(LibraryManager.library, {
  js_call: function (mrb, handle, name_p, argv_p, argc) {

    // Supported types. Currently we are only considering
    // false, true, number and string. Those are the
    // primitive types specified in the JSON spec.
    var TYPE_HANDLERS = {
      0: function() { return false; }, // MRB_TT_FALSE
      2: function() { return true; },  // MRB_TT_TRUE
      3: _mruby_js_get_integer,        // MRB_TT_FIXNUM
      6: _mruby_js_get_float,          // MRB_TT_FLOAT
      17: function() {
        var str_p = _mruby_js_get_string.apply(this, arguments);
        return Module.Pointer_stringify(str_p);
      }                         // MRB_TT_STRING
    }

    // TODO: add code to specify base object
    var base_object = (typeof window === 'object') ? (window) : (global);
    var func = base_object[Module.Pointer_stringify(name_p)];
    if (typeof func !== 'function') {
      _mruby_js_name_error(mrb);
    }

    var i = 0, args = [], type_handler;
    for (i = 0; i < argc; i++) {
      type_handler = TYPE_HANDLERS[_mruby_js_argument_type(mrb, argv_p, i)];
      args.push(type_handler(mrb, argv_p, i));
    }

    func.apply(this, args);
  },
});
