
var os =require("os");
console.log(process.platform);
console.log(process.execPath);
console.log(process.cwd());
console.log(process.version);
console.log(process.memoryUsage());


//cpu 字节序
console.log(os.endianness());

console.log(os.type);//操作系统名字

console.log(os.totalmen());

console.log(os.freemen());