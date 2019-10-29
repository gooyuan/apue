
INCL=-I$(HOME)/incl
OBJ=chapter03.o

c03: ${OBJ}
	cc -o c03 $(OBJ)

chapter03.o: apue.h chapter03.h

.PHONY: clean
clean:
	-rm c03 $(OBJ)
