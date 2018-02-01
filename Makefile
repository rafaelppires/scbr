CXX=g++-5

SGX_SDK := /opt/intel/sgxsdk

#DIRS
OBJDIR  := obj
BINDIR  := bin
LIBDIR  := lib
OUTDIRS := $(OBJDIR) $(BINDIR) $(LIBDIR)

FORWDIR := src/router/forwarding

all: scbr tests

scbr: $(BINDIR)/scbr

$(BINDIR)/scbr: | $(BINDIR)
	@$(MAKE) -C $(FORWDIR) all && mv $(FORWDIR)/scbr $@ && mv $(FORWDIR)/*.signed.so $(BINDIR) && rm $(FORWDIR)/*.so

tests: producer consumer scbr

producer: $(BINDIR)/producer

SAMPLESDIR      := src/client-samples
PRODUCEROBJS    := producer.o
$(BINDIR)/producer: $(addprefix $(OBJDIR)/, $(PRODUCEROBJS)) | $(OBJDIR)
	@$(CXX) $^ -o $@

consumer: $(BINDIR)/consumer

CONSUMEROBJS    := consumer.o
$(BINDIR)/consumer: $(addprefix $(OBJDIR)/, $(CONSUMEROBJS)) | $(OBJDIR)
	@$(CXX) $^ -o $@

test: tests

$(OUTDIRS):
	@mkdir $@

.PHONY: router tests test all clean

clean:
	$(MAKE) -C $(FORWDIR) clean
	rm -rf $(OUTDIRS)

