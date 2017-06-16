CXX        = g++ -c
LD         = g++
DEFINES    =
INCS       = -I/opt/local/include
LIBS       = -L/opt/local/lib
CXXFLAGS   = -O -fpic $(DEFINES) -w -I. $(INCS)
LDFLAGS    = -O $(LIBS)
#CXXFLAGS   = -g -fpic $(DEFINES) -w -I. $(INCS)
#LDFLAGS    = -g $(LIBS)
LDLIBFLAGS = -shared

SRCS = cbr.cc subscription.cc event.cc graph.cc util.cc prefilter.cc
OBJS = $(SRCS:.cc=.o)
BINS = cbr

all: $(BINS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) $< -o $@

cbr:	$(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

depend:
	makedepend -Y -I. $(SRCS)

clean:
	rm -f $(BINS) *.o
# DO NOT DELETE

cbr.o: cbr.hh common.hh event.hh subscription.hh graph.hh util.hh
cbr.o: prefilter.hh
subscription.o: subscription.hh common.hh event.hh
event.o: event.hh common.hh
graph.o: graph.hh common.hh util.hh subscription.hh event.hh
util.o: util.hh common.hh
prefilter.o: prefilter.hh common.hh util.hh subscription.hh event.hh graph.hh
