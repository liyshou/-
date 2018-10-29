var events = require('events');
var eventEmitter = new events.EventEmitter();

//监听器1
var listen1 = function listener1() {
    console.log('监听器1执行。');

}

//监听器2
var listener2 = function listener2(){
    console.log('监听器2执行');
}

//绑定连接事件，处理事件listen1
eventEmitter.addListener('connection',listen1);

eventEmitter.on('connection',listener2);

//获取连接监听数目
var eventListeners = eventEmitter.listenerCount('connection');
console.log(eventListeners + "个监听器监听连接事件。");

//触发连接事件
eventEmitter.emit('connection');

//移除监听绑定listtener1函数
eventEmitter.removeListener('connection',listen1);
console.log("listen1 不再受监听.");

eventEmitter.emit('connection');

eventListeners = eventEmitter.listenerCount('connection');
console.log(eventListeners + "个监听器监听连接事件。");

console.log("程序执行完毕。");

