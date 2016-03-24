var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var fs = require('fs');

var index=0;
var temp = 0;

var dataSize = 512;
var bufferSize = dataSize*2;
var value = new Array(bufferSize/2);
var index2 = 0;

/*
var read = fs.createReadStream('/tmp/A09', {
	bufferSize: bufferSize
});
*/
var fs = require('fs');

fs.open('/tmp/A09','r', function(status,fd){
	if(status){
		console.log(status.message);
		return;
	}
	var buffer = new Buffer(bufferSize);
	setInterval(function(){
	fs.read(fd,buffer,0,bufferSize,null,function(err,num){
		value.length=0;
		for(index=0;index<bufferSize/2;index++){
			value.push((buffer[index*2+1]*256+buffer[index*2])/256); 
		}
		console.log(value);		
	});
	
	},10);
});

/*
read.on('data',function(data){
	value.length=0;
	for(index=0;index<bufferSize/2;index++){
		value.push((data[index*2+1]*256+data[index*2])/256);
	}
	console.log(index2++);
	})
  .on('end',   function()          { console.log('read: end');    })
  .on('error', function(e)         { console.log('read: error');  })
  .on('close', function()          { console.log('read: close');  })
  .on('fd',    function(fd)        { console.log('read: fd');     })
;
*/






