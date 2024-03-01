FLAGS := -I/usr/local/include -L/usr/local/lib -Isrc -DHAVE_CXX_STDHEADERS -D_GNU_SOURCE -D_REENTRANT -O3 -std=c++20

LIBS := -llmdb -lsqlparser
TEST_LIBS := -lgtest -lgtest_main -pthread

SRCS := $(wildcard src/*.cpp)
TESTS := $(wildcard tests/*.cpp)

OBJS := $(SRCS:.cpp=.o)
TEST_OBJS := $(filter-out src/main.o, $(OBJS)) $(TESTS:.cpp=.o)

MAIN := out
TEST := test

all: $(MAIN)

$(MAIN): $(OBJS)
	g++ $(FLAGS) -o $(MAIN) $(OBJS) $(LIBS)

$(TEST): $(TEST_OBJS)
	g++ $(FLAGS) -o $(TEST) $(TEST_OBJS) $(LFLAGS) $(LIBS) $(TEST_LIBS)

%.o: %.cpp
	g++ $(FLAGS) -c $< -o $@

clean:
	$(RM) src/*.o tests/*.o data/example.mdb/*.mdb $(MAIN) $(TEST)

db-clean:
	$(RM) data/example.mdb/*.mdb

