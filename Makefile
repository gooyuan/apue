
INCL=-I$(HOME)/incl

#OBJ=chapter03.o

OBJ=apue.o chapter08.o

apue: ${OBJ}
	cc -o apue $(OBJ)

#chapter03.o: apue.h chapter03.h

apue.o: apue.h

chapter08.o: apue.h chapter08.h

.PHONY: clean
clean:
	-rm apue $(OBJ)
