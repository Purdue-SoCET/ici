#DEPS =
OBJS = server.o

CFLAGS = -std=c99
GCC = gcc $(CLFAGS)

ici: ici.c
	$(GCC) ici.c -g -lpthread -lm -o ici

%.o: %.c
	$(GCC) $< -o $@ 

clean:
	#rm *.o
	rm ici
