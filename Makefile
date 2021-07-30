### MAIN SETUP ###################################################

-include ../../Rules.mk

# Name of the output app
TARGET_NAME = skanner3d_nard_server
ifdef ($(PKG_NAME))
	TARGET_NAME := $(PKG_NAME)
endif

# Names of used directories
BUILD_DIR   = build
SOURCE_DIR	= src
INCLUDE_DIR = include

# Additional includes for compilation
INCLUDES = 

INCLUDES += $(BOOSTDIR)

# Additional defines for compilation
DEFINES  = 

# Additional flags for compilation and linking
CXXFLAGS = -std=c++2a -Wall -Wpedantic
LDFLAGS  = 

ifeq ($(OS), Windows_NT)
	LDFLAGS  += -lws2_32 -lwsock32
else
	LDFLAGS  += -lboost_system -lboost_thread -lpthread -L/usr/lib/ -lstdc++fs
endif

# If 1, every cpp file inside SOURCE_DIR will be compiled
# IF 0, only the files specified by SOURCE_FILES will be compiled
COMPILE_ALL_SOURCES = 1
SOURCE_FILES = 

# If 1, the resulting binary will have the .exe extension
APPEND_EXE_ON_WINDOWS = 1

### MODES ########################################################

# List of all modes
MODES_LIST = debug release

# Default mode
MODE ?= debug

ifdef ($(PKG_NAME))
	MODE = nard
endif
# Modifiable procedures

ifeq ($(MODE), debug)
	DEFINES  += DEBUG
	CXXFLAGS += -g
endif
ifeq ($(MODE), nard)
	DEFINES  += RELEASE
	DEFINES  += NARD
	CXXFLAGS += -s -O2
	CXXFLAGS += $(CROSS_CFLAGS)
endif


# # You can specify additional commands for different targets
# .mode_debug_prebuild:
# 	@echo Compiling the $(MODE) mode with flags $(CXXFLAGS):
# .mode_release_prebuild:
# 	@echo Compiling the $(MODE) mode with flags $(CXXFLAGS):
# 	@echo Release mode is better
# .mode_debug_postbuild:
# 	@echo Debug compilation has ended



##################################################################
##################################################################

### OS specific settings and parameters ##########################

# Function used to convert filepath to correct format
P = $(1)
ifeq ($(OS), Windows_NT)
	P = $(subst /,\,$(1))
endif

### Command line functions
MKDIR = mkdir -p $(call P, $(1))
RMDIR = rm -rf $(call P, $(1))
CPDIR = cpdir $(call P, $(1)) $(call P, $(2))
CPDIR_TREE = 
IF_NOT_EXISTS = 
PIPE = |
ifeq ($(OS), Windows_NT)
	IF_NOT_EXISTS = if not exist $(1) $(2)
	IF_EXISTS = if exist $(1) $(2)
	MKDIR = $(call IF_NOT_EXISTS, $(call P, $(1)), mkdir $(call P, $(1)))
	RMDIR = $(call IF_EXISTS, $(call P, $(1)), rmdir /Q /S $(call P, $(1)))
	CPDIR = xcopy /I /T $(call P, $(1)) $(call P, $(2))
	CPDIR_TREE = xcopy /T /E /I $(call P, $(1)) $(call P, $(2))
	PIPE = &
endif


### Common Configuration #########################################

# Seting up the build directories
OBJECTS_DIR = $(BUILD_DIR)/objects/$(MODE)
DEPEND_DIR  = $(BUILD_DIR)/depend/$(MODE)
BIN_DIR     = $(BUILD_DIR)/bin/$(MODE)

# Setting the SOURCE_FILES if COMPILE_ALL_SOURCES is set

# Make does not offer a recursive wildcard function, so here's one:
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
ifeq ($(COMPILE_ALL_SOURCES), 1)
#	SOURCE_FILES = $(wildcard $(SOURCE_DIR)**/*.cpp)
	SOURCE_FILES = $(call rwildcard,$(SOURCE_DIR)/,*.cpp)
endif

# Generating all object files names for each source file
OBJECT_FILES = $(SOURCE_FILES:$(SOURCE_DIR)%.cpp=$(OBJECTS_DIR)%.o)

# Create the target file name
TARGET = $(BIN_DIR)/$(TARGET_NAME)
ifeq ($(APPEND_EXE_ON_WINDOWS), 1)
	ifeq ($(OS), Windows_NT)
		TARGET := $(TARGET).exe
	endif
endif


### Make targets #################################################

.PHONY: all setup build clean distclean install run $(MODES_LIST:%=.mode_%_prebuild) $(MODES_LIST:%=.mode_%_postbuild)

all: setup build

setup:
	@$(call CPDIR_TREE, $(SOURCE_DIR)/ $(OBJECTS_DIR)/)
	@$(call CPDIR_TREE, $(SOURCE_DIR)/ $(DEPEND_DIR)/)
	@$(call MKDIR, $(BIN_DIR))
	

clean: 
	@$(call RMDIR, $(BUILD_DIR))
	@$(call RMDIR, $(OBJECTS_DIR))
	@$(call RMDIR, $(DEPEND_DIR))
	
distclean:
	@$(call RMDIR, $(OBJECTS_DIR))
	@$(call RMDIR, $(DEPEND_DIR))

run:
	$(call P, $(TARGET))

INCLUDE_PARAMS = -I$(INCLUDE_DIR) $(INCLUDES:%=-I%)
DEFINE_PARAMS  = $(DEFINES:%=-D%)

ifdef ($(PKG_NAME))

COMPILER := "$(PATH_CROSS_CC)gcc"
LDFLAGS += -lstdc++
clean:
	$(std-clean)
distclean:
	$(std-distclean)
	
build: $(TARGET) install
install: $(TARGET)
	install -m 0755 -d "$(dir $@)"
	install -m 0755 $(PKG_NAME) "$@"
	touch "$@"

$(TARGET): Makefile

else

COMPILER := $(CXX)
build: $(TARGET)

endif


# Main compilation function
$(TARGET) : $(OBJECT_FILES)
	$(COMPILER) $(CXXFLAGS) $^ -o $(TARGET) $(LDFLAGS)

# Compilation of each translation unit
$(OBJECTS_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(COMPILER) $(CXXFLAGS) $(INCLUDE_PARAMS) $(DEFINE_PARAMS) -c $< -o $@  -MMD -MF $(DEPEND_DIR)/$(<:$(SOURCE_DIR)/%.cpp=%.d)



-include $(SOURCE_FILES:$(SOURCE_DIR)%.cpp=$(DEPEND_DIR)%.d)
