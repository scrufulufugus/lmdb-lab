CXXFLAGS = -I/usr/local/db6/include -Isrc -DHAVE_CXX_STDHEADERS -D_GNU_SOURCE -D_REENTRANT -O3 -std=c++11
DEBUGFLAGS = -g
TESTFLAGS = -I/usr/local/include

LFLAGS = -L/usr/local/db6/lib
LIBS = -ldb_cxx -lsqlparser
TESTLIBS = -lgtest -lgtest_main -pthread

SRCS = $(wildcard src/*.cpp) # use Make's wildcard
TESTS = $(wildcard tests/*.cpp)

OBJS = $(SRCS:.cpp=.o)
DEBUG_OBJS = $(SRCS:.cpp=.debug.o)
TEST_OBJS = $(filter-out src/main.o, $(OBJS)) $(TESTS:.cpp=.o)

MAIN = sql5300
DEBUG = debug
TEST = test

all: $(MAIN)

debug: $(DEBUG)

test: $(TEST)

$(MAIN): $(OBJS)
	g++ $(CXXFLAGS) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

$(DEBUG): $(DEBUG_OBJS)
	g++ $(CXXFLAGS) $(DEBUGFLAGS) -o $(DEBUG) $(DEBUG_OBJS) $(LFLAGS) $(LIBS)

$(TEST): $(TEST_OBJS)
	g++ $(CXXFLAGS) $(TESTFLAGS) -o $(TEST) $(TEST_OBJS) $(LFLAGS) $(LIBS) $(TESTLIBS)

%.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@

%.debug.o: %.cpp
	g++ $(CXXFLAGS) $(DEBUGFLAGS) -c $< -o $@

clean:
	$(RM) src/*.o tests/*.o $(MAIN) $(DEBUG) $(TEST)
