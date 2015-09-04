
# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR		= baremetal duinomite duinomite-emega explorer16 \
                  fubarino max32 maximite maximite-color meb mmb-mx7 \
                  picadillo pinguino-micro sdxl starter-kit ubw32 wf32

default:

all:
		-for i in $(SUBDIR); do ${MAKE} -C $$i all; done

install:

clean:
		-for i in $(SUBDIR); do ${MAKE} -C $$i clean; done
		find .. -name \*~ | xargs rm -f

reconfig:
		-for i in $(SUBDIR); do echo $$i; \
                I=`echo $$i | awk '{print toupper($$0)}'`; \
                (cd $$i; ../../../tools/configsys/config $$I); done
