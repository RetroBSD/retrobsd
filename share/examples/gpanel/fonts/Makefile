CFLAGS		= -Wall -O -g
LDFLAGS		= -L/opt/local/lib -lpng

FONTS           = digits32.c \
                  digits20.c \
                  lucidasans11.c \
                  lucidasans15.c \
                  lucidasans9.c \
                  lucidasans7.c \
                  verdana7.c \
                  5x7.c \
                  6x9.c

# Latin
CHARSET         = 20_126 164

# Cyrillics: Ё, ё, А-я
#CHARSET         += 1025 1105 1040_1103

all:		$(FONTS)

compile:        $(FONTS)
		$(CC) -c $(FONTS)
		size *.o

# Get Lucida_Sans_Unicode.ttf from internet free font collection:
# http://www.webpagepublicity.com/free-fonts/l/Lucida%20Sans%20Unicode.ttf
# Use otf2bdf utility from ftp://crl.nmsu.edu/CLR/multiling/General/
# Under Ubuntu, install otf2bdf package.
digits32.c:	convbdf ttf/Lucida_Sans_Unicode.ttf
		-otf2bdf -p 32 -l "32 48_58" ttf/Lucida_Sans_Unicode.ttf > digits32.bdf
		./convbdf -a12 -d15 digits32.bdf
		rm -f digits32.bdf

digits20.c:	convbdf ttf/Lucida_Sans_Unicode.ttf
		-otf2bdf -p 20 -l "32 48_58" ttf/Lucida_Sans_Unicode.ttf > digits20.bdf
		./convbdf -a9 -d11 digits20.bdf
		rm -f digits20.bdf

lucidasans11.c:	convbdf ttf/Lucida_Sans_Unicode.ttf
		-otf2bdf -p 11 -l "$(CHARSET)" ttf/Lucida_Sans_Unicode.ttf > lucidasans11.bdf
		./convbdf -u164 -a2 -d3 lucidasans11.bdf
		rm -f lucidasans11.bdf

lucidasans15.c:	convbdf ttf/Lucida_Sans_Unicode.ttf
		-otf2bdf -p 15 -l "$(CHARSET)" ttf/Lucida_Sans_Unicode.ttf > lucidasans15.bdf
		./convbdf -u164 -a5 -d5 lucidasans15.bdf
		rm -f lucidasans15.bdf

lucidasans28.c:	convbdf ttf/Lucida_Sans_Unicode.ttf
		-otf2bdf -p 28 -l "$(CHARSET)" ttf/Lucida_Sans_Unicode.ttf > lucidasans28.bdf
		./convbdf -u164 -a11 -d9 lucidasans28.bdf
		rm -f lucidasans28.bdf

lucidasans9.c:	convbdf ttf/Lucida_Sans_Unicode.ttf
		-otf2bdf -p 9 -l "$(CHARSET)" ttf/Lucida_Sans_Unicode.ttf > lucidasans9.bdf
		./convbdf -u164 -a2 -d3 lucidasans9.bdf
		rm -f lucidasans9.bdf

lucidasans7.c:	convbdf ttf/Lucida_Sans_Unicode.ttf
		-otf2bdf -p 7 -l "$(CHARSET)" ttf/Lucida_Sans_Unicode.ttf > lucidasans7.bdf
		./convbdf -u164 -a2 -d3 lucidasans7.bdf
		rm -f lucidasans7.bdf

# Use /usr/share/fonts/truetype/msttcorefonts/Verdana.ttf
verdana7.c:	convbdf ttf/Verdana.ttf
		-otf2bdf -p 7 -l "$(CHARSET)" ttf/Verdana.ttf > verdana7.bdf
		./convbdf -a2 verdana7.bdf
		rm -f verdana7.bdf

5x7.c:		convbdf bdf/5x7.bdf
		./convbdf -u0 -l0x44f -x0xa0-0x40f bdf/5x7.bdf

6x9.c:		convbdf bdf/6x9.bdf
		./convbdf -u0 -l0x44f -x0xa0-0x40f bdf/6x9.bdf

clean:
		-rm -f *~ *.o convbdf

clean-all:      clean
		-rm -f $(FONTS)
