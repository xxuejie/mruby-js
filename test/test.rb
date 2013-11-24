assert('get root object') do
  root = MrubyJs.get_root_object
  window = MrubyJs.window
  global = MrubyJs.global

  assert_equal window, root
  assert_equal window, global
  assert_true root.mrb_assert_root_object(root.handle)
end

assert('get test object') do
  global = MrubyJs.global
  test = global.get("MRB_JS_TEST")

  assert_true global.mrb_assert_test_object(test.handle)
end

assert('get JavaScript primitive fields') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  assert_equal 42, test.field_number
  assert_float 2.71828, test.field_float
  assert_true test.field_true
  assert_false test.field_false
  assert_nil test.field_undefined
  assert_equal "A JS String", test.field_string
end

assert('get JavaScript object') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  obj = test.field_object
  assert_true obj.instance_of?(MrubyJs::JsObject)
  assert_equal 1, obj.a
  assert_float 1.2, obj.b
  assert_equal "String", obj.c
end

assert('get JavaScript array') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  arr = test.field_array
  assert_true arr.instance_of?(MrubyJs::JsArray)
  assert_equal 3, arr.length
  assert_equal "a", arr[0]
  assert_nil arr[1]
  assert_equal 1, arr[2]
end

assert('get recursive JavaScript object') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  obj = test.field_recursive_object
  assert_true obj.instance_of?(MrubyJs::JsObject)

  sub_obj = obj.obj
  assert_true sub_obj.instance_of?(MrubyJs::JsObject)
  assert_true sub_obj.a

  sub_arr = obj.arr
  assert_true sub_arr.instance_of?(MrubyJs::JsArray)
  assert_equal 2, sub_arr.length

  sub_sub_arr = sub_arr[0]
  assert_true sub_sub_arr.instance_of?(MrubyJs::JsArray)
  assert_equal 4, sub_sub_arr.length
  assert_equal 1, sub_sub_arr[0]
  assert_equal 2, sub_sub_arr[1]
  assert_equal 3, sub_sub_arr[2]
  assert_equal 4, sub_sub_arr[3]

  sub_sub_obj = sub_arr[1]
  assert_true sub_sub_obj.instance_of?(MrubyJs::JsObject)
  assert_equal "field", sub_sub_obj.val
end

assert('call JavaScript function') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  assert_true test.func_no_arg[]
  assert_true test.func_no_arg.invoke
end

assert('call function with primitive Ruby types') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  assert_true test.func_number(11)
  assert_true test.func_float(1.789)
  assert_true test.func_boolean(true)
  assert_true test.func_nil(nil)
  assert_true test.func_string('Ruby string')
end

assert('call function with object') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  assert_true test.func_object({a: 'field', b: 123})
end

assert('call function with array') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  assert_true test.func_array([7, 'abc'])
end

assert('call function with recursive object') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  obj = {
    obj: {
      a: 123
    },
    arr: [ 10, [ 'a', 'b' ] ]
  }
  assert_true test.func_recursive_object(obj)
end

assert('call function with proc') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  assert_true test.func_proc(Proc.new { true })
end

assert('call function with proc and args') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  assert_true test.func_proc_with_arg(Proc.new { |a, b| a == 19 && b == 'zzz' },
                                      [19, 'zzz'])
end

assert('_mruby_js* fields are not present in js objects') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  assert_true test.func_mruby_fields({:aaa => 3})
end

assert('call function which is a property of another function') do
  test = MrubyJs.global.get("MRB_JS_TEST")

  assert_equal 12345, test.func_on_func.second(12345)
end
