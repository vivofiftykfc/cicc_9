OPENCV_CFLAGS := $(shell pkg-config --cflags opencv4)
OPENCV_LIBS   := $(shell pkg-config --libs   opencv4)

# 指定 C++ 编译器和选项
CXX       := g++
CXXFLAGS  := -std=c++11 -Wall -Iheaders $(OPENCV_CFLAGS)

# 源码与目标
SRCDIR    := .
FNDIR     := functions

# 列出所有的 .cpp 源文件
SOURCES   := $(SRCDIR)/main.cpp \
             $(FNDIR)/MLX90640_API.cpp \
             $(FNDIR)/MLX90640_I2C_Driver.cpp \
             $(FNDIR)/pseudo_color.cpp \

# 将 .cpp 后缀替换为 .o
OBJECTS   := $(patsubst %.cpp,%.o,$(SOURCES))

# 最终可执行文件名
TARGET    := mlx_app

.PHONY: all clean

# 默认目标：编译可执行文件
all: $(TARGET)

# 链接：把所有 .o 文件链接成可执行文件
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(OPENCV_LIBS)

# 编译规则：任何 .cpp -> .o
# 自动推导依赖头文件（简化版）
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理中间文件与可执行文件
clean:
	rm -f $(OBJECTS) $(TARGET)
