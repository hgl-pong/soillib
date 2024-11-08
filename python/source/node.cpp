#ifndef SOILLIB_PYTHON_LAYER
#define SOILLIB_PYTHON_LAYER

#include <nanobind/nanobind.h>
namespace nb = nanobind;

#include <nanobind/ndarray.h>

#include <nanobind/stl/string.h>
#include <nanobind/stl/function.h>

#include <soillib/core/types.hpp>

#include <soillib/core/node.hpp>

#include <soillib/node/cached.hpp>
#include <soillib/node/constant.hpp>
#include <soillib/node/computed.hpp>

#include <soillib/node/algorithm/noise.hpp>
#include <soillib/node/algorithm/normal.hpp>
#include <soillib/node/algorithm/flow.hpp>

#include <iostream>

#include "glm.hpp"

//! General Node Binding Function
void bind_node(nb::module_& module){

  //
  // Layer Wrapper Type
  //

  auto node = nb::class_<soil::node>(module, "node");
  node.def(nb::init<const soil::buffer>());
  node.def(nb::init<soil::cached&&>());
  node.def(nb::init<soil::constant&&>());
  node.def(nb::init<soil::computed&&>());

  node.def_prop_ro("type", &soil::node::type);
  node.def_prop_ro("buffer", [](soil::node& node){
    return node.as<soil::cached>().buffer;
  }, nb::rv_policy::reference_internal);








  // node.def("bake", &soil::node::bake);

  node.def("__setitem__", [](soil::node& node, const nb::slice& slice, const nb::object value){

    auto buffer = node.as<soil::cached>().buffer;
    soil::select(buffer.type(), [&buffer, &slice, &value]<typename S>(){

      auto buffer_t = buffer.as<S>();           // Assignable Strict-Type Buffer
      const auto value_t = nb::cast<S>(value);  // Assignable Value

      // Read Slice:
      Py_ssize_t start, stop, step;
      if(PySlice_GetIndices(slice.ptr(), buffer_t.elem(), &start, &stop, &step) != 0)
        throw std::runtime_error("slice is invalid!");
      
      // Assign Values!
      for(int index = start; index < stop; index += step)
        buffer_t[index] = value_t;
    
    });
  });

  /*
  node.def("__call__", [](soil::node& node, const size_t index){
    return soil::select(node.dnode(), [&node, index]<typename S>() -> nb::object {
      auto node_t = node.as<S>();
      return soil::select(node_t.type(), [&node_t, index]<typename T>() -> nb::object {
        T value = node_t.template as<T>()(index);
        return nb::cast<T>(std::move(value));
      });
    });
  });
  */

//  node.def("__getitem__", [](soil::node& ))

  node.def("__mul__", [](soil::node node, const nb::object object){
    return soil::select(node.type(), [node, object]<typename T>() -> soil::node {
      T value = nb::cast<T>(object);
      std::function<T(const size_t)> func = [node, value](const size_t index) -> T {
        return value * select(node.dnode(), [node, index]<typename S>(){
          auto node_t = node.template as<S>();
          return node_t.template operator()<T>(index);
        });
      };
      return soil::node(std::move(soil::computed(node.type(), func)));
    });
  });

  node.def("numpy", [](soil::node& node, soil::index& index){
    
    return soil::select(index.type(), [&]<typename I>() -> nb::object {

      auto index_t = index.as<I>();                 // Cast Index to Strict-Type
      soil::flat_t<I::n_dims> flat(index_t.ext());  // Hypothetical Flat Buffer

      //! \todo Remove this requirement, not actually necessary.
      auto buffer = node.as<soil::cached>().buffer;

      return soil::select(buffer.type(), [&]<typename T>() -> nb::object {

        if constexpr(nb::detail::is_ndarray_scalar_v<T>){

          soil::buffer_t<T> source = buffer.as<T>();  // Source Buffer w. Index

          // Typed Buffer of Flat Size
          //! \todo make sure this is de-allocated correctly,
          //! i.e. the numpy buffer should perform a copy.
          soil::buffer_t<T>* target  = new soil::buffer_t<T>(flat.elem()); 

          // Fill w. NaN Value
          T value = std::numeric_limits<T>::quiet_NaN();
          for(size_t i = 0; i < target->elem(); ++i)
            target->operator[](i) = value;

          // Iterate over Flat Index
          for(const auto& pos: index_t.iter()){
            const size_t i = index_t.flatten(pos);
            target->operator[](flat.flatten(pos - index_t.min())) = source[i];
          }

          size_t shape[I::n_dims]{0};
          for(size_t d = 0; d < I::n_dims; ++d)
            shape[d] = flat[d];

          nb::ndarray<nb::numpy, T, nb::ndim<I::n_dims>> array(
            target->data(),
            I::n_dims,
            shape,
            nb::handle()
          );
          return nb::cast(std::move(array));

        }
        
        //! \todo Make this Generic
        else if constexpr(std::same_as<T, soil::vec3>) {

          soil::buffer_t<T> source = buffer.as<T>();  // Source Buffer w. Index

          // Typed Buffer of Flat Size
          //! \todo make sure this is de-allocated correctly,
          //! i.e. the numpy buffer should perform a copy.
          soil::buffer_t<T>* target  = new soil::buffer_t<T>(flat.elem()); 

          // Fill w. NaN Value
          //! \todo automate the related NaN value determination
          //buffer->fill(T{std::numeric_limits<float>::quiet_NaN()});

          T value = T{std::numeric_limits<float>::quiet_NaN()};
          for(size_t i = 0; i < target->elem(); ++i)
            target->operator[](i) = value;

          // Iterate over Flat Index
          for(const auto& pos: index_t.iter()){
            const size_t i = index_t.flatten(pos);
            target->operator[](flat.flatten(pos - index_t.min())) = source[i];
          }

          size_t shape[I::n_dims + 1]{0};
          for(size_t d = 0; d < I::n_dims; ++d)
            shape[d] = flat[d];
          shape[I::n_dims] = 3;

          nb::ndarray<nb::numpy, float, nb::ndim<I::n_dims+1>> array(
            target->data(),
            I::n_dims+1,
            shape,
            nb::handle()
          );
          return nb::cast(std::move(array));

        } else if constexpr(std::same_as<T, soil::vec2>) {

          soil::buffer_t<T> source = buffer.as<T>();  // Source Buffer w. Index

          // Typed Buffer of Flat Size
          //! \todo make sure this is de-allocated correctly,
          //! i.e. the numpy buffer should perform a copy.
          soil::buffer_t<T>* buffer  = new soil::buffer_t<T>(flat.elem()); 

          // Fill w. NaN Value
          //! \todo automate the related NaN value determination
          //buffer->fill(T{std::numeric_limits<float>::quiet_NaN()});

          T value = T{std::numeric_limits<float>::quiet_NaN()};
          for(size_t i = 0; i < buffer->elem(); ++i)
            buffer->operator[](i) = value;

          // Iterate over Flat Index
          for(const auto& pos: index_t.iter()){
            const size_t i = index_t.flatten(pos);
            buffer->operator[](flat.flatten(pos - index_t.min())) = source[i];
          }

          size_t shape[I::n_dims + 1]{0};
          for(size_t d = 0; d < I::n_dims; ++d)
            shape[d] = flat[d];
          shape[I::n_dims] = 2;

          nb::ndarray<nb::numpy, float, nb::ndim<I::n_dims+1>> array(
            buffer->data(),
            I::n_dims+1,
            shape,
            nb::handle()
          );
          return nb::cast(std::move(array));

        } else if constexpr(std::same_as<T, soil::ivec2>) {

          soil::buffer_t<T> source = buffer.as<T>();  // Source Buffer w. Index

          // Typed Buffer of Flat Size
          //! \todo make sure this is de-allocated correctly,
          //! i.e. the numpy buffer should perform a copy.
          soil::buffer_t<T>* target  = new soil::buffer_t<T>(flat.elem()); 

          // Fill w. NaN Value
          //! \todo automate the related NaN value determination
          //buffer->fill(T{std::numeric_limits<float>::quiet_NaN()});

          T value = T{std::numeric_limits<int>::quiet_NaN()};
          for(size_t i = 0; i < target->elem(); ++i)
            target->operator[](i) = value;

          // Iterate over Flat Index
          for(const auto& pos: index_t.iter()){
            const size_t i = index_t.flatten(pos);
            target->operator[](flat.flatten(pos - index_t.min())) = source[i];
          }

          size_t shape[I::n_dims + 1]{0};
          for(size_t d = 0; d < I::n_dims; ++d)
            shape[d] = flat[d];
          shape[I::n_dims] = 2;

          nb::ndarray<nb::numpy, int, nb::ndim<I::n_dims+1>> array(
            target->data(),
            I::n_dims+1,
            shape,
            nb::handle()
          );
          return nb::cast(std::move(array));

        } else {

          throw std::invalid_argument("can't convert type to numpy array");

        }
      
      });
    });
  });

  //
  // Special Layer-Based Operations
  //  These will be unified and expanded later!
  //

  node.def("track", [](soil::node& lhs, soil::node& rhs, const float lrate){

    if(lhs.type() != rhs.type())
      throw std::invalid_argument("nodes are not of the same type");

    soil::select(rhs.type(), [&lhs, &rhs, lrate]<typename T>(){
      if constexpr (std::is_floating_point_v<T>) {
        auto lhs_t = lhs.as<soil::cached>().buffer.as<T>();
        auto rhs_t = rhs.as<soil::cached>().buffer.as<T>();
        for(size_t i = 0; i < lhs_t.elem(); ++i){
          const T lhs_value = lhs_t[i];
          const T rhs_value = rhs_t[i];
          lhs_t[i] = lhs_value * (T(1.0) - lrate) + rhs_value * lrate;
        }
      } else
        throw std::invalid_argument("invalid type for operation");
      // throw soil::error::type_op_error<T>();
    });

  });

  //
  // Cache-Valued Layer, i.e. Lookup Table
  //

  module.def("cached", [](const soil::buffer& buffer){
    return soil::node(std::move(soil::cached(buffer)));
  });

  module.def("cached", [](const soil::dtype type, const size_t size){
    auto buffer = soil::buffer(type, size);
    return soil::node(std::move(soil::cached(std::move(buffer))));
  });

  //
  // Constant-Valued Layer
  //

  module.def("constant", [](const soil::dtype type, const nb::object object){
    return soil::select(type, [type, &object]<typename T>(){
      const T value = nb::cast<T>(object);
      return soil::node(std::move(soil::constant(type, value)));
    });
  });

  //
  // Generic Computed Layer
  //

  module.def("computed", [](const soil::dtype type, const nb::callable object){
    return soil::select(type, [type, &object]<typename T>(){
      using func_t = std::function<T(const size_t)>;
      func_t func = nb::cast<func_t>(object);
      return soil::node(std::move(soil::computed(type, func)));
    });
  });

  //
  //
  //

  auto normal = nb::class_<soil::normal>(module, "normal");
  normal.def(nb::init<const soil::index&, const soil::node&>());
  normal.def("full", [](const soil::normal& normal){
    return soil::node(std::move(normal.full()));
  });

  auto flow = nb::class_<soil::flow>(module, "flow");
  flow.def(nb::init<const soil::index&, const soil::node&>());
  flow.def("__call__", [](const soil::flow& flow){
    return soil::node(std::move(flow.full()));
  });

  auto direction = nb::class_<soil::direction>(module, "direction");
  direction.def(nb::init<const soil::index&, const soil::node&>());
  direction.def("__call__", [](const soil::direction& direction){
    return soil::node(std::move(direction.full()));
  });

  auto accumulation = nb::class_<soil::accumulation>(module, "accumulation");
  accumulation.def(nb::init<const soil::index&, const soil::node&>());
  accumulation.def("__call__", [](const soil::accumulation& accumulation){
    return soil::node(std::move(accumulation.full()));
  });

  accumulation.def_prop_rw("steps", 
  [](const soil::accumulation& accumulation){
    return accumulation.steps;
  },
  [](soil::accumulation& accumulation, const size_t steps){
    accumulation.steps = steps;
  });

  accumulation.def_prop_rw("iterations", 
  [](const soil::accumulation& accumulation){
    return accumulation.iterations;
  },
  [](soil::accumulation& accumulation, const size_t iterations){
    accumulation.iterations = iterations;
  });

  accumulation.def_prop_rw("samples", 
  [](const soil::accumulation& accumulation){
    return accumulation.samples;
  },
  [](soil::accumulation& accumulation, const size_t samples){
    accumulation.samples = samples;
  });

  //
  // Noise Sampler Type
  //

  module.def("noise", soil::make_noise);

}

#endif