FROM ubuntu:22.04



# 设置时区
ENV TZ="Asia/Shanghai"
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# 构建环境
RUN sed -i s@/archive.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list
RUN apt update
RUN apt install -y build-essential cmake libssl-dev git gdb libglfw3-dev zenity fonts-droid-fallback

# 安装Catch2
RUN git clone https://github.com/catchorg/Catch2.git --depth=1 && \
    cd Catch2 && \
    cmake -B build -S . -DBUILD_TESTING=OFF && \
    cmake --build build/ --target install && \
    cd .. && \
    rm -rf Catch2

# 更新动态库配置
RUN ldconfig

ENV MYPATH /root/backupmanager
RUN mkdir $MYPATH
COPY . $MYPATH
WORKDIR $MYPATH

# 编译安装
RUN rm -rf build && mkdir build
RUN cd build && cmake .. && make && make install