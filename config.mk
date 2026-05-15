##############################################################################
# Configuration for Makefile
#

PROJECT := Gorge
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

# Keep section GC enabled to drop unused code/data pulled by SDK and libm.
ULIBS  = -Wl,--gc-sections -lm

##############################################################################
# Macros
#

# Release-size profile:
# - keep debug info out of unit artifacts
# - allow linker garbage collection to be effective
# - strip unwind/frame metadata not needed for firmware runtime
UDEFS = -DNDEBUG \
	-g0 \
	-ffunction-sections \
	-fdata-sections \
	-fomit-frame-pointer \
	-fno-ident \
	-fno-unwind-tables \
	-fno-asynchronous-unwind-tables \
	-fno-math-errno

