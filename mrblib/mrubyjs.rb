module MrubyJs
  class JsObject
    def call(name, *args)
      get_func(name).invoke(*args)
    end

    def call_constructor(name, *args)
      get_func(name).invoke_constructor(*args)
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
    # 1. If args is an empty array, we will fetch the field
    # 2. If args is not empty, we will fetch the function and make the call
    # Our solution here only covers the most common cases, users
    # will need to explicitly use call or get for other rare cases
    def method_missing(key, *args)
      if args.length > 0
        call(key.to_s, *args)
      else
        get(key.to_s)
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

    def invoke_constructor(*args)
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
end
