
INCL=-I$(HOME)/incl

#OBJ=chapter03.o

OBJ=apue.o chapter10.o

apue: ${OBJ}
	cc -o ./build/apue $(OBJ)

#chapter03.o: apue.h chapter03.h

apue.o: apue.h

#chapter08.o: apue.h chapter08.h

chapter10.o: apue.h chapter10.h

.PHONY: clean
clean:
	-rm apue $(OBJ)
