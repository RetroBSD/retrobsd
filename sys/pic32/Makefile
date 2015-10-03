
# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR		= baremetal duinomite duinomite-emega explorer16 \
                  fubarino max32 maximite maximite-color mmb-mx7 \
                  picadillo pinguino-micro sdxl snadpic starter-kit \
                  ubw32 wf32

default:

all:
		-for i in $(SUBDIR); do ${MAKE} -C $$i all; done

install:

clean:
		-for i in $(SUBDIR); do ${MAKE} -C $$i clean; done

clean-all:
		-for i in $(SUBDIR); do ${MAKE} -C $$i clean-all; done

reconfig:
		-for i in $(SUBDIR); do ${MAKE} -C $$i reconfig; done

configsys:
		-for i in $(SUBDIR); do echo $$i; \
                I=`echo $$i | awk '{print toupper($$0)}'`; \
                if [ ! -f $$i/Config ]; then \
                (cd $$i; ../../../tools/configsys/config $$I); fi; done
