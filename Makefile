build:
	gcc -Wall ./*.c -o neo1541

run:
	sudo ./neo1541

clean:
	rm neo1541