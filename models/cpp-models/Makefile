CXXFLAGS=-std=c++11 -g -O2 -Wall -Wextra -Weffc++ -pedantic
HEADERS=machines.hh model.hh

all: method1.o method2.o method4.o

method1.o: method1.cc method1.hh $(HEADERS)

method2.o: method1.cc method1.hh $(HEADERS)

method4.o: method1.cc method1.hh $(HEADERS)

clean:
	@rm -f method1.o method2.o method4.o
	@echo "cleaned..."

