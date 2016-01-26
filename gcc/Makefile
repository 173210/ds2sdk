
CC := /ppt/mipsel-4.1.2-nopic/bin/mipsel-linux-gcc
CURR_DIR = ${shell pwd}

all: $(CC)


$(CC):
	mkdir -p /ppt
	cd /ppt;tar -xjf $(CURR_DIR)/mipsel-4.1.2-nopic.tar.bz2
	chmod 777 $(CURR_DIR)/../tools/makeplug

