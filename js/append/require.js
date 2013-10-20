(function () {
  // In node.js, the variable `r` below will be undefined after executing
  // the following code:
  //
  // var obj = global;
  // var arg = 'require';
  // var r = obj[arg];
  //
  // One guess is that node.js uses special implementation of require.
  // For this to work on webruby, we have to add the following patch.
  if (!(typeof window === 'object')) {
    global['require'] = require;
  }
}) ();
