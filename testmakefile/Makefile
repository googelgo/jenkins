#!/bin/csh

# include the make.h of this module
include make.h


#程序编辑完成之后，需要编译链接。
#我们可以用gcc编译每个.c文件。如果有三个.c文件a.c、b.c、c.c，编译方法如下：
#gcc  -c a.c  –o  a.o                    //将三个.c文件编译成.o文件
#gcc  -c b.c  -o  b.o
#gcc  -c c.c  -o  c.o
#gcc  a.o  b.o  c.o  –o  all         //将三个.o文件编译成一个可执行文件

# -E: 预处理，主要是进行宏展开等步骤，生成的文件微test.i
# gcc -E test.c
# -S: 编译，生成汇编代码，生成的文件为test.S
# gcc -S test.c
# -c: 汇编：生成机器码，生成的文件未test.o
# gcc -c test.c
# (-o): 链接：生成可执行文件
# gcc test.c (-o test)

# Project/Module Object Files(s)
PROJ_OBJ_LIST	 = $(PROJ_OBJ_DIR)/main.${ObjSuffix}
PROJ_OBJ_LIST 	+= $(PROJ_OBJ_DIR)/utility.${ObjSuffix}
PROJ_OBJ_LIST 	+= $(PROJ_OBJ_DIR)/osixpth.${ObjSuffix}
PROJ_OBJ_LIST 	+= $(PROJ_OBJ_DIR)/srmmem.${ObjSuffix}
PROJ_OBJ_LIST 	+= $(PROJ_OBJ_DIR)/taska.${ObjSuffix}

PROJ_OBJ 	 	 = $(PROJ_NAME).${ObjSuffix}

# make
# 语法 Object: 依赖的所有object 或者 文件列表
$(PROJ_OBJ) : obj $(PROJ_OBJ_LIST)
	gcc ${PROJ_OBJ_LIST} -o $(PROJ_OBJ) -lpthread

obj :
	mkdir -p obj

# The rules required for the compilation of the individual Object files
# PROJ_INCS 是 include 文件目录
#$(PROJ_OBJ_DIR)/main.${ObjSuffix }: $(PROJ_SRC_DIR)/main.c $(PROJ_DEPENDS)
#	gcc -c $(PROJ_INCS) $(PROJ_SRC_DIR)/main.c -o $(PROJ_OBJ_DIR)/main.${ObjSuffix}	
$(PROJ_OBJ_DIR)/main.${ObjSuffix} : $(PROJ_SRC_DIR)/main.c ${PROJ_DEPENDS}
	gcc -c $(PROJ_INCS) -o $@ $<
	
#$(PROJ_OBJ_DIR)/utility.${ObjSuffix} : $(PROJ_SRC_DIR)/utility.c ${PROJ_DEPENDS}
#	gcc -c $(PROJ_INCS) $(PROJ_SRC_DIR)/utility.c -o $(PROJ_OBJ_DIR)/utility.${ObjSuffix}
$(PROJ_OBJ_DIR)/utility.${ObjSuffix} : $(PROJ_SRC_DIR)/utility.c ${PROJ_DEPENDS}
	gcc -c $(PROJ_INCS) -o $@ $<
	
$(PROJ_OBJ_DIR)/osixpth.${ObjSuffix} : $(PROJ_SRC_DIR)/osixpth.c ${PROJ_DEPENDS}
	gcc -c $(PROJ_INCS) -o $@ $<
	
$(PROJ_OBJ_DIR)/srmmem.${ObjSuffix} : $(PROJ_SRC_DIR)/srmmem.c ${PROJ_DEPENDS}
	gcc -c $(PROJ_INCS) -o $@ $<	
	
$(PROJ_OBJ_DIR)/taska.${ObjSuffix} : $(PROJ_SRC_DIR)/taska.c ${PROJ_DEPENDS}
	gcc -c $(PROJ_INCS) -o $@ $<

	
# clean .${ObjSuffix} files
clean :
	@echo "Cleaning module object files"
	rm -rf *.o
# for debug	
rc:
	echo PROJ_OBJ=${PROJ_OBJ}
