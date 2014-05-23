
# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR		= baremetal dip duinomite duinomite-uart duinomite-e \
                  duinomite-e-uart explorer16 max32 max32-eth maximite \
                  meb pinguino-micro starter-kit ubw32 ubw32-uart \
                  ubw32-uart-sdram baremetal fubarino mmb-mx7 maximite-color \
                  32mxsdram-uart

default:

all:
		-for i in $(SUBDIR); do make -C $$i all; done

install:

clean:
		-for i in $(SUBDIR); do make -C $$i clean; done
		find .. -name \*~ | xargs rm -f

reconfig:
		-for i in $(SUBDIR); do echo $$i; \
                I=`echo $$i | awk '{print toupper($$0)}'`; \
                (cd $$i; ../../../tools/configsys/config $$I); done
