SUBDIRS         = $(shell find * -maxdepth 0 -type d)
SUBDIR_ALL      = $(SUBDIRS:=.all)
SUBDIR_SIM      = $(SUBDIRS:=.sim)
SUBDIR_CLEAN    = $(SUBDIRS:=.clean)

all: $(SUBDIR_ALL)
sim: $(SUBDIR_SIM)
clean: $(SUBDIR_CLEAN)

$(SUBDIR_ALL): %.all:
	make -C $* all

$(SUBDIR_SIM): %.sim:
	unbuffer make -C $* clean sim | tee sim.out && ! grep -iqE "fail|error:" sim.out

$(SUBDIR_CLEAN): %.clean:
	make -C $* clean
