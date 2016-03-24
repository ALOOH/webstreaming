var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var fs = require('fs');
//var spawn = require('child_process').spawn,subprocess;
var exec = require('child_process').exec;

var index=0;
var temp = 0;

var dataSize = 256;
var bufferSize = dataSize*2;
var value = new Array(bufferSize/2);
var dataList;
var index2 = 0;

var index3 = 0;
var connected = 0;
function testfunction(data){};

function on_child_stdout(data){
		var k = 0;
		var temp = 0;
		var number;
		//var buffer = new Buffer(2);
		//		buffer.writeUInt16LE(data,0,2);
									//console.log('size :'+data.length)	;
									//		console.log('data :'+data)	;
					console.log('str :'+data)	;
		/*

		 dataList= new Array();
		if(data.length % 2 == 0){
		while(k < data.length)
			{
				temp = data[k]+data[k+1];
				//parseInt(temp);
				
				number = parseInt(temp, 16);
				if(isNaN(number))
				{
					console.log('str :'+data)	;
					return;
				}
				
				dataList.push(number);
				//console.log(number)
				k+=2;
				
			}
		//if(connected == 1)
			{
			k =0;
			io.emit('chat message',dataList);
				console.log(dataList);

					//dataList.splice(0, dataList.length);

			}
		}
		*/
};

app.get('/', function(req, res){

	var query = "ps | grep \"ADStreamServer\" | grep -v grep | awk \'{print $1}\' | xargs kill -9 > /dev/null";
    

 exec(query, function(error, stdout, stderr) {
      	
	  	
res.sendfile('lws_client.html');
	child = exec('./runADServer.sh');
	child.stdout.setEncoding('utf8');
	child.stdout.on('data', on_child_stdout);
	child.stderr.on('data', on_child_stdout);
	child.on('exit', testfunction);

		/*
		for(index2=0;index2<400;index2++){
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
		
		
		//setInterval(function(){
		/*
			fs.read(fd,buffer,0,bufferSize,null,function(err,num){
				value.length=0;
				//value.push(buffer);
				//console.log(value);		
				//console.log(buffer);
 				io.emit('chat message',buffer);
			});
			*/
	
//		},1);
		
//	});
 	});
});

app.get('/processing.js', function(req, res){
  res.sendfile('processing.js');
});

io.on('connection', function(socket){
  console.log('a user connected');
   connected = 1;
  socket.on('disconnect', function(){
    console.log('user disconnected');
  });
  socket.on('chat message', function(msg){
     console.log(msg);
   connected = 0;
//    io.emit('chat message', msg);
  });
});

http.listen(3000, function(){
  console.log('listening on *:3000');
});




