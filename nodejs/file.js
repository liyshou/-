var fs = require('fs');

console.log("准备打开文件。");
fs.stat('E:\\work\\SvnCode\\helloNode\\input.txt' ,function (err,stats) {
    if(err)
        return console.log(err);
    console.log(stats);
    console.log("读取文件成功。");

    //检测文件类型
    console.log("是否文件?" +stats.isFile() );
    console.log("是否为目录？",stats.isDirectory());
});//获取文件的格式信息


//打开文件
var buf = new Buffer.alloc(1024);
fs.open('E:\\work\\SvnCode\\helloNode\\input.txt','r+',function (err,fd) {

    if(err)
        return console.log(err);
    console.log(fd.toString());
    fs.read(fd,buf,0,buf.length,0,function (err,bytes) {
        if(err)
            return console.log(err);
        console.log(bytes + "字节被读取。");
        if(bytes>0)
            console.log(buf.slice(0,bytes).toString());
    });
    fs.close(fd,function (err) {
        if(err)
            console.log(err);
        console.log("close success.");
    });
});

//读取文件
fs.readFile("E:\\work\\SvnCode\\helloNode\\input.txt",function (err,data) {
    if(err)
        return console.log(err);
    console.log(data.toString());

});

//写文件
fs.writeFile("E:\\work\\SvnCode\\helloNode\\input.txt",'www.hellokiki.com.',function (err) {
    if(err)
        return console.log(err);
    fs.readFile('E:\\work\\SvnCode\\helloNode\\input.txt',function (err,data) {
        if(err)
            return console.log(err);
        console.log(data.toString());
    });
});

