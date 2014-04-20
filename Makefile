# Makefile for the cs244b mazewar assignment
# ------------------------------------------
#
# Inspired by:
#   Ben Werther <benw@cs.stanford.edu> Feb 1999
# Modified by:
#   Constantine Sapuntzakis <csapuntz@cs.stanford.edu> Apr 1999
# Modified by:
#   Timothy Knight <tjk@cs.stanford.edu> Mar 2004
#
#
# To build mazewar, use the command "make"; the executable file 
# "mazewar" will be created.
# To clean the directory, use the command "make clean".
#
# This makefile has been tested on epic, elaine, and saga machines using 
# GNU Make 3.80, which is at the following path on the Leland machines:
#
#    /usr/pubsw/bin/make
#
# A makefile must be submitted with your assignment. It may be based
# on this makefile, or may be re-written by you, but must work with
# the above version of make, and shouldn't assume any environment
# variables to be set to particular values.
#
# Feel free to use other platforms, including Linux, but remember that 
# you'll be evaluated during the demo on the Sweet Hall Leland 
# machines, so it's recommended that you do your development and 
# testing on those machines.
#

CC         = /usr/bin/g++
CXX        = /usr/bin/g++
RM         = /bin/rm
SED        = /bin/sed
SH         = /bin/sh

INC_DIRS   = -I. -I/usr/openwin/include
FLAGS      = -g -Wall $(INC_DIRS)
LIBRARIES  = -lXt -lX11  -lnsl -lICE -lSM
LIB_DIRS   = -L. -L/usr/X11R6/lib fwk/BaseCollection.o fwk/BaseNotifiee.o fwk/Exception.o
#Removed -lsocket
CFLAGS     = $(FLAGS) 
CXXFLAGS   = $(FLAGS)




all: mwar
# dummy target points to the real target
dummytarget: mwar

############################
#
# Applications
#

APP_OBJS   = toplevel.o display.o init.o winsys.o
APP = mazewar

$(APP): $(APP_OBJS)
	$(CC) -o $(APP) $(APP_OBJS) $(LIB_DIRS) $(LIBRARIES)



############################
#
# Makefile stuff
# (you probably won't need to change any of this)
#

.PHONY: dummytarget 

mwar:	framework deps 
	@echo "Building Mazewar..."
	@$(MAKE) INCLUDE_DEPS=1 mazewar

clean:
	@echo "Cleaning Mazewar directory..."
	$(MAKE) -C fwk/ clean
	$(RM) -f *.o *.d *~ mazewar

framework:
	$(MAKE) -C fwk/

# Dependency creation 
deps: ${APP_OBJS:.o=.d}
	@echo "Updating Mazewar dependencies..."

# Include the dependencies, once we've built them
ifdef INCLUDE_DEPS
include ${APP_OBJS:.o=.d}
endif


#################################################################
#
# Dependency stuff
#

%.cc: %.cpp
	cp $< $@

%.d: %.cc
	@$(SH) -ec '$(CXX) -MM $(CXXFLAGS) $< \
                    | $(SED) '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
                    [ -s $@ ] || $(RM) -f $@'

%.d: %.cpp
	@$(SH) -ec '$(CXX) -MM $(CXXFLAGS) $< \
                    | $(SED) '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
                    [ -s $@ ] || $(RM) -f $@'

%.d: %.c
	@$(SH) -ec '$(CC) -MM $(CFLAGS) $< \
                    | $(SED) '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
                    [ -s $@ ] || $(RM) -f $@'
