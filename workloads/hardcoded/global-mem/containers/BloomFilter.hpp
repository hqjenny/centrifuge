#pragma once

#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <BCL.hpp>

namespace BCL {
  template <typename T>
  struct BloomFilter {
    std::vector <BCL::GlobalPtr <uint16_t>> data;
    size_t my_size;
    size_t local_size;

    // "Number of hash functions"
    size_t k = 0;

    std::hash <T> hash_fn;

    // TODO: Better hash function
    uint64_t hash(uint64_t key) {
      key = (~key) + (key << 21); // key = (key << 21) - key - 1;
      key = key ^ (key >> 24);
      key = (key + (key << 3)) + (key << 8); // key * 265
      key = key ^ (key >> 14);
      key = (key + (key << 2)) + (key << 4); // key * 21
      key = key ^ (key >> 28);
      key = key + (key << 31);
      return key;
    }

    size_t size() {
      return my_size;
    }

    size_t bit_size() {
      return size() * sizeof(uint16_t) * 8;
    }

    // n - expected size of data
    // p - desired false positive rate
    BloomFilter(size_t n, double p = 0.01) {
      double bits_per_elem = -log2(p) / log(2);
      int m = (bits_per_elem * n) + 0.5;

      k = (-(log(p) / log(2))) + 0.5;

      if (k < 1) {
        k = 1;
      }

      /*
      BCL::print("%d unique elements, %lf bits per elem, %d bits total, %lf (%d) hash fns\n",
        n, bits_per_elem, m, (-(log(p) / log(2))), k);
      */

      my_size = (m + 8*sizeof(uint16_t) - 1) / (8*sizeof(uint16_t));

      /*
      BCL::print("That's %dx%d-bit shorts=%d bits\n", my_size, 8*sizeof(uint16_t), my_size*8*sizeof(uint16_t));
      */

      local_size = (size() + BCL::nprocs() - 1) / BCL::nprocs();
      /*
      BCL::print("%d shorts per proc (%d procs)\n", local_size, BCL::nprocs());
      */

      data.resize(BCL::nprocs(), nullptr);
      for (int rank = 0; rank < BCL::nprocs(); rank++) {
        if (BCL::rank() == rank) {
          data[rank] = BCL::alloc <uint16_t> (local_size);

          for (int i = 0; i < local_size; i++) {
            data[rank].local()[i] = 0x0;
          }
        }
        data[rank] = BCL::broadcast(data[rank], rank);
      }
    }

    ~BloomFilter() {
      if (!BCL::bcl_finalized) {
        if (BCL::rank() < data.size() && data[BCL::rank()] != nullptr) {
          BCL::dealloc(data[BCL::rank()]);
        }
      }
    }

    void print() {
      BCL::barrier();
      if (BCL::rank() == 0) {
        for (int i = 0; i < size()*8*sizeof(uint16_t); i++) {
          printf("%d", get_bit(i));
        }
        printf("\n");
      }
      BCL::barrier();
    }

    // Set a bit positive in the bloom filter.
    bool set_bit(size_t bit) {
      size_t slot = bit / (8*sizeof(uint16_t));
      size_t node = slot / local_size;
      size_t node_slot = slot - node*local_size;
      int local_bit = bit % (8*sizeof(uint16_t));

      bool present = true;
      uint16_t val = BCL::rget(data[node] + node_slot);

      while ((val & (0x1 << local_bit)) == 0) {
        uint16_t new_val = val | (0x1 << local_bit);
        uint16_t old_val = BCL::uint16_compare_and_swap(data[node] + node_slot, val, new_val);
        if (old_val == val) {
          present = false;
          val = new_val;
        } else {
          val = old_val;
        }
      }
      return present;
    }

    bool get_bit(size_t bit) {
      size_t slot = bit / (8*sizeof(uint16_t));
      size_t node = slot / local_size;
      size_t node_slot = slot - node*local_size;
      int local_bit = bit % (8*sizeof(uint16_t));

      uint16_t val = BCL::rget(data[node] + node_slot);

      return val & (0x1 << local_bit);
    }

    // Insert val into bloom filter
    void insert(const T &val) {
      uint64_t my_hash = hash(hash_fn(val));

      set_bit(my_hash % bit_size());

      for (int i = 1; i < k; i++) {
        my_hash = hash(my_hash);
        set_bit(my_hash % bit_size());
      }
    }

    // Check if val is in bloom filter
    bool find(const T &val) {
      uint64_t my_hash = hash(hash_fn(val));

      if (!get_bit(my_hash % bit_size())) {
        return false;
      }

      for (int i = 1; i < k; i++) {
        my_hash = hash(my_hash);
        if (!get_bit(my_hash % bit_size())) {
          return false;
        }
      }
      return true;
    }

    BloomFilter(const BloomFilter &bloom_filter) = delete;
  };
}
