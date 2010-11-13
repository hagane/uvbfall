LDFLAGS = -lglut -lGLU -lGL -lSDL -lm -lfftw3
CFLAGS = -Wall -std=c99 -march=native

all : uvbfall

.PHONY: clean all
clean:
	$(RM) *.o uvbfall
