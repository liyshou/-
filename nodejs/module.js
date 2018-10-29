//Node.js 提供了 exports 和 require 两个对象，其中 exports 是模块公开的接口，require 用于从外部获取一个模块的接口，即所获取模块的 exports 对象。

var hello  =require('./helloworld');//
//hello.world();

//另外一个把整个对象封装到模块中
//模块接口的唯一变化是使用 module.exports = Hello 代替了exports.world = function(){}。 在外部引用该模块时，其接口对象就是要输出的 Hello 对象本身，而不是原先的 exports。
Hi = new hello();
Hi.setName('shou');
Hi.sayHello();