include vars.mk

all: UntitledOne

UntitledOne:
	$(CC) main.c $(LDFLAGS) -o $(EXE)

install: all
	cp $(EXE) $(PREFIX)/bin/$(EXE)

clean:
	rm ./$(EXE) 
