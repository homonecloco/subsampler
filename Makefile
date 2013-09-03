#MAC=1
all		: subsampler
#all	: cortex_con_cs solid_converter 
#all: solid_converter
ifndef CC
  CC = gcc	
endif

BIN = bin
LIB = lib

ifeq ($(MAXK),31)
   BITFIELDS = 1
endif

ifeq ($(MAXK),63)
   BITFIELDS = 2
endif

ifeq ($(MAXK),95)
   BITFIELDS = 3
endif

ifeq ($(MAXK),127)
   BITFIELDS = 4
endif

ifeq ($(MAXK),160)
   BITFIELDS = 5
endif

ifeq ($(MAXK),192)
   BITFIELDS = 6
endif

ifeq ($(MAXK),223)
   BITFIELDS = 7
endif

ifeq ($(MAXK),255)
   BITFIELDS = 8
endif

ifndef BITFIELDS
   BITFIELDS = 1
   MAXK = 31
endif 

# Main program includes
IDIR_BASIC =include/basic
IDIR_UTIL =include/util



UNAME := $(shell uname)


ifeq ($(UNAME), Darwin)
    MAC=1
endif

# Compiler options
OPT		= $(ARCH) -Wall -O3   -g

ifdef DEBUG
OPT	= $(ARCH) -Wall -O0 -g 
endif

ifdef ENABLE_READ_PAIR
 OPT +=  -DENABLE_READ_PAIR
endif

ifdef ENABLE_READ_PAIR_OLD
 OPT += -DENABLE_READ_PAIR_OLD
endif

ifdef READ_PAIR_DEBUG_GRAPH
 OPT += -DREAD_PAIR_DEBUG_GRAPH
endif

ifdef DEBUG_PRINT_LABELS
 OPT += -DDEBUG_PRINT_LABELS
endif


# Include dirs
CFLAGS_SUBSAMPLER_CON	= -I$(IDIR_BASIC)  -I$(IDIR_UTIL)

# Program objects
CORTEX_CON_OBJ = obj/cortex_con/file_format.o obj/cortex_con/flags.o obj/cortex_con/cleaning.o obj/cortex_con/path.o obj/cortex_con/perfect_path.o obj/cortex_con/branches.o obj/cortex_con/y_walk.o obj/cortex_con/cmd_line.o obj/cortex_con/binary_kmer.o obj/cortex_con/seq.o obj/cortex_con/element.o obj/cortex_con/hash_value.o obj/cortex_con/hash_table.o obj/cortex_con/dB_graph.o obj/cortex_con/file_reader.o obj/cortex_con/cortex_con.o obj/cortex_con/logger.o obj/cortex_con/metacortex.o obj/cortex_con/coverage_walk.o obj/util/node_queue.o



SUBSAMPLER_OBJ = obj/util/flags.o  obj/util/seq.o obj/util/logger.o obj/util/file_format.o obj/util/subsampler.o


# Main rules
subsampler: remove_objects $(SUBSAMPLER_OBJ)
		mkdir -p $(BIN); $(CC) -lm $(OPT) -o $(BIN)/subsampler $(SUBSAMPLER_OBJ)


# Cleaning rules
.PHONY : clean
clean :
	rm -rf $(BIN)/*
	rm -rf obj

remove_objects:
	rm -rf obj


obj/util/%.o : src/basic/%.c include/basic/%.h
	mkdir -p obj/util;  $(CC) $(CFLAGS_SUBSAMPLER_CON) $(OPT) -c $< -o $@

# Utils
obj/util/subsampler.o : src/util/subsampler.c 
		mkdir -p obj/util;  $(CC) $(CFLAGS_SUBSAMPLER_CON) $(OPT) -c $? -o $@


