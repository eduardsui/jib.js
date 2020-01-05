var sock = _net_.socket(2, 1, 0);
console.log(sock);
console.log(_net_.bind(sock, "0.0.0.0", 80));
console.log(_net_.listen(sock, 100));

console.log(_net_.poll(sock, 0, { "read": function() {			
						var sock2 = _net_.accept(sock);
						console.log("accept", sock2);
						_net_.poll(sock2, 0, { "read": function() {
							var buf = new ArrayBuffer(1024);
							console.log(_net_.recv(sock2, buf, 1024));
							console.log(_net_.strerror(_net_.errno()));
							console.log(new TextDecoder("utf-8").decode(buf));
							var buf2 = new TextEncoder("utf-8").encode("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nHello world!");
							_net_.send(sock2, buf2, buf2.byteLength);
							_net_.unpoll(sock2);
							_net_.close(sock2);
						}});
					}
				}));

setInterval(function() {
	console.log("here");
}, 1000);