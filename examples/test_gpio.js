for (var i = 0; i < 35; i ++)
	gpio.padSelectGpio(i);

var val = 1;
setInterval(function() {
	for (var i = 0; i < 35; i ++)
		gpio.setLevel(i, val);
	console.log('GPIO', val);
	if (val == 1)
		val = 0;
	else
		val = 1; 
}, 1000);
