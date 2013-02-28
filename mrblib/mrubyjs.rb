module MrubyJs
  # procs that are used in callbacks
  @@procs = {}

  def self.add_proc(p, times = -1)
    @@procs[p] = times unless @@procs.has_key? p
  end

  def self.call_proc(p)
    if @@procs.has_key? p
      t = @@procs[p]
      return if t == -1

      if t <= 1
        # remove expired procs
        @@procs.delete(p)
      else
        @@procs[p] = t - 1
      end
    end
  end

  class JsObject
    def call(name, *args)
      get_func(name).invoke(*args)
    end

    def call_new(name, *args)
      get_func(name).invoke_new(*args)
    end

    def call_with_this(name, this_value, *args)
      get_func(name).invoke_with_this(this_value, *args)
    end

    def get_func(name)
      func = get(name)
      raise ArgumentError, "#{name} does not exist!" unless func
      func
    end

    # For methods that do not conflict with Ruby, we can directly
    # call them or fetch them, the following rules are used:
    # 1. If key is of format "field=", we will set the field with given arguments
    # 2. If args is an empty array, we will fetch the field
    # 3. If args is not empty, we will fetch the function and make the call
    # Our solution here only covers the most common cases, users
    # will need to explicitly use call or get for other rare cases
    def method_missing(key, *args)
      key = key.to_s
      if key[-1] == '='
        # This is a set method
        set(key[0..-2], *args)
      else
        return call(key, *args) if args.length > 0
        get(key)
      end
    end
  end

  class JsFunction
    attr_reader :parent_object

    def initialize(handle, parent_object)
      super(handle)

      @parent_object = parent_object
    end

    def invoke(*args)
      invoke_internal(0, *args)
    end

    def invoke_new(*args)
      invoke_internal(1, *args)
    end

    def invoke_with_this(this_value, *args)
      invoke_internal(2, this_value, *args)
    end

    def [](*args)
      invoke(*args)
    end

    def method_missing(key, *args)
      invoke(key.to_s, *args)
    end
  end

  class JsArray
    def each(&block)
      idx ,len = 0, self.length
      while idx < len
        block.call(self[idx])
        idx += 1
      end
    end
  end
end

class Proc
  def release_after(n)
    MrubyJs::add_proc(self, n)
    self
  end
end

# NOTE: Actually Array#toJsArray can also be implemented like this,
# it is implemented in C hoping for a little performance gain(there
# may be not, maybe a profiling is needed later.)
# On the other hand, we will need to use Hash#keys for Hash#toJsObject
# anyway, so we just do this here for programmers' convenience
class Hash
  def toJsObject
    o = MrubyJs::JsObject.create
    self.each do |key, value|
      o[key] = value
    end
    o
  end
end

class Symbol
  # Create a proc based on the method specified by the symbol,
  # we need +num_args+ to indicate the number of arguments passing
  # from JS side, since it is not allowed to pass less or more
  # arguments than specified(JS has no this limitation).
  # Here's two solutions to this problem:
  # 1. Add an argument indicating the number of arguments
  # 2. Fix mruby internally to by-pass this restriction
  # One of webruby's goal is to bring the same Ruby as you run
  # elsewhere, like in iOS. So we will keep the original Ruby
  # as much as possible. As a result of this, we will use solution
  # 1 here by default. At a later time, we may add code to support
  # solution 2, but the default option will stays 1.
  def to_proc(num_args = -1)
    sym = self
    Proc.new do |*args|
      args = args[0, num_args] if (num_args != -1 && args.length > num_args)
      send(sym, *args)
    end
  end
end
