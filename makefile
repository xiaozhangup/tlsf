build: clean libtlsf
# 	gcc main.c tlsflink.c libtlsf.a -o main
	gcc main.c tlsflink.c -L . -l tlsf -o main

libtlsf: tlsf.c tlsf.h 
	gcc -c tlsf.c
	ar rcs libtlsf.a tlsf.o
	gcc tlsf.o -o libtlsf.so -shared

test: build
	export LD_LIBRARY_PATH=.; ./main

clean:
	rm -f tlsf.o libtlsf.a libtlsf.so main