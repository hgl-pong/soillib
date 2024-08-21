#ifndef SOILLIB_INDEX_QUAD
#define SOILLIB_INDEX_QUAD

#include <soillib/soillib.hpp>
#include <soillib/util/types.hpp>
#include <soillib/util/yield.hpp>
#include <soillib/index/flat.hpp>

#include <climits>
#include <vector>

namespace soil {

struct quad_node {

  typedef soil::ivec2 vec_t;  //!< Underlying Vector Type
  typedef flat_t<2> index_t;  //!< Node Index Type

  quad_node(const vec_t _min, const vec_t _ext):
    _min{_min},
    _ext{_ext},
    _max{_min+_ext},
    index(_ext){}

  // Flatten / Unflatten / Bounds Interface

  size_t flatten(const vec_t pos) const {
    return index.flatten(pos - this->_min);
  }

  vec_t unflatten(const int i) const {
    return this->_min + index.unflatten(i);
  }

  bool oob(const vec_t pos) const {
    return index.oob(pos - this->_min);
  }

  // Data Inspection Methods

  //! Number of Elements of Node
  vec_t min() const noexcept { return this->_min; }
  vec_t max() const noexcept { return this->_max; }
  vec_t ext() const noexcept { return this->_ext; }
  inline size_t elem() const { return index.elem(); } 

private:
  const vec_t _min; //!< Min Position of Node (World)
  const vec_t _ext; //!< Total Extent of Node
  const vec_t _max; //!< Max Position of Node (World)
  index_t index;    //!< Underlying Flat Index
};

//! basic quadtree structured index
//! \todo implement
struct quad: indexbase {

  static constexpr size_t D = 2;  //!< Dimensionality
  typedef soil::ivec2 vec_t;      //!< Underlying Vector Type
  typedef flat_t<D> index_t;      //!< Node Index Type
  typedef quad_node node_t;       //!< Node Type

  quad(std::vector<node_t>& nodes):
    nodes{nodes}{
    for(const auto& node: nodes){
      this->_min = glm::min(this->_min, node.min());
      this->_max = glm::max(this->_max, node.max());
    }
    this->_ext = this->_max - this->_min;
  }

  constexpr inline size_t dims() const noexcept { 
    return D; 
  }

  constexpr soil::dindex type() noexcept override {
    return soil::dindex::QUAD;
  }

  inline size_t operator[](const size_t d) const {
    return this->_ext[d];
  }

  // Flattening / Unflattening Interface

  size_t flatten(const vec_t pos) const {
    for(const auto& node: nodes)
      if(!node.oob(pos)) 
        return node.flatten(pos);
    return -1;
  }

  bool oob(const vec_t pos) const {
    for(const auto& node: nodes)
      if(!node.oob(pos)) 
        return false;
    return true;
  }

  // Data Inspection Interface

  vec_t min() const noexcept { return this->_min; }
  vec_t max() const noexcept { return this->_max; }
  vec_t ext() const noexcept { return this->_ext; }
  inline size_t elem() const {
    size_t count = 0;
    for(const auto& node: nodes)
      count += node.elem();
    return count;
  }

private:
  std::vector<node_t> nodes;    //!< Vector of Nodes
  vec_t _min = vec_t(INT_MAX);  //!< Min Position of Quad (World)
  vec_t _max = vec_t(INT_MIN);  //!< Max Position of Quad (World)
  vec_t _ext = vec_t(0);        //!< Total Extent of Quad (World)
  size_t area = 0;
};

} // end of namespace soil

#endif