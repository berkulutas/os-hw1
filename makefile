build:
	g++ -g eshell.cpp parser.c -o eshell

clean:
	rm eshell