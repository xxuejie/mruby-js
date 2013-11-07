// mruby-js testing framework at JS side
(function () {
  var CACHE_STRING = 'MRB_JS_OBJ_CACHE';
  var TEST_STRING = 'MRB_JS_TEST';

  var root = (typeof window === 'object') ? (window) : (global);
  root[TEST_STRING] = root[TEST_STRING] || {};
  var test = root[TEST_STRING];

  root['mrb_assert_root_object'] = function (handle) {
    return root === root[CACHE_STRING][handle];
  };
  root['mrb_assert_test_object'] = function (handle) {
    return test === root[CACHE_STRING][handle];
  };

  test['field_number'] = 42;
  test['field_float'] = 2.71828;
  test['field_true'] = true;
  test['field_false'] = false;
  test['field_undefined'] = undefined;
  test['field_string'] = "A JS String";
  test['field_object'] = {
    a: 1,
    b: 1.2,
    c: "String"
  };
  test['field_array'] = [ "a", undefined, 1 ];
  test['field_recursive_object'] = {
    obj: {
      a: true
    },
    arr: [
      [1, 2, 3, 4],
      { val: "field" }
    ]
  };

  var is_js_array = function (val) {
    return (typeof val !== 'undefined' &&
            val && val.constructor === Array);
  };

  test['func_no_arg'] = function () {
    return true;
  };
  test['func_number'] = function (num) {
    return num == 11;
  };
  test['func_float'] = function (f) {
    return Math.abs(f - 1.789) < 1e-10;
  };
  test['func_boolean'] = function (b) {
    return b === true;
  };
  test['func_nil'] = function (u) {
    return u === undefined;
  };
  test['func_string'] = function (s) {
    return s === 'Ruby string';
  };
  test['func_object'] = function (o) {
    return (typeof o === 'object') &&
      (o.a === 'field') &&
      (o.b === 123);
  };
  test['func_array'] = function (a) {
    return is_js_array(a) &&
      (a.length === 2) &&
      (a[0] === 7) &&
      (a[1] === 'abc');
  };
  test['func_recursive_object'] = function(o) {
    return (typeof o === 'object') &&
      (typeof o.obj === 'object') &&
      (o.obj.a === 123) &&
      is_js_array(o.arr) &&
      (o.arr.length === 2) &&
      (o.arr[0] === 10) &&
      is_js_array(o.arr[1]) &&
      (o.arr[1].length === 2) &&
      (o.arr[1][0] === 'a') &&
      (o.arr[1][1] === 'b');
  };
  test['func_proc'] = function(p) {
    return p();
  };
  test['func_proc_with_arg'] = function(p, args) {
    return p.apply(this, args);
  };
  test['func_mruby_fields'] = function(o) {
    for (var i in o) {
      if ((i === '_mruby_js_id') || (i === '_mruby_js_count')) {
        return false;
      }
    }
    return true;
  };
  test['func_on_func'] = function() {};
  test['func_on_func']['second'] = function(a) {
    return a;
  };
}) ();
