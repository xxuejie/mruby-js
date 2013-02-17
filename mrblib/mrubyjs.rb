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
  end

  class JsFunction
    attr_reader :parent_object

    def initialize(handle, parent_object)
      puts "Creating js function! handle: #{handle}, parent: #{parent_object.inspect}"

      super(handle)

      @parent_object = parent_object
    end

    def invoke(*args)
      puts "Invoke: #{parent_object.inspect}"

      invoke_internal(0, *args)
    end

    def invoke_constructor(*args)
      invoke_internal(1, *args)
    end

    def invoke_with_this(this_value, *args)
      invoke_internal(2, this_value, *args)
    end
  end
end
