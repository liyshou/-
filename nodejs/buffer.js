const buf = Buffer.from('baidu','utf-8');

console.log(buf.toString('hex'));
console.log(buf.toString('base64'));


//js 字符编码；
/* ASCII 仅支持7位ascll
* utf8 多字节编码的unicode
* uft16le 2或4个字节，小字节序编码Unicode 编码
* base64
* latin1 把buffer 编码成一字节编码的字符串方式 用binary 别名
* hex 将每个字节编码为两个16进制字符*/

//创建一个长度为10 ，且用0填充的buffer
const buf1  = Buffer.alloc(10);

//创建一个长度10，且用0x1填充buffer
const buf2 =  Buffer.alloc(10,1);

//创建一个长度10，未初始化的buffer
//这个方法比调用buffer.alloc()更快
//但返回的buffer实列可能包含旧数据
//因此需要fill()或者write()重写
const buf3 = Buffer.allocUnsafe(10);

//buf.write()  ;//返回实际的写入大小，空间不足只写入部分

var buf4 = Buffer.alloc(256);
var len= buf4.write("www.baidu.com");
console.log("写入的字节数:"+len);

//从缓冲区读取数据buf.tostring(encoding ,start ,end)
//解码缓冲数据并使用指定的编码返回字符串
var buf5 = Buffer.alloc(26);
for(var i=0;i<26;i++)
    buf5[i] = i+97;
console.log(buf5.toString('ascii')); //abc...z
console.log(buf5.toString('ascii',0,5));//abcde
console.log(buf5.toString('utf8',0,5));//abcde
console.log(buf5.toString(undefined,0,5)); //使用默认的编码utf8

//将buffer 转换成json 对象
//buf.toJSON()
//当字符串化一个buffer 实列时，json.stringify() 会隐式地调用tojson

const buf6 = Buffer.from([0x1,0x2,0x3,0x4,0x5]);
const json  = JSON.stringify(buf6);
console.log(json);

const copy = JSON.parse(json,(key,value)=>{
    return value && value.type==='Buffer'? Buffer.from(value.data):value;
});
console.log(copy);


//缓冲区合并
var buf7 = Buffer.from("you");
var buf8 = Buffer.from("ni");
var buf9 = Buffer.concat([buf7,buf8]);
console.log(buf9.toString());

//缓冲区比较
var result  = buf7.compare(buf8);

console.log(result);

//拷贝缓冲区
//buf.copy(targetBuffer[, targetStart[, sourceStart[, sourceEnd]]])
//start 默认0，end 默认长度
 buf7.copy(buf8,2)//插到指定地位置
console.log(buf8.toString());

 //缓冲区裁剪
console.log(buf8.slice(0,2).toString());

//缓冲区长度
console.log(buf8.length);



