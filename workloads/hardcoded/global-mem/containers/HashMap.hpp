#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <type_traits>
#include <atomic>

#include <BCL.hpp>

#include <containers/Container.hpp>

namespace BCL {
  template <
    typename K,
    typename V,
    typename KeySerialize = BCL::serialize <K>,
    typename ValSerialize = BCL::serialize <V>
  >
  class HashMapEntry {
  public:
    BCL::Container <K, KeySerialize> key;
    BCL::Container <V, ValSerialize> val;

    HashMapEntry(const K &key, const V &val) {
      insert(key, val);
    }

    HashMapEntry(const HashMapEntry &entry) {
      key = entry.key;
      val = entry.val;
    }

    HashMapEntry() {}

    void insert(const K &key, const V &val) {
      this->key.set(key);
      this->val.set(val);
    }

    K get_key() const {
      return key.get();
    }

    V get_val() const {
      return val.get();
    }

    void set_key(const K &key) {
      this->key.set(key);
    }

    void set_val(const V &val) {
      this->val.set(val);
    }
  };

  template <
    typename K,
    typename V,
    typename KeySerialize = BCL::serialize <K>,
    typename ValSerialize = BCL::serialize <V>
    >
  class HashMap {
  public:
    using HME = HashMapEntry <K, V, KeySerialize, ValSerialize>;
    using KPTR = typename BCL::GlobalPtr <BCL::Container <K, KeySerialize>>;
    using VPTR = typename BCL::GlobalPtr <BCL::Container <V, ValSerialize>>;
    size_t size;
    size_t local_size;

    constexpr static int free_flag = 0;
    constexpr static int reserved_flag = 1;
    constexpr static int ready_flag = 2;

    std::hash <K> hash_fn;

    std::vector <BCL::GlobalPtr <HME>> hash_table;
    std::vector <BCL::GlobalPtr <int>> bucket_used;

    KPTR key_ptr(size_t slot) {
      uint64_t node = slot / local_size;
      uint64_t node_slot = slot - node*local_size;
      return pointerto(key, hash_table[node] + node_slot);
    }

    VPTR val_ptr(size_t slot) {
      uint64_t node = slot / local_size;
      uint64_t node_slot = slot - node*local_size;
      return pointerto(val, hash_table[node] + node_slot);
    }

    HashMap(const HashMap &hashmap) = delete;

    // Initialize a HashMap of at least size size.
    HashMap(size_t size) : size(size) {
      local_size = (size + BCL::nprocs() - 1) / BCL::nprocs();
      hash_table.resize(BCL::nprocs(), nullptr);
      bucket_used.resize(BCL::nprocs(), nullptr);

      hash_table[BCL::rank()] = BCL::alloc <HME> (local_size);
      bucket_used[BCL::rank()] = BCL::alloc <int> (local_size);

      if (bucket_used[BCL::rank()] == nullptr || hash_table[BCL::rank()] == nullptr) {
        fprintf(stderr, "error: not enough memory to allocate hash table.\n");
        return;
      }

      for (int i = 0; i < local_size; i++) {
        bucket_used[BCL::rank()].local()[i] = free_flag;
        new (&((hash_table[BCL::rank()].local())[i])) HME();
      }

      for (int rank = 0; rank < BCL::nprocs(); rank++) {
        hash_table[rank] = BCL::broadcast(hash_table[rank], rank);
        bucket_used[rank] = BCL::broadcast(bucket_used[rank], rank);
      }
      BCL::barrier();
    }

    ~HashMap() {
      if (!BCL::bcl_finalized) {
        if (BCL::rank() < hash_table.size() && hash_table[BCL::rank()] != nullptr) {
          BCL::dealloc(hash_table[BCL::rank()]);
        }
        if (BCL::rank() < bucket_used.size() && bucket_used[BCL::rank()] != nullptr) {
          BCL::dealloc(bucket_used[BCL::rank()]);
        }
      }
    }

    bool insert(const K &key, const V &val) {
      uint64_t hash = hash_fn(key);
      uint64_t probe = 0;
      bool success = false;
      do {
        uint64_t slot = (hash + get_probe(probe++)) % size;
        success = request_slot(slot, key);
        if (success) {
          HME entry = get_entry(slot);
          entry.set_key(key);
          entry.set_val(val);
          set_entry(slot, entry);
          ready_slot(slot);
        }
      } while (!success && probe < size);
      return success;
    }

    bool find_or_insert(const K &key, V &val) {
      uint64_t hash = hash_fn(key);
      uint64_t probe = 0;
      bool success = false;
      do {
        uint64_t slot = (hash + get_probe(probe++)) % size;
        int found = find_or_request_slot(slot, key);
        if (found == reserved_flag) {
          HME entry = get_entry(slot);
          entry.set_key(key);
          entry.set_val(val);
          set_entry(slot, entry);
          ready_slot(slot);
          success = true;
        } else if (found == ready_flag) {
          success = true;
          HME entry = get_entry(slot);
          val = entry.get_val();
        }
      } while (!success && probe < size);
      return success;
    }

    // Nonatomic with respect to remote inserts!
    bool local_nonatomic_insert(const HME &entry) {
      uint64_t hash = hash_fn(entry.get_key());
      uint64_t probe = 0;
      bool success = false;
      do {
        uint64_t slot = (hash + get_probe(probe++)) % size;
        uint64_t node_slot = slot - (local_size*BCL::rank());
        if (node_slot >= local_size || node_slot < 0) {
          break;
        } else if (bucket_used[BCL::rank()].local()[node_slot] == free_flag) {
          bucket_used[BCL::rank()].local()[node_slot] = ready_flag;
          set_entry(slot, entry);
          success = true;
        } else if (bucket_used[BCL::rank()].local()[node_slot] == ready_flag &&
          get_entry(slot).get_key() == entry.get_key()) {
          set_entry(slot, entry);
          success = true;
        }
      } while (!success);
      return success;
    }

    HME get_entry(uint64_t slot) {
      uint64_t node = slot / local_size;
      uint64_t node_slot = slot - node*local_size;
      return BCL::rget(hash_table[node] + node_slot);
    }

    void set_entry(uint64_t slot, const HME &entry) {
      uint64_t node = slot / local_size;
      uint64_t node_slot = slot - node*local_size;
      if (node_slot >= local_size) {
        throw std::runtime_error("node_slot too large!!!");
      }
      if (slot > size) {
        throw std::runtime_error("slot too large!!!");
      }
      BCL::rput(entry, hash_table[node] + node_slot);
    }

    int find_or_request_slot(uint64_t slot, const K &key) {
      uint64_t node = slot / local_size;
      uint64_t node_slot = slot - node*local_size;
      int used_val;
      /* If someone is currently inserting into this slot (reserved_flag), wait
         until they're finished to proceed. */
      do {
        used_val = BCL::int_compare_and_swap(bucket_used[node] + node_slot,
          free_flag, reserved_flag);
        // TODO: possibly optimize subsequent CASs to rget's
      } while (used_val == reserved_flag);
      /* used_val is ready_flag (remote is ready_flag) or
         free_flag (remote is now reserved_flag) */
      if (!(used_val == free_flag || used_val == ready_flag)) {
        throw std::runtime_error("HashMap forqs: used flag was somehow corrupted (-> reserved_flag). got " + std::to_string(used_val) + " at node " + std::to_string(node) + ", node_slot " + std::to_string(node_slot) + " " + (bucket_used[node] + node_slot).str());
      }
      if (used_val == ready_flag) {
        if (get_entry(slot).get_key() == key) {
          return ready_flag;
        } else {
          return free_flag;
        }
      } else {
        return reserved_flag;
      }
    }

    /* Request slot for key. If slot's free, take it.
       If slot's taken (ready_flag), reserve it (reserve_flag),
       so that you can write to it. */
    bool request_slot(uint64_t slot, const K &key) {
      uint64_t node = slot / local_size;
      uint64_t node_slot = slot - node*local_size;
      int used_val;
      /* If someone is currently inserting into this slot (reserved_flag), wait
         until they're finished to proceed. */
      do {
        used_val = BCL::int_compare_and_swap(bucket_used[node] + node_slot,
          free_flag, reserved_flag);
        // TODO: possibly optimize subsequent CASs to rget's
      } while (used_val == reserved_flag);
      /* used_val is ready_flag (remote is ready_flag) or
         free_flag (remote is now reserved_flag) */
      if (!(used_val == free_flag || used_val == ready_flag)) {
        throw std::runtime_error("HashMap rqs: used flag was somehow corrupted (-> reserved_flag). got " + std::to_string(used_val) + " at node " + std::to_string(node) + ", node_slot " + std::to_string(node_slot) + " " + (bucket_used[node] + node_slot).str() + " I am " + std::to_string(BCL::rank()));
      }
      if (used_val == ready_flag) {
        if (get_entry(slot).get_key() == key) {
          do {
            used_val = BCL::int_compare_and_swap(bucket_used[node] + node_slot,
              ready_flag, reserved_flag);
          } while (used_val != ready_flag);
          return true;
        } else {
          return false;
        }
      } else {
        return true;
      }
    }

    void check_slot(uint64_t slot) {
      uint64_t node = slot / local_size;
      uint64_t node_slot = slot - node*local_size;
      uint64_t used_val = BCL::int_compare_and_swap(bucket_used[node] + node_slot,
        reserved_flag, ready_flag);
      if (used_val != reserved_flag) {
        throw std::runtime_error("HashMap chslt: used flag was somehow corrupted (-> ready_flag). got " + std::to_string(used_val));
      }
    }

    int slot_status(uint64_t slot) {
      uint64_t node = slot / local_size;
      uint64_t node_slot = slot - node*local_size;
      return BCL::rget_atomic(bucket_used[node] + node_slot);
    }

    bool slot_ready(uint64_t slot) {
      uint64_t node = slot / local_size;
      uint64_t node_slot = slot - node*local_size;
      return (BCL::rget_atomic(bucket_used[node] + node_slot) == ready_flag);
    }

    void ready_slot(uint64_t slot) {
      uint64_t node = slot / local_size;
      uint64_t node_slot = slot - node*local_size;
      int used_val = BCL::int_compare_and_swap(bucket_used[node] + node_slot,
        reserved_flag, ready_flag);
      // TODO: if we fix updates to atomic, cannot be ready_flag
      if (!(used_val == reserved_flag || used_val == ready_flag)) {
        throw std::runtime_error("HashMap: used flag was somehow corrupted (-> ready_flag). got " + std::to_string(used_val));
      }
    }

    bool find(const K &key, V &val) {
      uint64_t hash = hash_fn(key);
      uint64_t probe = 0;
      bool success = false;
      HME entry;
      int status;
      do {
        uint64_t slot = (hash + get_probe(probe++)) % size;
        status = slot_status(slot);
        if (status == ready_flag) {
          entry = get_entry(slot);
          success = (entry.get_key() == key);
        }
      } while (!success && status != free_flag && probe < size);
      if (success) {
        val = entry.get_val();
        return true;
      } else {
        return false;
      }
    }

    bool find(const K &key, V &val, size_t &entry_slot) {
      uint64_t hash = hash_fn(key);
      uint64_t probe = 0;
      bool success = false;
      HME entry;
      int status;
      uint64_t slot;
      do {
        slot = (hash + get_probe(probe++)) % size;
        status = slot_status(slot);
        if (status == ready_flag) {
          entry = get_entry(slot);
          success = (entry.get_key() == key);
        }
      } while (!success && status != free_flag && probe < size);
      if (success) {
        val = entry.get_val();
        entry_slot = slot;
        return true;
      } else {
        return false;
      }
    }

    uint64_t get_probe(uint64_t probe) {
      return probe*probe;
    }
  };

  template <
    typename K,
    typename V,
    typename KeySerialize,
    typename ValSerialize
  >
  struct serialize <HashMapEntry <K, V, KeySerialize, ValSerialize>>
    : public BCL::identity_serialize <HashMapEntry <K, V, KeySerialize, ValSerialize>> {};
}
