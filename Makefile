CPPFLAGS     = -I/usr/local/include -Isrc -Wall -Wextra -Wpedantic
CXXFLAGS     = -DHAVE_CXX_STDHEADERS -D_GNU_SOURCE -D_REENTRANT -O3 -std=c++20
LDFLAGS      = -L/usr/local/lib
LDLIBS       = -llmdb -lsqlparser
TEST_LDLIBS := -lgtest -lgtest_main -pthread

SRCS := $(wildcard src/*.cpp)
TESTS := $(wildcard tests/*.cpp)

OBJS := $(SRCS:.cpp=.o)
TEST_OBJS := $(filter-out src/main.o, $(OBJS)) $(TESTS:.cpp=.o)

MAIN := lmdb-lab
TEST := test

all: $(MAIN)

$(MAIN): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(TEST): LDLIBS += $(TEST_LDLIBS)
$(TEST): $(TEST_OBJS)
	$(CXX) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) src/*.o tests/*.o data/example.mdb/*.mdb $(MAIN) $(TEST)

db-clean:
	$(RM) data/example.mdb/*.mdb

