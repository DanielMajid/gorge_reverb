##############################################################################
# Configuration for Makefile
#

PROJECT := gorge
PROJECT_TYPE := revfx

##############################################################################
# Sources
#

# C sources 
UCSRC = header.c

# C++ sources 
UCXXSRC = unit.cc \
		  src/Dattorro.cc \
		  src/dsp/filters/OnePoleFilters.cc

# List ASM source files here
UASMSRC = 

UASMXSRC = 

##############################################################################
# Include Paths
#

UINCDIR  = $(PROJECT_ROOT)/src

##############################################################################
# Library Paths
#

ULIBDIR = 

##############################################################################
# Libraries
#

ULIBS  = -lm

##############################################################################
# Macros
#

UDEFS = -DNDEBUG

