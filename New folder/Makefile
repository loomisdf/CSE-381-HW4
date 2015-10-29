CC = gcc
CCOPTS = -c -g -Wall -ggdb 
LINKOPTS = -g -pthread

all: enzyme

enzyme: enzyme.o smp2_tests.o testrunner.o
	$(CC) $(LINKOPTS) -o $@ $^

smp2_tests.o: smp2_tests.c enzyme.h
	$(CC) $(CCOPTS) -o $@ smp2_tests.c

enzyme.o: enzyme.c enzyme.h
	$(CC) $(CCOPTS) -o $@ enzyme.c

testrunner.o: testrunner.c testrunner.h
	$(CC) $(CCOPTS) -o $@ $<

test: enzyme
	./enzyme -test -f0 all

clean:
	rm -rf enzyme enzyme.exe *.o *~ *.err *.out
