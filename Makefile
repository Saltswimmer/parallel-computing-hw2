PROGRAM = GameOfLife
SOURCES = main.c
OBJECTS = main.o
CFLAGS = -Wall -O3 -fopenmp

.SUFFIXES:
.SUFFIXES: .c .o

.c.o: ; gcc $(CFLAGS) -c $<

$(PROGRAM) : $(OBJECTS)
	gcc -o $(PROGRAM) $(CFLAGS) $(OBJECTS)

.PHONY: clean
clean: ; /bin/rm -f $(PROGRAM) $(OBJECTS)
       
