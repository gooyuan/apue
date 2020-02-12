
INCL=-I$(HOME)/incl

#OBJ=chapter03.o

OBJ=apue.o chapter11.o

apue: ${OBJ}
	cc -pthread -o ./build/apue $(OBJ)

#chapter03.o: apue.h chapter03.h

apue.o: apue.h

#chapter08.o: apue.h chapter08.h

#chapter10.o: apue.h chapter10.h

chapter11.o: apue.h chapter11.h

.PHONY: clean
clean:
	-rm ./build/apue $(OBJ)
