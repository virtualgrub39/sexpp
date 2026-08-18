SEXPP_DIR 	:= $(dir $lastword $(MAKEFILE_LIST))
CFLAGS 		+= -I$(SEXPP_DIR)include
SOURCES 	+= $(SEXPP_DIR)source/sexpp.c
