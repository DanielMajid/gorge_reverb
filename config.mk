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
		  $(realpath $(PROJECT_ROOT)/src/Dattorro.cc) \
		  $(realpath $(PROJECT_ROOT)/src/dsp/filters/OnePoleFilters.cc)

# List ASM source files here
UASMSRC = 

UASMXSRC = 

##############################################################################
# Include Paths
#

UINCDIR  = $(realpath $(PROJECT_ROOT)/src)

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

UDEFS = 

