var fs = require('fs');

fs.open('/mnt/mmc/snow/SS_OLD/ALOOH/src/AnalogSensor/A07', 'r', function(status, fd) {
    if (status) {
        console.log(status.message);
        return;
    }
    var buffer = new Buffer(100);
    fs.read(fd, buffer, 0, 100, 0, function(err, num) {

	var index= 0;
	for(index=0;index<100;index++){
		console.log(buffer[index]);
	}
        //console.log(buffer.toString('utf-8', 0, num));
    });
});
