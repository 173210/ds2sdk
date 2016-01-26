
# CROSS :=#
CROSS := /opt/mipsel-4.1.2-nopic/bin/

CC = $(CROSS)mipsel-linux-gcc
AR = $(CROSS)mipsel-linux-ar rcsv
LD	= $(CROSS)mipsel-linux-ld
OBJCOPY	= $(CROSS)mipsel-linux-objcopy
NM	= $(CROSS)mipsel-linux-nm
OBJDUMP	= $(CROSS)mipsel-linux-objdump

TOPDIR = .
EXAMPLE_DIR = $(TOPDIR)/src
FS_DIR = $(TOPDIR)/../libsrc/fs
CONSOLE_DIR = $(TOPDIR)/../libsrc/console
KEY_DIR = $(TOPDIR)/../libsrc/key

SRC := $(EXAMPLE_DIR)/main.c \
		$(EXAMPLE_DIR)/ds2_main.c \
		$(EXAMPLE_DIR)/bitmap.c	\
		$(EXAMPLE_DIR)/wave.c \

SSRC :=

LIBS := ../lib/libds2b.a -lc -lm -lgcc
EXTLIBS := ../lib/libds2a.a

INC := -I../include -I$(EXAMPLE_DIR) -I$(FS_DIR) -I$(CONSOLE_DIR) -I$(KEY_DIR)

CFLAGS := -mips32 -O3 -mno-abicalls -fno-pic -fno-builtin \
	   -fno-exceptions -ffunction-sections -mlong-calls\
	   -fomit-frame-pointer -msoft-float -G 4

LINKS := ../specs/link.xn
STARTS := ../specs/start.S
STARTO := start.o

#OBJS	:= $(addsuffix .o , $(basename $(notdir $(SRC))))
#SOBJS	:= $(addsuffix .o , $(basename $(notdir $(SSRC))))

OBJS	:= $(SRC:.c=.o)
SOBJS	:= $(SSRC:.S=.o)


APP	:= example.elf


all: $(APP)
	$(OBJCOPY) -O binary $(APP) example.bin
	$(OBJDUMP) -d $(APP) > example.dump
	$(NM) $(APP) | sort > example.sym
	$(OBJDUMP) -h $(APP) > example.map
	../tools/makeplug example.bin demoplug.plg

$(APP):	depend $(SOBJS) $(OBJS) $(STARTO) $(LINKS) $(EXTLIBS)
	$(CC) -nostdlib -static -T $(LINKS) -o $@ $(STARTO) $(SOBJS) $(OBJS) $(EXTLIBS) $(LIBS)

$(EXTLIBS): 
	make -C ../source/

$(STARTO):
	$(CC) $(CFLAGS) $(INC) -o $@ -c $(STARTS)

.c.o:
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<
.cpp.o:
	$(CC) $(CFLAGS) $(INC) -fno-rtti -fvtable-gc -o $@ -c $<
.S.o:
	$(CC) $(CFLAGS) $(INC) -D_ASSEMBLER_ -D__ASSEMBLY__ -o $@ -c $<

clean:
	rm -fr *.o $(OBJS) $(OTHER) *.bin *.sym *.map *.dump *.lib
	rm depend

depend:	Makefile
	$(CC) -MM $(CFLAGS) $(INC) $(SSRC) $(SRC) > $@

sinclude depend

