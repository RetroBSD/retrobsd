# Set this to the retro root directory
#
# You should have at least a 'retro' binary, the 'retroImage', and
# 'library' directory there
#
# Note that while you can run Rancid under the Python VM, we need
# the C VM for building (at present)

RX = ~/code/rx

default:
	@cp $(RX)/retroImage .
	@ln -s $(RX)/library .
	$(RX)/retro --with rancid.rx --shrink
	@rm library

clean:
	@rm -f retroImage
