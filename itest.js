var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var fs = require('fs');

var index=0;
var temp = 0;

var dataSize = 256;
var bufferSize = dataSize*2;
var value = new Array(bufferSize/2);
var index2 = 0;

var index3 = 0;

app.get('/', function(req, res){
  	res.sendfile('itest.html');
	fs.open('/mnt/mmc/snow/SS_OLD/ALOOH/src/AnalogSensor/result/A09','r', function(status,fd){
	
		if(status){
			console.log(status.message);
			return;
		}
		var buffer = new Buffer(bufferSize);
		/*
		for(index2=0;index2<400;index2++){
			//console.log(index2);
			//io.emit('chat message',buffer);
			fs.read(fd,buffer,0,bufferSize,null,function(err,num){
				if(err){
					console.log(err);
	
				}
				else{
					value.length=0;
					for(index=0;index<bufferSize/2;index++){
						value.push((buffer[index*2+1]*256+buffer[index*2])/256); 
					}
					console.log(index3++);		
 					io.emit('chat message',value);
				}
			});
		}
		*/
		
		
		setInterval(function(){
		/*
			for(index2=0;index2<60;index2++){
				console.log(index2);
				//fs.readSync(fd,buffer,0,bufferSize,null);
				for(index3=0;index3<bufferSize;index3++){
					buffer.writeInt16LE(index2,index3,bufferSize);
				}
				io.emit('chat message',buffer);
				//console.log('-----'+index2 +'--------');
				//console.log(buffer);
			}
			*/
			
			fs.read(fd,buffer,0,bufferSize,null,function(err,num){
				value.length=0;
				//value.push(buffer);
				//console.log(value);		
				//console.log(buffer);
 				io.emit('chat message',buffer);
			});
			//buffer.writeUInt16LE(index3++,0,bufferSize);
			//io.emit('chat message',buffer);
		},1);
		
	});
});

app.get('/processing.js', function(req, res){
  res.sendfile('processing.js');
});

io.on('connection', function(socket){
  console.log('a user connected');
  socket.on('disconnect', function(){
    console.log('user disconnected');
  });
  socket.on('chat message', function(msg){
    io.emit('chat message', msg);
  });
});

http.listen(3000, function(){
  console.log('listening on *:3000');
});




