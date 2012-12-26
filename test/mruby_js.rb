assert('mruby-js test 1') do
  MrubyJs.respond_to? :test
end

assert('mruby-js test 2') do
  MrubyJs.respond_to? :call
end
