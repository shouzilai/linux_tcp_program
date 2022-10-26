CC = gcc
TARGET = $(notdir $(CURDIR))_server
# TARGET = $(notdir $(CURDIR))_client # 编译客户端时，记得修改CC为交叉编译的工具链
SRC =  $(shell ls *.c )
SRC +=  $(shell ls */*.c )
SRC += $(shell ls */*/*.c)
OBJS = $(patsubst %.c ,%.o ,$(SRC))  # 查找SRC中是否有 .c 和 .o 文件，有的话将.c 换成 .o，参与以下的语句
.PHONY: all
all: $(TARGET)
$(TARGET) : $(OBJS)     # gcc -o fifo_test *.o
	$(CC) -DINET_TCP_SERVER=1 -o $@ $^ -lpthread -lrt
%.o : %.c               # 两个% 匹配的是同一个字符串   $<:第一个依赖文件的名称  $^:所有的依赖文件  $@:目标完整的名称 
	$(CC) -c $< -DINET_TCP_SERVER=1 -o  $@ -lpthread -lrt
clean:                  # gcc -c tty.c -o fifo_test
	$(RM) *.o $(TARGET)
