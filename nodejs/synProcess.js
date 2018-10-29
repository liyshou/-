//非阻塞代码--回调
var fs = require("fs");
fs.readFile('E:\\work\\SvnCode\\helloNode\\input.txt',function (err,data) {
    if(err) return  console.error(err);
    console.log(data.toString());
});
console.log("程序执行结束");


