build:
	gcc -o neo1541 -Wall ./*.c ./libs/*.c -lm

clean:
	rm neo1541