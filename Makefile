###########目标文件的名称##########
RELEASE_DIR:=release
OBJ_DIR :=.objects/

EXEC_FILE:=$(RELEASE_DIR)/uart
######指定源文件所在目录
SRC_DIR := ./
 

######指定头文件所在目录
INCLUDE_DIR := $(SRC_DIR)


#############编译平台##########
#COMPILER_PREFIX:=arm-arago-linux-gnueabi-
CC  := $(COMPILER_PREFIX)gcc
CXX := $(COMPILER_PREFIX)g++
AR  := $(COMPILER_PREFIX)ar
LD  := $(COMPILER_PREFIX)ld

#####指定所依赖的静态库文件
#STATIC_LIBS := ./staticLib/libstatic.a

#####指定所依赖的动态库文件如：libtest.so, -L后面带依赖库的路径
#LDFLAGS+= -L./dynamicLib -ldynamic 

#####指定编译选项
USERCPPFLAGS := -ffunction-sections -fdata-sections
#USERCPPFLAGS += -pipe -std=c++0x -O1 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp -Wall -W -D_REENTRANT
VPATH := $(SRC_DIR)

CSrcs := $(foreach CurDir,$(SRC_DIR),$(wildcard $(CurDir)/*.c))
CppSrcs := $(foreach CurDir,$(SRC_DIR),$(wildcard $(CurDir)/*.cpp))

Objects := $(addprefix $(OBJ_DIR), $(addsuffix .o,$(notdir $(basename $(CSrcs) $(CppSrcs)))))

CDeps:= $(addsuffix .cdep, $(addprefix $(OBJ_DIR), $(basename $(notdir $(CSrcs)))))
CppDeps := $(addsuffix .cppdep, $(addprefix $(OBJ_DIR), $(basename $(notdir $(CppSrcs)))))

CPPFLAGS := -Werror -W $(USERCPPFLAGS)
CPPFLAGS += $(addprefix -I,$(INCLUDE_DIR))
LDFLAGS +=  -lpthread -ldl -lrt -lm 

$(EXEC_FILE): $(Objects) $(STATIC_LIBS)
	$(CXX) $^ -o $@ $(LDFLAGS)
	@#$(COMPILER_PREFIX)strip $(EXEC_FILE)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),cleandep)
-include $(CDeps)
-include $(CppDeps)
endif
endif

$(OBJ_DIR)%.cdep: %.c
	$(create_dep_file)

$(OBJ_DIR)%.cppdep: %.cpp
	$(create_dep_file)

$(OBJ_DIR)%.o:%.c
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(OBJ_DIR)%.o:%.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

define create_dep_file
	@if [ ! -e "$(RELEASE_DIR)" ];then mkdir "$(RELEASE_DIR)";fi
	@if [ ! -e "$(OBJ_DIR)" ];then mkdir "$(OBJ_DIR)"; fi
	@echo Updating dependance ====\> $@;\
	rm -f $@; \
	$(CC) $(CPPFLAGS) -MM $< | sed 's,$(*F).o:,$(OBJ_DIR)$*.o $@:,g' > $@;
endef

.PHONY: clean
clean:
	-rm $(OBJ_DIR) $(RELEASE_DIR)/* -rf

