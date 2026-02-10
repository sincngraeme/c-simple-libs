CC = gcc
OUTDIR = ./bin
BUILDDIR = .
PATHSEP = /
STD = gnu2x
CFLAGS = -Wall -Wextra -g -std=$(STD)
DEFS =
SRC = test/test.c
HDR = csl-test.h
OBJ = $(SRC:.c=.o)
ARGS = #fake.file -sS --custom-message="General Kenobi!"
PROGRAM = test

# Link
all: $(OBJ)
	$(CC) $(OBJ) -o $(OUTDIR)$(PATHSEP)$(PROGRAM)

# Compile
$(OBJ): $(SRC)
	$(CC) $(DEFS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OUTDIR)$(PATHSEP)$(PROGRAM) $(OBJ)

run:
	make && $(OUTDIR)$(PATHSEP)$(PROGRAM)
