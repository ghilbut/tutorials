# A sample Makefile for building Google Test and using it in user
# tests.  Please tweak it to suit your environment and project.  You
# may want to move it to your project's root directory.
#
# SYNOPSIS:
#
#   make [all]  - makes everything.
#   make TARGET - makes the given target.
#   make clean  - removes all files generated by make.

# Please tweak the following variable definitions as needed by your
# project, except GTEST_HEADERS, which you can use in your own targets
# but shouldn't modify.

# Points to the root of Google Test, relative to where this file is.
# Remember to tweak this if you move this file.
GTEST_DIR = ..

# Cross compile environment settings
TARGET = arm-unknown-linux-gnueabi
AR = $(TARGET)-ar
CC = $(TARGET)-gcc
CXX = $(TARGET)-g++

# Where to find user code.
USER_DIR = ../..
LGMD_HEADERS = $(USER_DIR)/lgmd_new
DRIVER_DIR = ../../../../../..
KADP_HEADERS = $(USER_DIR)/kadp
KAPI_HEADERS = $(USER_DIR)/kapi
# Modify PIONEER_TOPDIR according to user's pioneer dir
PIONEER_TOPDIR = /home/yk.hong/Projects/GTV_Media/gtvbox/trunk/pioneer
LGMD_LIBS = $(PIONEER_TOPDIR)/output/nfs/columbus/yk.hong/res/lglib

# Flags passed to the preprocessor.
CPPFLAGS += -I$(GTEST_DIR)/include
CPPFLAGS += -I$(LGMD_HEADERS)
CPPFLAGS += -I$(KADP_HEADERS)
CPPFLAGS += -I$(KAPI_HEADERS)
LDFLAGS+=-L$(LGMD_LIBS) -lkadaptor
LDFLAGS+=-L$(LGMD_LIBS)/lib -lreformatter


# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra

# All tests produced by this Makefile.  Remember to add new tests you
# created to the list.
TESTS = lgmd_adec_unittest

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# House-keeping build targets.

all : $(TESTS)

clean :
	rm -f $(TESTS) gtest.a gtest_main.a *.o

# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

# Builds a sample test.  A test should link with either gtest.a or
# gtest_main.a, depending on whether it defines its own main()
# function.

lgmd_adec_new.o : $(USER_DIR)/lgmd_adec_new.c
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/lgmd_adec_new.c

lgmd_adec_unittest.o : $(USER_DIR)/lgmd_adec_unittest.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/lgmd_adec_unittest.cc

lgmd_adec_unittest : lgmd_adec_new.o lgmd_adec_unittest.o gtest_main.a
	$(CXX) -o $@ -lpthread $(LDFLAGS) $^
