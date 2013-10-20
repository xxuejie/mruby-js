assert('get root object') do
  root = MrubyJs.get_root_object
  window = MrubyJs.window
  global = MrubyJs.global

  (root == window) && (window == global) &&
    root.mrb_assert_root_object(root.handle)
end

assert('get test object') do
  global = MrubyJs.global
  test = global.get("MRB_JS_TEST")

  global.mrb_assert_test_object(test.handle)
end

assert('get JavaScript primitive fields') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  r1 = test.field_number == 42
  r2 = check_float(test.field_float, 2.71828)
  r3 = test.field_true
  r4 = !test.field_false
  r5 = test.field_undefined.nil?
  r6 = test.field_string == "A JS String"

  r1 && r2 && r3 && r4 && r5 && r6
end

assert('get JavaScript object') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  obj = test.field_object
  r1 = obj.instance_of? MrubyJs::JsObject
  r2 = obj.a == 1
  r3 = check_float(obj.b, 1.2)
  r4 = obj.c == "String"

  r1 && r2 && r3 && r4
end

assert('get JavaScript array') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  arr = test.field_array
  r1 = arr.instance_of? MrubyJs::JsArray
  r2 = arr.length == 3
  r3 = arr[0] == "a"
  r4 = arr[1].nil?
  r5 = arr[2] == 1

  r1 && r2 && r3 && r4 && r5
end

assert('get recursive JavaScript object') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  obj = test.field_recursive_object
  r1 = obj.instance_of? MrubyJs::JsObject

  sub_obj = obj.obj
  r2 = sub_obj.instance_of? MrubyJs::JsObject
  r3 = sub_obj.a

  sub_arr = obj.arr
  r4 = sub_arr.instance_of? MrubyJs::JsArray
  r5 = sub_arr.length == 2

  sub_sub_arr = sub_arr[0]
  r6 = sub_sub_arr.instance_of? MrubyJs::JsArray
  r7 = sub_sub_arr.length == 4
  r8 = sub_sub_arr[0] == 1
  r9 = sub_sub_arr[1] == 2
  r10 = sub_sub_arr[2] == 3
  r11 = sub_sub_arr[3] == 4

  sub_sub_obj = sub_arr[1]
  r12 = sub_sub_obj.instance_of? MrubyJs::JsObject
  r13 = sub_sub_obj.val == "field"

  r1 && r2 && r3 && r4 && r5 && r6 && r7 && r8 && r9 && r10 && r11 && r12 && r13
end

assert('call JavaScript function') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  test.func_no_arg[] && test.func_no_arg.invoke
end

assert('call function with primitive Ruby types') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  test.func_number(11) && test.func_float(1.789) &&
    test.func_boolean(true) && test.func_nil(nil) &&
    test.func_string('Ruby string')
end

assert('call function with object') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  test.func_object({a: 'field', b: 123})
end

assert('call function with array') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  test.func_array([7, 'abc'])
end

assert('call function with recursive object') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  obj = {
    obj: {
      a: 123
    },
    arr: [ 10, [ 'a', 'b' ] ]
  }
  test.func_recursive_object(obj)
end

assert('call function with proc') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  test.func_proc(Proc.new { true })
end

assert('call function with proc and args') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  test.func_proc_with_arg(Proc.new { |a, b| a == 19 && b == 'zzz' },
                          [19, 'zzz'])
end

assert('_mruby_js* fields are not present in js objects') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  test.func_mruby_fields({:aaa => 3})
end
