This is the mrbgem for calling JavaScript from mruby. It is primarily created to work with [webruby](https://github.com/xxuejie/webruby). But since it is grouped as a mrbgem and only requires a small JavaScript snippet, feel free to use it in your own project if you found it useful.

This project is distributed under the MIT License. See LICENSE for further details.

#TODO

1. Array handling(should we processes all items at once or should we use similar techniques as objects which keep the actual items only at JS side?)
2. Function argument(easy to handle just like objects, but should we allow Procs as functions? should we allow JS function creation in mruby?)
