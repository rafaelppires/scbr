CXX=g++-5

SGX_SDK := /opt/intel/sgxsdk

#DIRS
OBJDIR  := obj
BINDIR  := bin
LIBDIR  := lib
OUTDIRS := $(OBJDIR) $(BINDIR) $(LIBDIR)

FORWDIR := src/router/forwarding

all: router tests

router: $(BINDIR)/router

$(BINDIR)/router: | $(BINDIR)
	@$(MAKE) -C $(FORWDIR) clean all && mv $(FORWDIR)/scbr $@ && mv $(FORWDIR)/*.signed.so $(BINDIR) && rm $(FORWDIR)/*.so

tests: producer consumer router

producer:

consumer:

test: tests

$(OUTDIRS):
	@mkdir $@

.PHONY: router tests test all clean

clean:
	rm -rf $(OUTDIRS)

