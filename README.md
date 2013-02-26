This is the mrbgem for calling JavaScript from mruby. It is primarily created to work with [webruby](https://github.com/xxuejie/webruby). But since it is grouped as a mrbgem and only requires a small JavaScript snippet, feel free to use it in your own project if you found it useful.

This project is distributed under the MIT License. See LICENSE for further details.

#Features

* Fetch JS numbers, strings, arrays, objects and functions
* Use either '.' syntax or '[]' syntax to retrieve fields in a JS object
* Perform JS Function call using normal method, new method or method with specified this value
* Auto conversion from Ruby arrays to JS arrays, and Ruby hashes to JS objects
* Use `method_missing` to expose JS functions to Ruby
* Use Ruby procs as JS callback functions, you can even pass JS values as arguments to the Ruby procs!
