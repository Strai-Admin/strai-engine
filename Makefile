ifneq ("$(wildcard /usr/bin/nvcc)", "")
CUDAPATH ?= /usr
else ifneq ("$(wildcard /usr/local/cuda/bin/nvcc)", "")
CUDAPATH ?= /usr/local/cuda
endif

IS_JETSON   ?= $(shell if grep -Fwq "Jetson" /proc/device-tree/model 2>/dev/null; then echo true; else echo false; fi)
NVCC        :=  ${CUDAPATH}/bin/nvcc
CCPATH      ?=

IS_PRODUCTION ?= 0
BUILD_DIR ?= build
MARKER := $(BUILD_DIR)/.marker

override CFLAGS   ?=
override CFLAGS   += -O3
override CFLAGS   += -Wno-unused-result
override CFLAGS   += -I${CUDAPATH}/include
override CFLAGS   += -std=c++11
override CFLAGS   += -DIS_JETSON=${IS_JETSON}
override CFLAGS   += -DIS_PRODUCTION=${IS_PRODUCTION}

override LDFLAGS  ?=
override LDFLAGS  += -lcuda
override LDFLAGS  += -L${CUDAPATH}/lib64
override LDFLAGS  += -L${CUDAPATH}/lib64/stubs
override LDFLAGS  += -L${CUDAPATH}/lib
override LDFLAGS  += -L${CUDAPATH}/lib/stubs
override LDFLAGS  += -Wl,-rpath=${CUDAPATH}/lib64
override LDFLAGS  += -Wl,-rpath=${CUDAPATH}/lib
override LDFLAGS  += -lcublas_static
override LDFLAGS  += -lcudart_static
override LDFLAGS  += -lpthread -ldl -lrt


COMPUTE      ?= 75
CUDA_VERSION ?= 11.8.0
IMAGE_DISTRO ?= ubi8

override NVCCFLAGS ?=
override NVCCFLAGS += -I${CUDAPATH}/include
override NVCCFLAGS += -arch=compute_$(subst .,,${COMPUTE})
override NVCCFLAGS += -DIS_PRODUCTION=${IS_PRODUCTION}

IMAGE_NAME ?= cudabench

all: $(BUILD_DIR)/cudabench.gz

$(MARKER):
	mkdir $(BUILD_DIR) -p
	touch $@

.PHONY: clean dockerized all

$(BUILD_DIR)/cudabench: $(BUILD_DIR)/kernel.o
	g++ -o $@ $< -O3 ${LDFLAGS}

$(BUILD_DIR)/cudabench-tagged: $(BUILD_DIR)/cudabench
	cp $< $@
	echo "\n\ngit sha: $(shell git rev-parse HEAD)" >> $@

$(BUILD_DIR)/cudabench.gz: $(BUILD_DIR)/cudabench-tagged
	gzip -9 $^ -c > $@

$(BUILD_DIR)/%.o: %.cpp $(MARKER)
	g++ ${CFLAGS} -c $<

#%.ptx: %.cu
#	PATH="${PATH}:${CCPATH}:." ${NVCC} ${NVCCFLAGS} -ptx $< -o $@

$(BUILD_DIR)/%.o: %.cu *.h $(MARKER)
	PATH="${PATH}:${CCPATH}:." ${NVCC} ${NVCCFLAGS} -c $< -o $@

clean:
	$(RM) $(BUILD_DIR)/*.ptx $(BUILD_DIR)/*.o $(BUILD_DIR)/cudabench

#image:
#	docker build --build-arg CUDA_VERSION=${CUDA_VERSION} --build-arg IMAGE_DISTRO=${IMAGE_DISTRO} -t ${IMAGE_NAME} .

dockerized: Dockerfile
	docker build . -t cuda-build:11.8
	docker run --volume $(CURDIR):/benchmark --user $(shell id -u) cuda-build:11.8 make BUILD_DIR=build-docker COMPUTE=61 IS_PRODUCTION=1 -C /benchmark clean all
	docker run -it --volume $(CURDIR):/benchmark --gpus all --entrypoint /bin/bash brunneis/python:3.9.0-ubuntu -c '/benchmark/build-docker/cudabench 600'
