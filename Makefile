
all:  fs_expect

CCFLAGS = -c -g -O0 -D_GNU_SOURCE -Wall -pthread
LDFLAGS = -Wl,-v -Wl,-Map=a.map -Wl,--cref -Wl,-t -lpthread -pthread
#LDFLAGS = -lpthread -pthread
ARFLAGS = -rcs

# buildroot passes these in
CC = gcc
LD = gcc
AR = ar


fs_expect: fln_serial.o Makefile main.o ffile.o frbuff.o fprintbuff.o
	$(LD) $(LDFLAGS) -o fs_expect  fln_serial.o main.o ffile.o fprintbuff.o frbuff.o 

main.o: main.c
	$(CC)  $(CCFLAGS) -o main.o  main.c
fln_serial.o: fln_serial.c fln_serial.h
	$(CC)  $(CCFLAGS) -o fln_serial.o  fln_serial.c
ffile.o: ffile.c  ffile.h
	$(CC)  $(CCFLAGS) -o ffile.o  ffile.c
frbuff.o: frbuff.c frbuff.h
	$(CC)  $(CCFLAGS) -o frbuff.o  frbuff.c

fprintbuff.o: fprintbuff.c fprintbuff.h
	$(CC)  $(CCFLAGS) -o fprintbuff.o  fprintbuff.c




clean:
	rm -rf *.o
	rm -rf *.map
	rm -rf fs_expect

.phony x:
x:
	./fs_expect


