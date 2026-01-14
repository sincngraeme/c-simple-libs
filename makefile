CC = gcc
OUTDIR = bin
BUILDDIR = .
PATHSEP = /
CFLAGS = -Wall -Wextra -g
STD = gnu2x
SRC = ./test/enums.c
HDR = csl-enums.h
OBJ = $(SRC:.c=.o)
ARGS = #fake.file -sS --custom-message="General Kenobi!"
PROGRAM = test

# Link
all: $(OBJ)
	$(CC) $(OBJ) -o $(OUTDIR)$(PATHSEP)$(PROGRAM)

# Compile
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -std=$(STD) -c $< -o $@

clean:
	rm -f $(OUTDIR)$(PATHSEP)$(PROGRAM) $(OBJ)

run:
	make && $(OUTDIR)$(PATHSEP)$(PROGRAM)
