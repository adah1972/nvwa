# Windows/Cygwin support
ifdef windir
    WINDOWS := 1
    CYGWIN  := 0
else
    ifdef WINDIR
        WINDOWS := 1
        CYGWIN  := 1
    else
        WINDOWS := 0
    endif
endif
ifeq ($(WINDOWS),1)
    EXEEXT := .exe
    DLLEXT := .dll
    DEVNUL := nul
    ifeq ($(CYGWIN),1)
        PATHSEP := /
    else
        PATHSEP := $(strip \ )
    endif
else
    EXEEXT :=
    DLLEXT := .so
    DEVNUL := /dev/null
    PATHSEP := /
endif

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<

%.dep: %.cpp
	$(CXX) -MM $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) $< > $@

LD  = $(CXX) $(CXXFLAGS) $(TARGET_ARCH)

INCLUDE  = -I..
CFLAGS   = -W -Wall
CXXFLAGS = -std=c++14 $(CFLAGS)
CPPFLAGS = -D_DEBUG -DBOOST_TEST_DYN_LINK $(INCLUDE)

OBJS     = $(CXXFILES:.cpp=.o)
DEPS     = $(patsubst %.o,%.dep,$(OBJS))

VPATH    = ../nvwa
CXXFILES = $(wildcard *.cpp) \
	   bool_array.cpp \
	   mem_pool_base.cpp

LIBS     = -lboost_unit_test_framework

TARGET   = boost_test$(EXEEXT)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(DEPS) $(OBJS)
	$(LD) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)

clean:
	$(RM) *.o *.dep $(TARGET)

-include $(wildcard *.dep)
