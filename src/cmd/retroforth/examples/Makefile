RX = ../

default: clean
	@echo Copying Retro from $(RX)
	@cp $(RX)/retro .
	@cp $(RX)/retroImage .
	@ln -s $(RX)/library .

clean:
	@echo Cleaning up old files...
	@rm -f retro retroImage library *~
