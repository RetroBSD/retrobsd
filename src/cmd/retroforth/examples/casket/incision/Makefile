# Set this to the retro root directory
#
# You should have at least a 'retro' binary, the 'retroImage', and
# 'library' directory there
#
# Note that while you can run Rancid under the Python VM, we need
# the C VM for building (at present)

RX = ~/code/rx

default: clean
	cp $(RX)/retroImage .
	@ln -s $(RX)/library .
	clang -Wall $(RX)/src/vm/retro.c -o retro
	./retro --with incision.rx --shrink
	chmod 666 retroImage
	chmod 666 current

clean:
	rm -f retroImage nextImage library retro
