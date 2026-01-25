CC = gcc
OUTDIR = bin
BUILDDIR = .
PATHSEP = /
CFLAGS = -Wall -Wextra -g -fsanitize=address
LFLAGS = -fsanitize=address
STD = gnu2x
SRC = ./test/smrtptrs.c
OBJ = $(SRC:.c=.o)
ARGS = #fake.file -sS --custom-message="General Kenobi!"
PROGRAM = test

# Link
all: $(OBJ)
	$(CC) $(LFLAGS) $(OBJ) -o $(OUTDIR)$(PATHSEP)$(PROGRAM)

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -std=$(STD) -c $< -o $@

clean:
	rm -f $(OUTDIR)$(PATHSEP)$(PROGRAM) $(OBJ)

run:
	make && $(OUTDIR)$(PATHSEP)$(PROGRAM)

log:
	@echo $(OBJ)
	@echo $(SRC)
