#!/bin/sh
$APATH/spipower.sh 0
wait
$APATH/spipower.sh 1
wait
timestamp() {
  # date +"%T"
    date +"%Y-%m-%d_%H-%M-%S"
}
   
flag=0
 
pid=$(pidof monitering_wlan0.sh)
if [ $pid ]; then
$NPATH/netled.sh stopAD
flag=1
fi
ps | grep "ADStreamServer" | grep -v grep | awk '{print $1}' | xargs kill -9 > /dev/null
#date 

#TEMP=/mnt/mmc
#echo " -- Set Mode "
#AD1_MODE=$(./readReg AD1 99)
#echo $AD1_MODE

#AD2_MODE=$(./readReg AD2 99)
#echo $AD2_MODE

#AD3_MODE=$(./readReg AD3 99)
#echo $AD3_MODE

AD4_MODE=$(./readReg AD4 99)
#echo $AD4_MODE

#./setMode AD1 $AD1_MODE
#echo ""
#./setMode AD2 $AD2_MODE
#echo ""
#./setMode AD3 $AD3_MODE
#echo ""
#./setMode AD4 1
#./setMode AD4 2 
#echo "" 

nice -20 ./ADStreamServer -W -p0 -f  $TEMP -s 12000000 -t $A_TIMECNT # -v





date

if [ $flag = 1 ]; then 
 $NPATH/netled.sh ClientAD
fi 

sync
#date

