CC = gcc
RM = rm -f

TARGETS = demo

all: $(TARGETS)

demo: demo.o niusb6501.o
	$(CC) -o $@ $+ -lusb

%.o: %.c
	$(CC) -O3 -o $@ -c $<

clean:
	$(RM) *.o $(TARGETS)

.PHONY: all clean

