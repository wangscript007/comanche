#pragma once

#include <vector>
#include <numeric>
#include <memory>

#include <iostream>
#include <fstream>

#include <curand.h>
#include <cuda.h>
#include <thrust/device_ptr.h>
#include <thrust/fill.h>

/* Comanche Common */
#include <common/dump_utils.h>
#include <common/utils.h>
#include <api/components.h>
#include <api/kvstore_itf.h>

using pool_t = uint64_t;

template <typename T>
class Tensor {
    std::vector<int> dims_;
    int size_;

    struct deleteCudaPtr {
        void operator()(T *p) const {
            cudaFree(p);
        }
    };

    std::shared_ptr<T> ptr_;

public:

    Tensor() {}

    Tensor(std::vector<int> dims) : dims_(dims) {
        T* tmp_ptr;
        size_ = std::accumulate(dims_.begin(), dims_.end(), 1, std::multiplies<int>());
        cudaMalloc(&tmp_ptr, sizeof(T) * size_ + MB(4));

        ptr_.reset(tmp_ptr, deleteCudaPtr());
    }

    T* begin() const { return ptr_.get(); }
    T* end()   const { return ptr_.get() + size_; }
    int size() const { return size_; }
    std::vector<int> dims() const { return dims_; }
};

template <typename T>
Tensor<T> fill(std::vector<int> dims, float val) {
     Tensor<T> tensor(dims);
     thrust::fill(thrust::device_ptr<T>(tensor.begin()),
                  thrust::device_ptr<T>(tensor.end()), val);
     return tensor;
}

template <typename T>
Tensor<T> zeros(std::vector<int> dims) {
    Tensor<T> tensor(dims);
    thrust::fill(thrust::device_ptr<T>(tensor.begin()),
                 thrust::device_ptr<T>(tensor.end()), 0.f);
    return tensor;
}

template <typename T>
typename std::enable_if<(std::is_same<T, float>::value), Tensor<T>>::type
rand(std::vector<int> dims, curandGenerator_t curand_gen) {
    Tensor<T> tensor(dims);
    curandGenerateUniform(curand_gen, tensor.begin(), tensor.size());
    return tensor;
}

template <typename T>
typename std::enable_if<!(std::is_same<T, float>::value), Tensor<T>>::type
rand(std::vector<int> dims, curandGenerator_t curand_gen) {

    Tensor<T> tensor(dims);
    Tensor<float> tensor_f(dims);
    curandGenerateUniform(curand_gen, tensor_f.begin(), tensor_f.size());

    thrust::copy(thrust::device_ptr<float>(tensor_f.begin()),
                 thrust::device_ptr<float>(tensor_f.end()),
                 thrust::device_ptr<T>(tensor.begin()));

    return tensor;
}

void pad_dim(int & dim, int pad_v) {
    assert(pad_v > 0);
    if (dim % pad_v) {
        int pad = pad_v - dim%pad_v;
        dim += pad;
    }
}

template <typename T>
void write_to_file(std::string file_name, Tensor<T> tensor) {
	std::ofstream out_file;
	std::cout << "Input Size: " << tensor.size() << std::endl;
	out_file.open(file_name, std::ios::out | std::ios::binary);
	void * h_tmp = (void*) malloc(sizeof(T)*tensor.size());
	assert(h_tmp);
	CU_CHECK(cuMemcpyDtoH(h_tmp, (CUdeviceptr) tensor.begin(), tensor.size()));
	hexdump(h_tmp, 32);
	out_file.write((char*)h_tmp, tensor.size());
	out_file.close();
}

template <typename T>
void read_from_file(std::string file_name, Tensor<T> tensor) {
	std::ifstream in_file;
	in_file.open(file_name, std::ios::in | std::ios::binary);
	void * h_tmp = (void*) malloc(sizeof(T)*tensor.size());
	assert(h_tmp);
	in_file.read((char*)h_tmp, tensor.size());
	CU_CHECK(cuMemcpyHtoD((CUdeviceptr) tensor.begin(), h_tmp, tensor.size()));
	in_file.close();
	free(h_tmp);
}

// put_direct version
template <typename T>
void put_into_kvstore(std::string key, Tensor<T> tensor, Component::IKVStore * store, pool_t pool) {
	assert(store);
	std::cout << "Input Size: " << tensor.size() << std::endl;
	void * h_tmp = (void*) malloc(sizeof(T)*tensor.size());
	assert(h_tmp);
	CU_CHECK(cuMemcpyDtoH(h_tmp, (CUdeviceptr) tensor.begin(), tensor.size()));
	hexdump(h_tmp, 32);
	Component::IKVStore::memory_handle_t handle = store->register_direct_memory(h_tmp, tensor.size());
	status_t rc = store->put_direct(pool, key, h_tmp, tensor.size(), handle);
	assert(rc == S_OK);
	store->unregister_direct_memory(handle);
	free(h_tmp);
}

template <typename T>
void hexdump_tensor(Tensor<T> tensor) {
	void * h_tmp = (void*) malloc(sizeof(T)*tensor.size());
	assert(h_tmp);
	CU_CHECK(cuMemcpyDtoH(h_tmp, (CUdeviceptr) tensor.begin(), tensor.size()));
	hexdump(h_tmp, 32);
	free(h_tmp);
}
