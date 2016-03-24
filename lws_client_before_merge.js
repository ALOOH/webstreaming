<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <script src="http://ajax.googleapis.com/ajax/libs/jquery/1.7.2/jquery.min.js"></script>
        <script type="text/javascript">
				var recvBufArray= new Array();
var byteArray;
var websocket;
var clickedStop = 0;
var isFFT = 0;
var num = 0;
var frameRateTest = 0;
			function onStartClick()
			{
			clickedStop = 0;
			if(document.getElementById("mode").value == 2)
				isFFT = 1;
			else 
				isFFT = 0;
				websocket.send('start;channel='+document.getElementById("channel").value+';mode='+document.getElementById("mode").value);	
				alert(document.getElementById("channel").value);
				document.getElementById("start").disabled = true;
				document.getElementById("stop").disabled = false;
			}
			
			function onStopClick()
			{
			clickedStop = 1;
				document.getElementById("start").disabled = false;
				document.getElementById("stop").disabled = true;
				alert(document.getElementById("channel").value);
				 websocket.send('stop');	
			}
            function connect() {
			
			document.getElementById("start").disabled = false;
			document.getElementById("stop").disabled = true;
			
                window.WebSocket = window.WebSocket || window.MozWebSocket;

                 websocket = new WebSocket('ws://192.168.0.188:9000', // <---- 요기 수정
                    'dumb-increment-protocol');

                websocket.onopen = function () {
                    $('h1').css('color', 'green');
                };

                websocket.onerror = function () {
                    $('h1').css('color', 'red');
                };

                websocket.onmessage = function (message) {
                    //$('div').append($('<p>', { text: 'received'}));
                    //console.log(message.data);
                    //console.log(message.data);
					var reader = new FileReader();
					reader.onload = function() {
					//$('div').append($('<p>', { text: 'loaded'}));
				//arrayBuffer = this.result;
				    // var array5 =  new Uint8Array(this.result);
					  //for(var i=0;i<array5.length ;i++)
					//$('div').append($('<p>', { text: array5[i]}));
					//if( isFFT == 0)
					
					recvBufArray =recvBufArray.concat(Array.prototype.slice.call(new Float32Array(this.result))); 
					console.log(recvBufArray[10]);
					document.getElementById("stop").value = recvBufArray.length;
					//else
					//recvBufArray =recvBufArray.concat(Array.prototype.slice.call(new Uint32Array(this.result))); 
					};
					
					reader.readAsArrayBuffer(message.data);
                };

/*
                $('button').click(function(e) {
                    e.preventDefault();
                    websocket.send($('input').val());
                    $('input').val('');
                });
				*/
            }
        </script>
    </head>
    <body>
        <h1>WebSockets test</h1>
        <form>
            //<input type="text" />
			<h4>채널 선택 </h4>
			<select name="channel" id="channel">
    <option value=1>1</option>
    <option value=2>2</option>
    <option value=3>3</option>
    <option value=4>4</option>
</select>
			<h4>모드 선택 </h4>
			<select name="mode" id="mode">
    <option value=1>TIME</option>
    <option value=2>FFT</option>
    <option value=3>FIR</option>
    <option value=4>RMS</option>
</select>
<div>
			<input id="do_connect" type="button" onclick="connect()" value="Connect"/>
            <input id="start" type="button" onclick="onStartClick()" value="Start"/>
			<input id="stop" type="button" onclick="onStopClick()" value="Stop" disabled="true"/>
			
			</div>
        </form>
        <div>  	<canvas id="mycanvas" width=1024 height=600></canvas>
</div>
		<script src="processing.js"></script>
	<script type="text/processing" data-processing-target="mycanvas">
	
	int Xmax = 512;
	int Ymax = 256;
	int Yref = Ymax/2;
	int q = 256;
	void setup()
	{	
	  int i; 
	int index = 0;
	byte lower = 0;
	char upper=0;
	int bufSize = 0;
	  frameRate(1);
	  size(1024,600);
	  background(125);
	 // fill(255);
	  //noLoop();
	 // PFont fontA = loadFont("courier");
	 // textFont(fontA, 14);  
	}

	void draw(){  
	int j = 0;
	char pointX1 = 0;
	char pointX2 = 0;
	int pointX3 = 0;
	int pointX4 = 0;
	int tempindex = 0;
	
	int xArixIndex = 0;
	upper = 0;
	if(frameRateTest++ > 5)
	{
	frameRate(50);
	
	}
	println("frameRateTest "+frameRateTest);
	 //println("recvBufArray size :"+recvBufArray.length);

	if(recvBufArray != null)
	{
		 if(recvBufArray.length> Xmax)
		 {
			
				tempbuf	  = recvBufArray.splice(0, Xmax);
			if(clickedStop == 1)
			{
				if(recvBufArray.length > 0)
				recvBufArray = recvBufArray.splice(0, 0);
				
			}
			else
			{
			background(125);
				stroke(255);
			}
		  	j = Xmax*2-2;
		  	index = 0;
/*
		  	while(j > 0  )
		  	{
				lower = (parseInt(tempbuf[index]));
				upper = byte(parseInt(tempbuf[index+1]))*256;

				upper += lower;
				pointX1= upper;

				lower = (parseInt(tempbuf[index+2]));
				upper = byte(parseInt(tempbuf[index+3]))*256;

				upper += lower;
				pointX2= upper;


				line(j,pointX1/10+300,j-1,pointX2/10+300);
				j --;
				index +=2;
			}
	*/		
			
			
		  	while(j > 0  )
		  	{
			
				if(isFFT == 0)
				{
					lower = (parseInt(tempbuf[index]));
					upper = byte(parseInt(tempbuf[index+1]))*256;

					upper += lower;
					pointX1= upper;

					lower = (parseInt(tempbuf[index+2]));
					upper = byte(parseInt(tempbuf[index+3]))*256;

					upper += lower;
					pointX2= upper;
					index +=2;
				}
				else
				{
				//println(tempbuf[index]+'   '+tempbuf[index]+1+'   '+tempbuf[index+2]+'   '+tempbuf[index+3]);
					//pointX1 = tempbuf[index].toFixed(1);
					//pointX2 = tempbuf[index+1].toFixed(1);
					
				}
				//println(pointX1+'   '+pointX2);
				line(j,tempbuf[index+1]+300,j+1,tempbuf[index]+300);
				//println("index "+index+" j:"+j);
				index+=1;
				j-=2;
				//println("recvBufArray size :"+recvBufArray.length);
				
			}

			

	
		  }
	}
	else
	 println("recvBufArray is null");
	//if(i > 0)
	//println("testdata"+list.get(i-1));
	
	}
	</script>
    </body>
</html>