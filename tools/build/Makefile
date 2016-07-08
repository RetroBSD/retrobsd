BOARD_LIST      = duinomite duinomite-emega fubarino max32 maximite maximite-color \
                  picadillo pinguino-micro sdxl snadpic ubw32

SKEL_SCRATCH    = skeleton/linux32/pic32prog \
                  skeleton/linux64/pic32prog \
                  skeleton/macosx/pic32prog \
                  skeleton/pic32prog.exe \
                  skeleton/sdcard.img \
                  skeleton/unix.dis \
                  skeleton/unix.hex

PIC32PROG       = https://raw.githubusercontent.com/sergev/pic32prog/master

DATE            := $(shell date +'%Y-%m-%d')

all:            build sdcard-$(DATE).zip
		for b in $(BOARD_LIST); do \
		    $(MAKE) retrobsd-$$b-$(DATE).zip TARGET=$$b; \
                done

clean:
		rm -rf *.zip $(wildcard retrobsd-*-????-??-??)

clean-all:
		rm -f $(SKEL_SCRATCH) build-$(DATE).log

build:          build-$(DATE).log

sdcard-$(DATE).zip: ../../sdcard.img
		zip -jq $@ ../../sdcard.img

retrobsd-$(TARGET)-$(DATE).zip: update-skeleton
		@rm -rf retrobsd-$(TARGET)-$(DATE) $@
		cp -rp skeleton retrobsd-$(TARGET)-$(DATE)
		ln ../../sdcard.img retrobsd-$(TARGET)-$(DATE)/sdcard.img
		ln ../../sys/pic32/$(TARGET)/unix.hex retrobsd-$(TARGET)-$(DATE)/unix.hex
		ln ../../sys/pic32/$(TARGET)/unix.dis retrobsd-$(TARGET)-$(DATE)/unix.dis
		zip -rq $@ retrobsd-$(TARGET)-$(DATE)

build-$(DATE).log:
		make -C ../.. cleanall > /dev/null
		echo === RetroBSD Revision $(shell git rev-list HEAD --count), Commit Id $(shell git rev-parse --short HEAD) === > build-$(DATE).log
		make -C ../../tools >> build-$(DATE).log 2>&1
		make -C ../../lib >> build-$(DATE).log 2>&1
		make -C ../../src install >> build-$(DATE).log 2>&1
		make -C ../.. fs >> build-$(DATE).log 2>&1
		make -C ../../sys/pic32 all >> build-$(DATE).log 2>&1

update-skeleton: skeleton/linux32/pic32prog skeleton/linux64/pic32prog \
                 skeleton/macosx/pic32prog skeleton/pic32prog.exe \

skeleton/linux32/pic32prog:
		(cd skeleton/linux32 && \
		curl -o pic32prog $(PIC32PROG)/linux32/pic32prog)

skeleton/linux64/pic32prog:
		(cd skeleton/linux64 && \
		curl -o pic32prog $(PIC32PROG)/linux64/pic32prog)

skeleton/macosx/pic32prog:
		(cd skeleton/macosx && \
		curl -o pic32prog $(PIC32PROG)/macosx/pic32prog)

skeleton/pic32prog.exe:
		(cd skeleton && \
		curl -o pic32prog.exe $(PIC32PROG)/pic32prog.exe)
