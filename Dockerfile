FROM srpatsu21/dear-glfw-vulkan-compiler:1.3.0

RUN git clone --depth 1 \
    --branch release-v3-c-cpp \
    https://github.com/NVIDIA/NVTX.git \
    /opt/NVTX