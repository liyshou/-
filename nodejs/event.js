//引入events模块
var events = require('events');

//enevntEmitter 提供了多个属性，or ：用于绑定事件函数，emit 用于触发事件发生
var enevntEmitter = new events.EventEmitter();
//创建事件处理程序
var connectHandler = function connected() {
    console.log('连接成功。');

    //触发data_received 事件
    enevntEmitter.emit('data_received');
}
//绑定事件及事件的处理程序
enevntEmitter.on('connection',connectHandler);

//使用匿名函数绑定data_received事件
enevntEmitter.on('data_received',function () {
    console.log('数据接收成功。');
});
//触发connection事件
enevntEmitter.emit('connection');
console.log('程序执行完毕。');