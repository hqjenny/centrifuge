#include "../../BCL.hpp"
#include "../../containers/Container.hpp"

void print_vec(std::vector <size_t> &v) {
  for (size_t val : v) {
    printf("%lu ", val);
  }
  printf("\n");
}

// TODO: elegant support for begin => ARRAY_BEGIN,
//       end => ARRAY_END
struct slc {
  size_t begin, end, stride;

  slc(size_t begin, size_t end, size_t stride = 1) :
    begin(begin), end(end), stride(stride) {
    if (stride == 0) {
      throw std::runtime_error("slc: stride cannot be zero");
    }

    if (stride != 1) {
      throw std::runtime_error("slc: whoa buddy, do not support nonunit stride yet");
    }

    if (end <= begin) {
      throw std::runtime_error("slc: end of slice must precede beginning");
    }
  }

  size_t size() const noexcept {
    return (end - begin) / stride;
  }

  void print() {
    printf("slice(%lu, %lu, %lu)\n", begin, end, stride);
  }
};

template <typename T>
struct ArrayView;

template <typename T>
struct Array {
  BCL::GlobalPtr <T> data = nullptr;
  std::vector <size_t> dims;

  uint64_t host;

  template <typename... Args>
  Array(uint64_t host, size_t first_dim, Args... args) {
    dims = concat(dims, first_dim, args...);

    this->host = host;

    if (BCL::rank() == host) {
      data = BCL::alloc <T> (size());
    }
  }

  size_t size() const noexcept {
    if (dims.size() == 0) {
      return 0;
    }
    size_t my_size = 1;
    for (const size_t &dim : dims) {
      my_size *= dim;
    }
    return my_size;
  }

  Array(const Array &array) = delete;

  template <typename... Args>
  std::vector <size_t> concat(std::vector <size_t> &vals, size_t last) {
    vals.push_back(last);
    return vals;
  }

  template <typename... Args>
  std::vector <size_t> concat(std::vector <size_t> &vals, size_t first, Args... args) {
    vals.push_back(first);
    return concat(vals, args...);
  }

  template <typename... Args>
  T &operator()(size_t first, Args... args) {
    if (BCL::rank() == host) {
      std::vector <size_t> idx;
      idx = concat(idx, first, args...);
      if (idx.size() != dims.size()) {
        throw std::runtime_error("BCL Array: Hey there soldier, I don't support slicing like this yet");
      }
      size_t my_idx = 0;

      for (int i = 0; i < idx.size(); i++) {
        size_t stride = 1;
        for (int j = i + 1; j < dims.size(); j++) {
          stride *= dims[j];
        }
        my_idx += idx[i] * stride;
      }

      if (my_idx > size()) {
        throw std::runtime_error("BCL Array: out of bounds error");
      }

      return data.local()[my_idx];
    } else {
      throw std::runtime_error("BCL Array: attempted to directly index remote array");
    }
  }

  template <typename... Args>
  std::vector <slc> concat(std::vector <slc> &vals, slc last) {
    vals.push_back(last);
    return vals;
  }

  template <typename... Args>
  std::vector <slc> concat(std::vector <slc> &vals, slc first, Args... args) {
    vals.push_back(first);
    return concat(vals, args...);
  }

  template <typename... Args>
  ArrayView <T> operator()(slc first, Args... args) {
    std::vector <slc> idx;
    idx = concat(idx, first, args...);
    if (idx.size() != dims.size()) {
      throw std::runtime_error("BCL Array: Hey there soldier, I don't support slicing like this yet");
    }
    return ArrayView <T> (this, idx);
  }

  void print() {
    std::vector <slc> slices(dims.size(), slc(0, std::numeric_limits <size_t>::max()));
    ArrayView <T> (this, slices).print();
  }
};

template <typename T>
struct ArrayView {
  Array <T> *array;

  std::vector <slc> slices;

  ArrayView(Array <T> *array, const std::vector <slc> &slices) :
    array(array), slices(slices) {}

  std::vector <size_t> shape() const noexcept {
    std::vector <size_t> my_shape;
    for (int i = 0; i < slices.size(); i++) {
      my_shape.push_back(std::min(slices[i].size(), array->dims[i]));
    }
    return my_shape;
  }

  size_t size() const noexcept {
    if (array->dims.size() == 0) {
      return 0;
    }
    size_t my_size = 1;
    for (int i = 0; i < slices.size(); i++) {
      my_size *= std::min(array->dims[i], slices[i].size());
    }
    return my_size;
  }

  template <typename... Args>
  std::vector <size_t> concat(std::vector <size_t> &vals, size_t last) {
    vals.push_back(last);
    return vals;
  }

  template <typename... Args>
  std::vector <size_t> concat(std::vector <size_t> &vals, size_t first, Args... args) {
    vals.push_back(first);
    return concat(vals, args...);
  }

  static void modify(ArrayView <T> &lview, const T &rval, std::vector <size_t> &my_idx,
      size_t slice_idx, void op(T &, const T &)) {
    if (lview.array->host == BCL::rank()) {
      if (slice_idx == lview.slices.size()) {
        op(lview.get_elem(my_idx), rval);
      } else {
        slc slice = lview.slices[slice_idx];
        for (size_t idx = 0; idx < lview.shape()[slice_idx]; idx++) {
          size_t stride = 1;
          for (int j = slice_idx + 1; j < lview.shape().size(); j++) {
            stride *= lview.shape()[j];
          }
          my_idx[slice_idx] = idx;
          modify(lview, rval, my_idx, slice_idx + 1, op);
        }
      }
    }
  }

  // lview = rview
  static void modify(ArrayView <T> &lview, const ArrayView <T> &rview,
    std::vector <size_t> &my_idx, size_t slice_idx, void op(T &, const T &)) {
    if (lview.array->host == BCL::rank() && rview.array->host == BCL::rank()) {
      if (slice_idx == lview.slices.size()) {
        op(lview.get_elem(my_idx), rview.get_elem(my_idx));
      } else {
        slc slice = lview.slices[slice_idx];
        for (size_t idx = 0; idx < lview.shape()[slice_idx]; idx++) {
          size_t stride = 1;
          for (int j = slice_idx + 1; j < lview.shape().size(); j++) {
            stride *= lview.shape()[j];
          }
          my_idx[slice_idx] = idx;
          modify(lview, rview, my_idx, slice_idx + 1, op);
        }
      }
    }
  }

  ArrayView <T> &operator=(const ArrayView <T> &view) {
    std::vector <size_t> my_idx(shape().size(), 0);
    modify(*this, view, my_idx, 0, [] (T &lval, const T &rval) {
      lval = rval;
    });
    return *this;
  }

  ArrayView <T> &operator+=(const ArrayView <T> &view) {
    std::vector <size_t> my_idx(shape().size(), 0);
    modify(*this, view, my_idx, 0, [] (T &lval, const T &rval) {
      lval += rval;
    });
    return *this;
  }

  ArrayView <T> &operator-=(const ArrayView <T> &view) {
    std::vector <size_t> my_idx(shape().size(), 0);
    modify(*this, view, my_idx, 0, [] (T &lval, const T &rval) {
      lval -= rval;
    });
    return *this;
  }

  ArrayView <T> &operator*=(const ArrayView <T> &view) {
    std::vector <size_t> my_idx(shape().size(), 0);
    modify(*this, view, my_idx, 0, [] (T &lval, const T &rval) {
      lval *= rval;
    });
    return *this;
  }

  ArrayView <T> &operator/=(const ArrayView <T> &view) {
    std::vector <size_t> my_idx(shape().size(), 0);
    modify(*this, view, my_idx, 0, [] (T &lval, const T &rval) {
      lval /= rval;
    });
    return *this;
  }

  void print(std::vector <size_t> &my_idx, size_t slice_idx) const {
    if (slice_idx == slices.size()) {
      std::cout << get_elem(my_idx) << " ";
    } else {
      slc slice = slices[slice_idx];
      std::cout << "[";
      for (size_t idx = 0; idx < shape()[slice_idx]; idx++) {
        size_t stride = 1;
        for (int j = slice_idx + 1; j < shape().size(); j++) {
          stride *= shape()[j];
        }
        my_idx[slice_idx] = idx;
        print(my_idx, slice_idx + 1);
      }
      std::cout << "]" << std::endl;
    }
  }

  void print() const {
    std::vector <size_t> my_idx(shape().size(), 0);
    print(my_idx, 0);
  }

  ArrayView <T> &operator=(const T &val) {
    std::vector <size_t> my_idx(shape().size(), 0);
    modify(*this, val, my_idx, 0, [] (T &lval, const T &rval) {
      lval = rval;
    });
    return *this;
  }

  T &get_elem(const std::vector <size_t> &idx) {
    if (idx.size() != array->dims.size()) {
      throw std::runtime_error("BCL Array: Hey there soldier, I don't support slicing like this yet");
    }
    size_t my_idx = 0;

    for (int i = 0; i < idx.size(); i++) {
      if (idx[i] >= slices[i].size()) {
        throw std::runtime_error("BCL Array: out of bounds error");
      }
    }

    for (int i = 0; i < idx.size(); i++) {
      size_t stride = 1;
      for (int j = i+1; j < array->dims.size(); j++) {
        stride *= array->dims[j];
      }
      my_idx += (slices[i].begin + idx[i]*slices[i].stride) * stride;
    }

    if (my_idx > array->size()) {
      throw std::runtime_error("BCL Array: window making out of bounds access on Array");
    }

    return array->data.local()[my_idx];
  }

  T get_elem(const std::vector <size_t> &idx) const {
    if (idx.size() != array->dims.size()) {
      throw std::runtime_error("BCL Array: Hey there soldier, I don't support slicing like this yet");
    }
    size_t my_idx = 0;

    for (int i = 0; i < idx.size(); i++) {
      if (idx[i] >= slices[i].size()) {
        throw std::runtime_error("BCL Array: out of bounds error");
      }
    }

    for (int i = 0; i < idx.size(); i++) {
      size_t stride = 1;
      for (int j = i+1; j < array->dims.size(); j++) {
        stride *= array->dims[j];
      }
      my_idx += (slices[i].begin + idx[i]*slices[i].stride) * stride;
    }

    if (my_idx > array->size()) {
      throw std::runtime_error("BCL Array: window making out of bounds access on Array");
    }

    return array->data.local()[my_idx];
  }

  template <typename... Args>
  T &operator()(size_t first, Args... args) {
    std::vector <size_t> idx;
    idx = concat(idx, first, args...);
    if (idx.size() != array->dims.size()) {
      throw std::runtime_error("BCL Array: Hey there soldier, I don't support slicing like this yet");
    }
    return get_elem(idx);
  }
};

namespace BCL {
  template <typename T>
  struct serialize <ArrayView <T>> {
    ArrayView <T> operator()(const ArrayView <T> &val) const noexcept {
      return val;
    }
    ArrayView <T> deserialize(const ArrayView <T> &val) const noexcept {
      return val;
    }
  };

}
