build: tlsf.c tlsf.h main.c
	@echo "Fine"

libslsf: clean tlsf.c tlsf.h 
	gcc -c tlsf.c -o tlsf.o
	ar rcs libtlsf.a tlsf.o
	gcc -shared -o libtlsf.so tlsf.o

clean:
	rm -f tlsf.o libtlsf.a libtlsf.so