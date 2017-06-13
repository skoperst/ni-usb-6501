CC = gcc
RM = rm -f

TARGETS = demo niusb6218



all: $(TARGETS)

niusb6218: niusb6218.o
	$(CC) -g -o $@ $+ -lusb-1.0 -lpthread
	
demo: demo.o niusb6501.o
	$(CC) -g -o $@ $+ -lusb-1.0

%.o: %.c
	$(CC) -g -o $@ -c $<

clean:
	$(RM) *.o $(TARGETS)

.PHONY: all clean

