
# Device Driver Makefile
CC = arm-linux-gcc
KERNELDIR = /usr/local/linux-2.4.19-rmk4-pax2-empx1
INCLUDEDIR = -I$(KERNELDIR)/include -I./
CFLAGS = -D__KERNEL__ -DMODULE -Wall -O2 -I$(INCLUDEDIR)
TEST_TARGET = user_app

all: $(TEST_TARGET) gpioport.o segmentport.o textlcdport.o pushbuttonport.o discreteledport.o 

segmentport.o :
	$(CC) $(CFLAGS) -c segmentport.c -w
gpioport.o :
	$(CC) $(CFLAGS) -c gpioport.c -w
pushbuttonport.o :
	$(CC) $(CFLAGS) -c pushbuttonport.c -w
discreteledport.o  :
	$(CC) $(CFLAGS) -c discreteledport.c -w
textlcdport.o :
	$(CC) $(CFLAGS) -c textlcdport.c -w
$(TEST_TARGET) : 
	$(CC) -o user_app user_app.c

clean:
	rm -f *.o
	rm -f $(TEST_TARGET)

