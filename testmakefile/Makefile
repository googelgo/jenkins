#!/bin/csh

# include the make.h of this module
include make.h


#����༭���֮����Ҫ�������ӡ�
#���ǿ�����gcc����ÿ��.c�ļ������������.c�ļ�a.c��b.c��c.c�����뷽�����£�
#gcc  -c a.c  �Co  a.o                    //������.c�ļ������.o�ļ�
#gcc  -c b.c  -o  b.o
#gcc  -c c.c  -o  c.o
#gcc  a.o  b.o  c.o  �Co  all         //������.o�ļ������һ����ִ���ļ�

# -E: Ԥ��������Ҫ�ǽ��к�չ���Ȳ��裬���ɵ��ļ�΢test.i
# gcc -E test.c
# -S: ���룬���ɻ����룬���ɵ��ļ�Ϊtest.S
# gcc -S test.c
# -c: ��ࣺ���ɻ����룬���ɵ��ļ�δtest.o
# gcc -c test.c
# (-o): ���ӣ����ɿ�ִ���ļ�
# gcc test.c (-o test)

# Project/Module Object Files(s)
PROJ_OBJ_LIST	 = $(PROJ_OBJ_DIR)/main.${ObjSuffix}
PROJ_OBJ_LIST 	+= $(PROJ_OBJ_DIR)/utility.${ObjSuffix}
PROJ_OBJ_LIST 	+= $(PROJ_OBJ_DIR)/osixpth.${ObjSuffix}
PROJ_OBJ_LIST 	+= $(PROJ_OBJ_DIR)/srmmem.${ObjSuffix}
PROJ_OBJ_LIST 	+= $(PROJ_OBJ_DIR)/taska.${ObjSuffix}

PROJ_OBJ 	 	 = $(PROJ_NAME).${ObjSuffix}

# make
# �﷨ Object: ����������object ���� �ļ��б�
$(PROJ_OBJ) : obj $(PROJ_OBJ_LIST)
	gcc ${PROJ_OBJ_LIST} -o $(PROJ_OBJ) -lpthread

obj :
	mkdir -p obj

# The rules required for the compilation of the individual Object files
# PROJ_INCS �� include �ļ�Ŀ¼
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