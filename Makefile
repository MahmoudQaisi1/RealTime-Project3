
all: parent 

parent: final.c
	gcc -g final.c -o final -lglut -lGLU -lGL -lm -pthread -std=c99

run:
	./final

clean:
	rm -f final
