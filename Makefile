CC:=gcc
AS:=as

INCDIR:=include
BIN:=libopentac.so
TEST:=run_test

TESTSRC:=test.c
SRC:=lib.c print.c regalloc.c pass.c grammar.tab.c lex.yy.c
OBJ:=lib.o print.o regalloc.o pass.o grammar.tab.o lex.yy.o
INC:=$(INCDIR)/opentac.h grammar.tab.h

CFLAGS:=-g -ggdb -Wall -Wextra -pedantic -std=c11 -Wno-unused-function -D_GNU_SOURCE=1 -fPIC
LDFLAGS:=-lm
ASFLAGS:=

.PHONY: all build clean mrproper

all: $(BIN)

build: $(BIN)

test: $(TEST)
	LD_LIBRARY_PATH=. ./$(TEST) ./examples/sanity.tac

$(TEST): $(TESTSRC) $(BIN)
	$(CC) -o $@ $(CFLAGS) $(TESTSRC) $(LDFLAGS) -L. -lopentac

$(BIN): $(OBJ) $(INC)
	$(CC) -shared -o $(BIN) $(OBJ) $(LDFLAGS)

$(OBJ): %.o: %.c $(INC)
	$(CC) -c -o $@ $< $(CFLAGS)

grammar.tab.c: grammar.y
	bison $^

grammar.tab.h: grammar.y
	bison --defines $^

lex.yy.c: lex.l grammar.tab.h
	flex $<

clean:
	rm -rf $(OBJ)

mrproper: clean
	rm -rf $(BIN)
	rm -rf $(TEST)
	rm -f grammar.tab.c
	rm -f grammar.tab.h
	rm -f lex.yy.c
