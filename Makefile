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

TESTEXECS := minimalist user_friendly producer consumer
tests: scbr $(TESTEXECS)

############################## TESTS
$(TESTEXECS) : % : $(BINDIR)/%

SAMPLESDIR      := src/client-samples
LIBS            := zmq cryptopp m boost_system boost_chrono pthread
$(addprefix $(BINDIR)/, $(TESTEXECS)): $(BINDIR)/% : $(OBJDIR)/%.o $(USERLIB) | $(BINDIR)
	@echo "LINK $@ <= $^"
	@$(CXX) $^ -o $@ $(addprefix -l, $(LIBS))

############################## USER LIBRARY
ULOBJS := message communication_zmq sgx_cryptoall sgx_utils_rp utils scbr_api
USERLIBOBJS := $(addprefix $(OBJDIR)/, $(addsuffix .o, $(ULOBJS)))
$(USERLIB): $(USERLIBOBJS) | $(LIBDIR)
	@ar r $@ $^
	@ranlib $@

test: tests
	@./bin/scbr -c localhost:6666 -c localhost:6667 > /dev/null &
	@sleep 3
	@timeout 1s $(BINDIR)/minimalist > /dev/null && echo "PASSED" || echo "FAILED"
	@timeout 1s $(BINDIR)/user_friendly > /dev/null && echo "PASSED" || echo "FAILED"
	@killall -9 scbr

INCLUDEDIRS := $(CLIENTSDIR) $(USERLIBDIR)/include $(addprefix src/router/, glue matching) ../sgx_common
CLIENTOBJS := $(addsuffix .o, $(TESTEXECS))
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

.PHONY: $(TESTEXECS) router tests test all clean

clean:
	$(MAKE) -C $(FORWDIR) clean
	rm -rf $(OUTDIRS)

