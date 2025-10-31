build: libslsf
	gcc main.c tlsflink.c libtlsf.a -o main

libslsf: clean tlsf.c tlsf.h 
	gcc -c tlsf.c
	ar rcs libtlsf.a tlsf.o
	gcc tlsf.o -o libtlsf.so  -shared

clean:
	rm -f tlsf.o libtlsf.a libtlsf.so main