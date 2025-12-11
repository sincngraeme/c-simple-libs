CC = gcc
OUTDIR = bin
BUILDDIR = .
PATHSEP = /
CFLAGS = -Wall -Wextra -g
STD = c2x
SRC = main.c
OBJ = main.o
PROGRAM = test

# Link
all: $(OBJ)
	$(CC) $(OBJ) -o $(PROGRAM)

# Compile
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -std=$(STD) -c $< -o $@

clean:
	rm -f $(PROGRAM) $(OBJ)

run:
	make && $(BUILDDIR)$(PATHSEP)$(PROGRAM)
