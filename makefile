.SUFFIXES : .c .o

CC = gcc

	
LIBRARY	+= -lpthread -lwebsockets -lz
			
CFLAGS = -g $(INC) 
				
OBJS = analogSensor.o Httpd.o gpio.o ss_spi.o Linked_queue.o websocketserver.o
SRCS = analogSensor.c Httpd.c gpio.c ss_spi.c Linked_queue.c websocketserver.c
					
TARGET = ADStreamServer
						
all : $(TARGET)
											
$(TARGET) : $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LIBRARY)
																
$(OBJS) : $(SRCS)
	$(CC) -c $(SRCS)
																										
dep :
	gccmakedep  $(SRCS)
																														
clean :
	rm -rf $(OBJS) $(TARGET)
																																													
new : 
	$(MAKE) clean 
	$(MAKE)# DO NOT DELETE
