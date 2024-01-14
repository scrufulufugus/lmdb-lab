CXXFLAGS = -I/usr/local/db6/include -DHAVE_CXX_STDHEADERS -D_GNU_SOURCE -D_REENTRANT -O3 -std=c++11
DEBUGFLAGS = -g

LFLAGS = -L/usr/local/db6/lib
LIBS = -ldb_cxx -lsqlparser

SRCS = $(wildcard src/*.cpp) # use Make's wildcard

OBJS = $(SRCS:.cpp=.o)
DEBUG_OBJS = $(SRCS:.cpp=.debug.o)

MAIN = sql5300
DEBUG = debug

all: $(MAIN)

debug: $(DEBUG)

$(MAIN): $(OBJS) 
	g++ $(CXXFLAGS) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

$(DEBUG): $(DEBUG_OBJS)
	g++ $(CXXFLAGS) $(DEBUGFLAGS) -o $(DEBUG) $(DEBUG_OBJS) $(LFLAGS) $(LIBS)

%.o: %.cpp
	g++ $(CXXFLAGS) -c $< -o $@

%.debug.o: %.cpp
	g++ $(CXXFLAGS) $(DEBUGFLAGS) -c $< -o $@

clean:
	$(RM) src/*.o $(MAIN) $(DEBUG)