CXX = g++
CC  = gcc
LD  = g++
F90 = gfortran

ifeq ($(OMP), 1)
	OMPFLAGS  = -fopenmp
endif

ifeq ($(OMPCXX), 1)
	OMPFLAGS += -D_GLIBCXX_PARALLEL
endif

OFLAGS = -O3 -g -Wall
ifeq ($(GDB), 1)
	OFLAGS = -O0 -g -Wall
endif

CXXFLAGS =  -fPIC $(OFLAGS) -Wstrict-aliasing=2 $(OMPFLAGS)

ifeq ($(SSE), 1)
	CXXFLAGS += -D__mySSE__
endif



LDFLAGS = 

SRCPATH = ./
SRC = kdtree.cpp
OBJ = $(SRC:%.cpp=%.o) 

PROG  = kdtree

RM = /bin/rm

all:	  $(PROG) $(PROG1) $(PROG2)


$(PROG): $(OBJ) 
	$(LD) $(LDFLAGS) $^ -o $@ $(OMPFLAGS)

%.o: $(SRCPATH)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


clean:
	/bin/rm -rf *.o $(PROG) 

clean_all:
	/bin/rm -rf *.o $(PROG) plummer.dat *~

$(OBJ): kdtree.h plummer.h mytimer.h







