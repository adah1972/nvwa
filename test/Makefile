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
        DEVNUL := /dev/null
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
CFLAGS   = -g -Og -W -Wall -pthread
CXXFLAGS = -std=c++20 -march=native $(CFLAGS)
CPPFLAGS = -D_DEBUG -DBOOST_TEST_DYN_LINK $(INCLUDE)
VPATH    = ../nvwa

CXXFILES_BOOSTTEST = boosttest_MAIN.cpp \
                     $(wildcard *_test.cpp) \
                     bool_array.cpp \
                     file_line_reader.cpp \
                     mmap_reader_base.cpp \
                     mem_pool_base.cpp
OBJS_BOOSTTEST     = $(CXXFILES_BOOSTTEST:.cpp=.o)
DEPS_BOOSTTEST     = $(patsubst %.o,%.dep,$(OBJS_BOOSTTEST))
LIBS_BOOSTTEST     = -lboost_unit_test_framework
TARGET_BOOSTTEST   = boost_test$(EXEEXT)

CXXFILES_TESTCXXFEAT = test_c++_features.cpp
OBJS_TESTCXXFEAT     = $(CXXFILES_TESTCXXFEAT:.cpp=.o)
DEPS_TESTCXXFEAT     = $(patsubst %.o,%.dep,$(OBJS_TESTCXXFEAT))
LIBS_TESTCXXFEAT     =
TARGET_TESTCXXFEAT   = test_c++_features$(EXEEXT)

.PHONY: all clean

all: $(TARGET_BOOSTTEST) $(TARGET_TESTCXXFEAT)

$(TARGET_BOOSTTEST): $(DEPS_BOOSTTEST) $(OBJS_BOOSTTEST)
	$(LD) $(OBJS_BOOSTTEST) \
	      -o $(TARGET_BOOSTTEST) $(LDFLAGS) $(LIBS_BOOSTTEST)
$(TARGET_TESTCXXFEAT): $(DEPS_TESTCXXFEAT) $(OBJS_TESTCXXFEAT)
	$(LD) $(OBJS_TESTCXXFEAT) \
	      -o $(TARGET_TESTCXXFEAT) $(LDFLAGS) $(LIBS_TESTCXXFEAT)

clean:
	$(RM) *.o *.dep $(TARGET_BOOSTTEST) $(TARGET_TESTCXXFEAT)

-include $(wildcard *.dep)
