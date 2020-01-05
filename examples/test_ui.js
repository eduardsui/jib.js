var ui = require("ui");
var fs = require("fs");

var window = ui.window(fs.readFileSync("form.html", "utf-8"), "hello world!");

window.onuievent = function() {
	console.log("event", console.stringify(window.data()));
	window.setContent("hello !");
}

window.ondestroy = function() {
	console.log("destroy");
}

app.onexit = function() {
	console.log("bye");
}

console.log("Arguments are", process.argv);
console.log("hello world!");
