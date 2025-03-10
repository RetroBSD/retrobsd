
# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR		= explorer16 olimex


all: subdirs

.PHONY: subdirs $(SUBDIR)

subdirs: $(SUBDIR)

$(SUBDIR):
	$(MAKE) -C $@

clean:
		-for i in $(SUBDIR); do ${MAKE} -C $$i clean; done

clean-all:
		-for i in $(SUBDIR); do ${MAKE} -C $$i clean-all; done

reconfig:
		-for i in $(SUBDIR); do ${MAKE} -C $$i reconfig; done
