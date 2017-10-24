# Makefile for seismicAnalyze

CC = g++
CFLAGS = -g -I.
OBJ = seismicAnalyze.o

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

seismicAnalyze: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core

