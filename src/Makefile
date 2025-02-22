%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FPIC) -Wall -c -o $@ $<

clean:
	rm -f remotewol *.o

remotewol: remotewol.o
	$(CC) $(LDFLAGS) -o $@ $^ -ldl

compile: remotewol

install: compile
	mkdir -p $(DESTDIR)/usr/bin
	cp remotewol $(DESTDIR)/usr/bin/remotewol