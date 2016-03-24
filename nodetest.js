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



	var query = "ps | grep \"nodetest\" | grep -v grep | awk \'{print $1}\' | xargs kill -9 > /dev/null";
    

 exec(query, function(error, stdout, stderr) {
      	
	  	
});





