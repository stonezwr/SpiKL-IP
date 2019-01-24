TARGET_ALL = NeuromorphicSim
CXX	= g++ 
CXXFLAGS = -pthread  -std=c++11 -O9

HDRS_ALL=	channel.h speech.h neuron.h synapse.h network.h parser.h simulator.h readout.h util.h def.h

SRCS_ALL=	channel.C speech.C neuron.C synapse.C network.C parser.C simulator.C readout.h util.C main.C

OBJS_ALL=	channel.o speech.o neuron.o synapse.o network.o parser.o simulator.o readout.o util.o main.o


MATOBJS = \

TARGET_TEST = TestUtil
HDRS_TEST=	util.h util_test.h
SRCS_TEST=	util.C util_test.C
OBJS_TEST=	util.o util_test.o


all: $(TARGET_ALL)

$(TARGET_ALL):  $(OBJS_ALL)  
	$(CXX) $(CXXFLAGS) $(OBJS_ALL) -o $(TARGET_ALL)

$(OBJS_ALL): $(HDRS_ALL)


test: $(TARGET_TEST)

$(TARGET_TEST):  $(OBJS_TEST)  
	$(CXX) $(CXXFLAGS) $(OBJS_TEST) -o $(TARGET_TEST)

$(OBJS_TEST): $(HDRS_TEST)



clean:
	/bin/rm -rf $(OBJS_ALL) $(TARGET_ALL)
	/bin/rm -rf $(OBJS_TEST) $(TARGET_TEST)
	/bin/rm -rf *~

