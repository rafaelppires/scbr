CXX=g++-5

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

tests: producer consumer scbr

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

############################## USER LIBRARY
USERLIBOBJS := $(addprefix $(OBJDIR)/, message.o communication_zmq.o sgx_cryptoall.o utils.o)
$(USERLIB): $(USERLIBOBJS) | $(LIBDIR)
	@ar r $@ $^
	@ranlib $@

test: tests

INCLUDEDIRS := $(CLIENTSDIR) $(USERLIBDIR)/include $(addprefix src/router/, glue matching) ../sgx_common
CLIENTOBJS := producer.o consumer.o
$(addprefix $(OBJDIR)/,$(CLIENTOBJS)) : $(OBJDIR)/%.o : $(CLIENTSDIR)/%.cpp | $(OBJDIR)
	@echo "CXX $@ <= $^"
	@$(CXX) -c $^ -o $@ $(addprefix -I, $(INCLUDEDIRS))

$(OBJDIR)/%.o : $(USERLIBDIR)/src/%.cpp | $(OBJDIR) 
	@echo "CXX $@ <= $^"
	@$(CXX) -c $^ -o $@ $(addprefix -I, $(INCLUDEDIRS))

$(OBJDIR)/%.o: $(SGXCOMMDIR)/%.cpp
	@echo "CXX $@ <= $^"
	@$(CXX) -std=c++11 -c $^ -o $@ $(addprefix -I, $(dir $<))

$(OUTDIRS):
	@mkdir $@

.PHONY: producer consumer router tests test all clean

clean:
	$(MAKE) -C $(FORWDIR) clean
	rm -rf $(OUTDIRS)

