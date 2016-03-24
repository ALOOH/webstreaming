var fs = require('fs');
//var colors = require('colors');

var bufferSize = 8;

var read = fs.createReadStream('/mnt/mmc/snow/SS_OLD/ALOOH/src/AnalogSensor/A07', {
	bufferSize: bufferSize
});

read.on('data',function(data){
	//console.log('read: data'.red);
	console.log(data.toString());
	})
  .on('end',   function()          { console.log('read: end');    })
  .on('error', function(e)         { console.log('read: error');  })
  .on('close', function()          { console.log('read: close');  })
  .on('fd',    function(fd)        { console.log('read: fd');     })
;
