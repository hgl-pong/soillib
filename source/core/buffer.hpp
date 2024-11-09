#ifndef SOILLIB_BUFFER
#define SOILLIB_BUFFER

//! A buffer represents a raw data extent
//! \todo add more detail about this file

#include <soillib/soillib.hpp>
#include <soillib/core/types.hpp>
#include <soillib/util/error.hpp>

namespace soil {

//! \todo Make sure that buffers are "re-interpretable"!

enum host_t {
  CPU,
  GPU
};

//! buffer_t<T> is a strict-typed, raw-data extent.
//!
//! buffer_t<T> contains a shared pointer to the
//! underlying data, meaning that copies of the buffer
//! can be made without copying the raw memory.
//!
template<typename T>
struct buffer_t: typedbase {

  buffer_t(){
    this->_data = NULL;
    this->_refs = new size_t(0);
    this->_size = 0;
    this->_host = CPU;
  }

  // Specialized Constructors

  buffer_t(const size_t size, const host_t host = CPU) {
    this->allocate(size, host);
  }

  ~buffer_t() override {
    this->deallocate();
  }

  // Copy Semantics

  buffer_t(const buffer_t<T>& other){
    this->_data = other._data;
    this->_refs = other._refs;
    this->_size = other._size;
    this->_host = other._host;
    if(this->_data != NULL){
      ++(*this->_refs);
    }
	}

	buffer_t& operator=(const buffer_t<T>& other){
    this->deallocate();
    this->_data = other._data;
    this->_refs = other._refs;
    this->_size = other._size;
    this->_host = other._host;
    if(this->_data != NULL){
      ++(*this->_refs);
    }
    return *this;
	}

  // Move Semantics

  buffer_t(buffer_t<T>&& other){
    this->_data = other._data;
    this->_refs = other._refs;
    this->_size = other._size;
    this->_host = other._host;
    other._data = NULL;
    other._refs = NULL;
    other._size = 0;
  }

	buffer_t& operator=(buffer_t<T>&& other){
    this->deallocate();
    this->_data = other._data;
    this->_refs = other._refs;
    this->_size = other._size;
    this->_host = other._host;
    other._data = NULL;
    other._refs = NULL;
    other._size = 0;
    return *this;
	}

  // GPU Uploading Procedure?
  // GPU Uploading Procedure:
  // If we are already on the GPU: continue.
  // If we are not already on the GPU:
  // directly upload this guy... note that we can copy construct.

  void to_gpu();
  void to_cpu();

  //! Type Enumerator Retrieval
  constexpr soil::dtype type() noexcept override {
    return soil::typedesc<T>::type;
  }

  GPU_ENABLE inline size_t elem() const { return this->_size; }              //!< Number of Elements
  GPU_ENABLE inline size_t size() const { return this->elem() * sizeof(T); } //!< Total Size in Bytes
  GPU_ENABLE inline void *data() { return (void *)this->_data; }               //!< Raw Data Pointer

  GPU_ENABLE inline size_t refs() const { return *this->_refs; }
  GPU_ENABLE inline host_t host() const { return this->_host; }

  //! Const Subscript Operator
  GPU_ENABLE T operator[](const size_t index) const noexcept {
    return this->_data[index];
  }

  //! Non-Const Subscript Operator
  GPU_ENABLE T &operator[](const size_t index) noexcept {
    return this->_data[index];
  }

private:

  void allocate(const size_t size, const host_t host = CPU);
  void deallocate();

  T* _data = NULL;          //!< Raw Data Pointer (Device Agnostic)
  size_t _size = 0;         //!< Number of Data Elements
  host_t _host = CPU;       //!< 
  size_t* _refs = NULL; 
};

//
//
//

//! buffer is a poylymorphic buffer_t wrapper type.
//!
//! A buffer holds a single strict-typed buffer_t through
//! a shared pointer, making copy and move semantics for
//! the underlying buffer work as expected.
//!
struct buffer {

  buffer() = default;
  //buffer(const buffer& buffer):impl{buffer.impl}{}

  buffer(const soil::dtype type, const size_t size): impl{make(type, size)} {}

  //! Note that since it holds a shared pointer to a buffer_t,
  //! holding a shared pointer, if the copied or moved object
  //! is destroyed, the underlying raw memory is not deleted.

  template<typename T>
  buffer(const soil::buffer_t<T> &buf) {
    impl = std::make_shared<soil::buffer_t<T>>(buf);
  }

  template<typename T>
  buffer(soil::buffer_t<T> &&buf) {
    impl = std::make_shared<soil::buffer_t<T>>(buf);
  }

  ~buffer() { this->impl = NULL; }

  //! retrieve the strict-typed type enumerator
  inline soil::dtype type() const noexcept {
    return this->impl->type();
  }

  //! unsafe cast to strict-type
  template<typename T>
  inline buffer_t<T> &as() noexcept {
    return static_cast<buffer_t<T> &>(*(this->impl));
  }

  template<typename T>
  inline const buffer_t<T> &as() const noexcept {
    return static_cast<buffer_t<T> &>(*(this->impl));
  }

  //! Const Subscript Operator
  template<typename T>
  T operator[](const size_t index) const {
    return select(this->type(), [self = this, index]<typename S>() -> T {
      if constexpr (std::same_as<S, T>) {
        return self->as<T>().operator[](index);
      } else if constexpr (std::convertible_to<S, T>) {
        return (T)self->as<S>().operator[](index);
      } else {
        throw soil::error::cast_error<S, T>();
      }
    });
  }

  //! Non-Const Subscript Operator
  template<typename T>
  T &operator[](const size_t index) {
    return this->as<T>()[index];
//    return select(this->type(), [self = this, index]<typename S>() -> T& {
//      if constexpr (std::same_as<S, T>) {
//        return this->as<T>()[index];
//      } else {
//        throw soil::error::cast_error<S, T>();
//      }
//    });
  }

  // Data Inspection Operations (Type-Deducing)

  size_t elem() const {
    return select(this->type(), [self = this]<typename S>() {
      return self->as<S>().elem();
    });
  }

  size_t size() const {
    return select(this->type(), [self = this]<typename S>() {
      return self->as<S>().size();
    });
  }

  void *data() {
    return select(this->type(), [self = this]<typename S>() {
      return self->as<S>().data();
    });
  }

  host_t host() const {
    return select(this->type(), [self = this]<typename S>() {
      return self->as<S>().host();
    });
  }

private:
  using ptr_t = std::shared_ptr<typedbase>;
  ptr_t impl; //!< Strict-Typed Implementation Base Pointer

  static ptr_t make(const soil::dtype type, const size_t size) {
    return select(type, [size]<typename S>() -> ptr_t {
      return std::make_shared<soil::buffer_t<S>>(size);
    });
  }
};

} // end of namespace soil

#endif