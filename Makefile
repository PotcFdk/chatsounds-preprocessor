program_NAME := chatsounds-preprocessor
program_CXX_SRCS := main.cpp
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_INCLUDE_DIRS := includes
program_LIBRARY_DIRS := lib
program_LIBRARIES := boost_system boost_filesystem boost_serialization bass

CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))

CXXFLAGS = -std=c++11 -s -O2 -Wl,-rpath=.,-rpath=lib

.PHONY: all clean distclean

all: $(program_NAME)

chatsounds-preprocessor: $(program_CXX_OBJS)
	$(LINK.cc) $(program_CXX_OBJS) -o $(program_NAME)

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(program_CXX_OBJS)

distclean: clean
