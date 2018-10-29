exports.world = function () {
    console.log('hello world');
}

function hello() {
    var name;
    this.setName = function (thyName) {
        name = thyName;//设置值
    }

    this.sayHello = function () {
        console.log('hello' +name);
    }

}
module.exports = hello;