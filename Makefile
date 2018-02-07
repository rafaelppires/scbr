CXX=g++-5
CXXCFlags=-std=c++17 -g

SGX_SDK := /opt/intel/sgxsdk

#DIRS
OBJDIR  := obj
BINDIR  := bin
LIBDIR  := lib
OUTDIRS := $(OBJDIR) $(BINDIR) $(LIBDIR)

FORWDIR    := src/router/forwarding
CLIENTSDIR := src/client-samples
SGXCOMMDIR := ../sgx_common

USERLIBDIR := src/user-library
USERLIBINC := $(USERLIBDIR)/include
USERLIB    := $(LIBDIR)/libscbr.a

all: scbr tests

scbr: $(USERLIB) $(BINDIR)/scbr

$(BINDIR)/scbr: | $(BINDIR)
	@$(MAKE) -C $(FORWDIR) all && mv $(FORWDIR)/scbr $@ && mv $(FORWDIR)/*.signed.so $(BINDIR) && rm $(FORWDIR)/*.so

tests: minimalist producer consumer scbr

############################## PRODUCER
producer: $(BINDIR)/producer

SAMPLESDIR      := src/client-samples
PRODUCEROBJS    := producer.o
LIBS            := zmq cryptopp m boost_system boost_chrono
$(BINDIR)/producer: $(addprefix $(OBJDIR)/, $(PRODUCEROBJS)) $(USERLIB) | $(BINDIR)
	@echo "LINK $@ <= $^"
	@$(CXX) $^ -o $@ $(addprefix -l, $(LIBS))

############################## CONSUMER
consumer: $(BINDIR)/consumer

CONSUMEROBJS    := consumer.o
$(BINDIR)/consumer: $(addprefix $(OBJDIR)/, $(CONSUMEROBJS)) $(USERLIB) | $(BINDIR)
	@echo "LINK $@ <= $^"
	@$(CXX) $^ -o $@ $(addprefix -l, $(LIBS))

############################## MINIMALIST
minimalist: $(BINDIR)/minimalist

MINIMALISTOBJS := minimalist.o
$(BINDIR)/minimalist: $(addprefix $(OBJDIR)/, $(MINIMALISTOBJS)) $(USERLIB) | $(BINDIR)
	@echo "LINK $@ <= $^"
	@$(CXX) $^ -o $@ $(addprefix -l, $(LIBS))

############################## USER LIBRARY
ULOBJS := message communication_zmq sgx_cryptoall utils scbr_api
USERLIBOBJS := $(addprefix $(OBJDIR)/, $(addsuffix .o, $(ULOBJS)))
$(USERLIB): $(USERLIBOBJS) | $(LIBDIR)
	@ar r $@ $^
	@ranlib $@

test: tests
	@$(BINDIR)/minimalist

INCLUDEDIRS := $(CLIENTSDIR) $(USERLIBDIR)/include $(addprefix src/router/, glue matching) ../sgx_common
CLIENTOBJS := producer.o consumer.o minimalist.o
$(addprefix $(OBJDIR)/,$(CLIENTOBJS)) : $(OBJDIR)/%.o : $(CLIENTSDIR)/%.cpp | $(OBJDIR)
	@echo "CXX $@ <= $^"
	@$(CXX) $(CXXCFlags) -c $^ -o $@ $(addprefix -I, $(INCLUDEDIRS))

$(OBJDIR)/%.o : $(USERLIBDIR)/src/%.cpp | $(OBJDIR) 
	@echo "CXX $@ <= $^"
	@$(CXX) $(CXXCFlags) -c $^ -o $@ $(addprefix -I, $(INCLUDEDIRS))

$(OBJDIR)/%.o: $(SGXCOMMDIR)/%.cpp
	@echo "CXX $@ <= $^"
	@$(CXX) $(CXXCFlags) -c $^ -o $@ $(addprefix -I, $(dir $<))

$(OUTDIRS):
	@mkdir $@

.PHONY: producer consumer minimalist router tests test all clean

clean:
	$(MAKE) -C $(FORWDIR) clean
	rm -rf $(OUTDIRS)

