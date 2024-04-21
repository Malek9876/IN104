CC=gcc
CFLAGS=`pkg-config --cflags gtk+-3.0 osmgpsmap-1.0`
LDFLAGS=`pkg-config --libs gtk+-3.0 osmgpsmap-1.0`


all: project.x


%.x: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -lm

clean:
	rm -f *~ *.o

realclean: clean
	rm -f *.x
