#!/bin/csh

# Set current directory to PROJ_DIR
ObjSuffix       = o
PROJ_NAME		= test
PROJ_DIR	    = .
PROJ_SRC_DIR	= ./src
PROJ_INC_DIR	= ./inc
PROJ_OBJ_DIR	= ./obj
#project include files directory 
PROJ_INCS       = -I$(PROJ_INC_DIR)

PROJ_H_FLIES    =  	$(PROJ_INC_DIR)/proto.h \
					$(PROJ_INC_DIR)/macro.h \
					$(PROJ_INC_DIR)/utlmacro.h \
					$(PROJ_INC_DIR)/osix.h \
					$(PROJ_INC_DIR)/osxinc.h \
					$(PROJ_INC_DIR)/srmmem.h \
					$(PROJ_INC_DIR)/srmmemi.h \
					$(PROJ_INC_DIR)/osxstd.h \
					

PROJ_DEPENDS 	=   $(PROJ_DIR)/make.h \
					$(PROJ_DIR)/Makefile \
					$(PROJ_H_FLIES)
