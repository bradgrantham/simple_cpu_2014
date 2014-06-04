CXXFLAGS=-I/opt/local/include/ -Wall --std=c++11 -O3
LDFLAGS=-L/opt/local/lib/ -lboost_program_options-mt -lboost_regex-mt

all: memory_test sim hello 

memory_test.o: simple_cpu_2014.hpp util.hpp
hello.o: simple_cpu_2014.hpp util.hpp
util.o: simple_cpu_2014.hpp util.hpp
sim.o: simple_cpu_2014.hpp util.hpp

memory_test: memory_test.o util.o
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

sim: sim.o util.o
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

hello: hello.o util.o
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm memory_test sim hello
