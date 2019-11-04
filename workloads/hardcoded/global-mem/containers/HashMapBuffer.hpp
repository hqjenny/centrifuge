#pragma once

#include <cstdlib>
#include <cstdio>
#include <vector>

#include <BCL.hpp>
#include <containers/CircularQueue.hpp>

namespace BCL {
  template <
    typename K,
    typename V,
    typename KeySerialize = BCL::serialize <K>,
    typename ValSerialize = BCL::serialize <V>
  >
  struct HashMapBuffer {
    using HME = HashMapEntry <K, V, KeySerialize, ValSerialize>;
    BCL::HashMap <K, V, KeySerialize, ValSerialize> *hashmap;

    std::vector <BCL::CircularQueue <HME>> queues;
    std::vector <std::vector <HME>> buffers;

    uint64_t buffer_size;

    HashMapBuffer(BCL::HashMap <K, V, KeySerialize, ValSerialize> &hashmap,
      uint64_t queue_capacity, uint64_t buffer_size) {
      this->hashmap = &hashmap;

      this->buffer_size = buffer_size;

      for (int rank = 0; rank < BCL::nprocs(); rank++) {
        queues.push_back(BCL::CircularQueue <HME> (rank, queue_capacity));
      }

      for (int rank = 0; rank < BCL::nprocs(); rank++) {
        buffers.push_back(std::vector <HME> ());
      }
    }

    bool insert(const K &key, const V &val) {
      uint64_t hash = hashmap->hash_fn(key);
      uint64_t slot = hash % hashmap->size;
      uint64_t node = slot / hashmap->local_size;
      buffers[node].push_back(HME(key, val));

      if (buffers[node].size() >= buffer_size) {
        bool success = queues[node].push(buffers[node]);
        if (success) {
          buffers[node].clear();
          return true;
        } else {
          return false;
        }
      } else {
        return true;
      }
    }

    bool flush() {
      bool full_queues = false;
      bool success = true;
      do {
        // Flush local buffers to remote queues.
        // Only returns false if remote queues
        // are full.
        full_queues = !flush_buffers();
        // Flush local queues to hash table.
        // Only returns false if hash table is full.
        success = flush_queues();
      } while (success && full_queues);
      return success;
    }

    // Flush HMEs sitting in local queues
    bool flush_queues() {
      BCL::barrier();
      std::vector <HME> failed_inserts;
      int num = 0;
      bool success;
      HME entry;
      do {
        num++;
        success = queues[BCL::rank()].local_nonatomic_pop(entry);
        if (success) {
          bool inserted = hashmap->local_nonatomic_insert(entry);
          if (!inserted) {
            failed_inserts.push_back(entry);
          }
        }
      } while (success);
      BCL::barrier();

      success = true;
      for (HME &entry : failed_inserts) {
        if (!hashmap->insert(entry.get_key(), entry.get_val())) {
          success = false;
          break;
        }
      }

      int success_ = (success) ? 0 : 1;
      success_ = BCL::allreduce(success_, std::plus <int> ());

      return (success_ == 0);
    }

    // Flush local HME buffers to remote queues
    bool flush_buffers() {
      BCL::barrier();
      bool success = true;
      for (int rank = 0; rank < buffers.size(); rank++) {
        if (!queues[rank].push(buffers[rank])) {
          success = false;
        } else {
          buffers[rank].clear();
        }
      }

      int success_ = (success) ? 0 : 1;
      success_ = BCL::allreduce(success_, std::plus <int> ());

      return (success_ == 0);
    }
  };
};
