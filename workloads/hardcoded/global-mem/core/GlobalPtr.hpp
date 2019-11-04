#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>

#include <BCL.hpp>

namespace BCL {
  extern uint64_t shared_segment_size;
  extern void *smem_base_ptr;

  extern uint64_t rank();

  template <class T, class M> M get_member_type(M T:: *);
  #define GET_MEMBER_TYPE(mem) decltype(BCL::get_member_type(mem))

  // For GlobalPtr <T> object_ptr, a global pointer to a struct of type T,
  // and item, a type M member of struct T, return a GlobalPtr <M> which
  // points to the member item within object_ptr
  #define pointerto(item, object_ptr) \
    BCL::reinterpret_pointer_cast <GET_MEMBER_TYPE(&decltype(object_ptr)::type::item)> \
    (BCL::reinterpret_pointer_cast <char> (object_ptr) + offsetof(typename decltype(object_ptr)::type, item))


  // TODO: support NULL in a way that allows
  // address 0x0 to go used in the future.
  template <typename T>
  struct GlobalPtr {
    uint64_t rank = 0;
    uint64_t ptr = 0;

    typedef T type;

    GlobalPtr(const uint64_t rank = 0, const uint64_t ptr = 0) :
      rank(rank), ptr(ptr) {}
    GlobalPtr(const GlobalPtr <T> &ptr) {
      this->rank = ptr.rank;
      this->ptr = ptr.ptr;
    }

    GlobalPtr <T> &operator=(const GlobalPtr <T> &ptr) {
      this->rank = ptr.rank;
      this->ptr = ptr.ptr;
      return *this;
    }

    GlobalPtr(const std::nullptr_t null) {
      this->rank = 0;
      this->ptr = 0;
    }

    GlobalPtr <T> &operator=(const std::nullptr_t null) {
      this->rank = 0;
      this->ptr = 0;
      return *this;
    }

    bool operator==(const std::nullptr_t null) const {
      return (rank == 0 && ptr == 0);
    }

    bool operator!=(const std::nullptr_t null) const {
      return !(*this == null);
    }

    /*
    template <typename U>
    operator GlobalPtr <U> () const {
      return GlobalPtr <U> (this->rank, this->ptr);
    }
    */

    bool is_local() const {
      return rank == BCL::rank();
    }

    // Local pointer to somewhere in my shared memory segment.
    T *local() const {
      if (rank != BCL::rank()) {
        // TODO: Exception
        fprintf(stderr, "error: calling local() on a remote GlobalPtr\n");
        return nullptr;
      } else {
        return (T *) (((char *) BCL::smem_base_ptr) + ptr);
      }
    }

    // Pointer to shared memory segment on another node.
    // Users should not use this unless they're writing
    // custom SHMEM.
    T *rptr() const {
      return (T *) (((char *) BCL::smem_base_ptr) + ptr);
    }

    GlobalPtr <T> operator+(const size_t offset) const {
      return GlobalPtr <T> (rank, ptr + offset*sizeof(T));
    }

    GlobalPtr <T> operator-(const size_t offset) const {
      return GlobalPtr <T> (rank, ptr - offset*sizeof(T));
    }

    uint64_t operator-(const GlobalPtr <T> &ptr) const {
      return (this->ptr - ptr.ptr) / sizeof(T);
    }

    GlobalPtr <T> &operator++(int) {
      ptr += sizeof(T);
      return *this;
    }

    GlobalPtr <T> &operator++() {
      ptr += sizeof(T);
      return *this;
    }

    GlobalPtr <T> &operator--(int) {
      ptr -= sizeof(T);
      return *this;
    }

    GlobalPtr <T> &operator--() {
      ptr -= sizeof(T);
      return *this;
    }

    GlobalPtr <T> &operator+=(const size_t offset) {
      ptr += offset*sizeof(T);
      return *this;
    }

    GlobalPtr <T> &operator-=(const size_t offset) {
      ptr -= offset*sizeof(T);
      return *this;
    }

    bool operator==(const GlobalPtr <T> &ptr) const {
      return (this->ptr == ptr.ptr && this->rank == ptr.rank);
    }

    bool operator!=(const GlobalPtr <T> &ptr) const {
      return this->ptr != ptr.ptr || this->rank != ptr.rank;
    }

    std::string str() const {
      if (*this != nullptr) {
        return "GlobalPtr(" + std::to_string(rank) + ": " +
          std::to_string(ptr) + ")";
      } else {
        return "GlobalPtr(nullptr)";
      }
    }
    void print() const {
      printf("%s\n", str().c_str());
    }
  };

  template <typename T, typename U>
  GlobalPtr <T> reinterpret_pointer_cast(const GlobalPtr <U> &ptr) noexcept {
    return GlobalPtr <T> (ptr.rank, ptr.ptr);
  }
}
