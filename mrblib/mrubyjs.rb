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
    # call them
    def method_missing(key, *args)
      call(key.to_s, *args)
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

    def method_missing(key, *args)
      invoke(key.to_s, *args)
    end
  end
end
