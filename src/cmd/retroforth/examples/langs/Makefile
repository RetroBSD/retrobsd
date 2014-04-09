default: clean bf_helloImage bf_squaresImage

clean:
	rm -f bf_helloImage bf_squaresImage

bf_helloImage:
	../retro --image ../retroImage --with examples/hello.bf
	mv appImage bf_helloImage

bf_squaresImage:
	../retro --image ../retroImage --with examples/squares.bf
	mv appImage bf_squaresImage

