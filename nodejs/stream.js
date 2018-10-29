//抽象接口
/*
* 四种流类型
* readable 可读
* writable  可写
* duplex  w +r
* transform 操作被写入数据，然后读出结果
* stream 对象都是EventEmitter 的实列
* data  当有数据可读触发
* end  没有更多数据可读触发
* error 在接收和写入过程中发生错误触发
* finish 所有数据被写入到底层系统被触发*/

var fs = require('fs');
var data= '';//初始化

//创建可读流

var readerStream = fs.createReadStream('E:\\work\\SvnCode\\helloNode\\input.txt');
//设置编码
readerStream.setEncoding('utf8');
readerStream.on('data',function (chunk) {

    data+=chunk;
});
readerStream.on('end',function () {
    console.log(data);
});

readerStream.on('error',function (err) {
    console.log(err.stack);
});

//写入流
var data1 ='liyuanshou';
var writeStream =fs.createReadStream('E:\\work\\SvnCode\\helloNode\\output.txt');
//设置编码
writeStream.write(data,'utf8');
//标记文件末尾
writeStream.end();

//处理流事件 --》 data,end ,and error
writeStream.on('finish',function () {
    console.log("write finish.");
});
writeStream.on('error',function (err) {
    console.log(err.stack);

});

//管道流
//一根管子(pipe)连接两个桶使得水从一个桶流入另一个桶
//将input.txt 的内容写到output.txt 中
readerStream.pipe(writeStream);

//链式流
var zlib = require('zlib');
//压缩input.txt
fs.createReadStream('input.txt')
    .pipe(zlib.createGzip())
    .pipe(fs.createWriteStream('input,txt.gz'));

//解压
fs.createReadStream('input.txt.gz')
    .pipe(zlib.createGunzip)
    .pipe(fs.createWriteStream('input.txt'));

