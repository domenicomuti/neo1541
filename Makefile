build:
	gcc -o neo1541 -Wall ./*.c ./libs/*.c -lm -lncursesw

clean:
	rm neo1541