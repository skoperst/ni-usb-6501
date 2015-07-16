CC = gcc
RM = rm -f

TARGETS = demo

all: $(TARGETS)

demo: demo.o niusb6501.o
	$(CC) -g -o $@ $+ -lusb-1.0

%.o: %.c
	$(CC) -g -o $@ -c $<

clean:
	$(RM) *.o $(TARGETS)

.PHONY: all clean

