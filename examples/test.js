#!shebang

// alert("hello !");

var seconds = 0;
var window = app.window("hello !", "hello world!");

window.ondestroy = function() {
	console.log("destroy");
}

setInterval(function() {
	console.log("seconds " + seconds ++);

	console.log("Is opened ? " + window.isOpen());

	if (seconds == 10)
		app.quit();
	window.setHtml("Seconds elapsed: " + seconds);
	if (seconds == 2)
		window.minimize();
	if (seconds == 3)
		window.restore();
	if (seconds == 4)
		window.maximize();
	if (seconds == 5) {
		// window.eval("alert('Window will close');");
		window.close();
	}
	return 0;
}, 1000);

app.onexit = function() {
	console.log("bye");
}

console.log("Arguments are", process.argv);
console.log("hello world!");

console.log(console.stringify(window));
