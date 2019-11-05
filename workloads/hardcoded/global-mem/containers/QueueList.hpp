#pragma once

#include <BCL.hpp>

// QueueList is somewhat experimental

namespace BCL {
  template <typename T, typename TSerialize = BCL::serialize <T>>
  struct QueueNode {
    BCL::GlobalPtr <QueueNode <T, TSerialize>> next = nullptr;
    BCL::Container <T, TSerialize> value;
    int used = 0;
    int next_used = 0;
  };

  template <typename T, typename TSerialize = BCL::serialize <T>>
  struct QueueList {
    BCL::GlobalPtr <QueueNode <T>> head = nullptr;

    QueueList(uint64_t host) {
      if (BCL::rank() == 0) {
        head = BCL::alloc <QueueNode <int>> (1);
        new (head.local()) QueueNode <int> ();
      }
      head = BCL::broadcast(head, 0);
    }

    ~QueueList() {
      // TODO
    }

    void insert(const T &val) {
      int used = BCL::int_compare_and_swap(pointerto(used, head), 0, 1);
      // W00t! You got the bucket.  All you have to do is copy your val.
      if (used == 0) {
        BCL::Container <T, BCL::serialize <T>> cval;
        cval.set(val);
        BCL::rput(cval, pointerto(value, head));
        used = BCL::int_compare_and_swap(pointerto(used, head), 1, 2);
        if (used != 1) {
          throw std::runtime_error("BCL QueueList: Something weird happened--I lost the bucket.");
        }
      } else {
        BCL::GlobalPtr <QueueNode <T>> my_bucket = BCL::ralloc <QueueNode <T>> (1, head.rank);
        QueueNode <T> local_bucket;
        local_bucket.value.set(val);
        local_bucket.used = 2;
        BCL::rput(local_bucket, my_bucket);

        bool inserted = false;
        while (!inserted) {
          used = BCL::int_compare_and_swap(pointerto(next_used, head), 0, 1);
          // W00t! You've reserved the next pointer. All you have to do is copy
          // the pointer to the bucket you've already created.
          if (used == 0) {
            inserted = true;
            BCL::rput(my_bucket, pointerto(next, head));
            used = BCL::int_compare_and_swap(pointerto(next_used, head), 1, 2);
            if (used != 1) {
              throw std::runtime_error("BCL QueueList: Something weird happened--I lost the next ptr.");
            }
          } else {
            BCL::GlobalPtr <QueueNode <T>> new_head = nullptr;
            while (new_head == nullptr) {
              int next_used = BCL::rget(pointerto(next_used, head));
              if (next_used == 2) {
                new_head = BCL::rget(pointerto(next, head));
              }
            }
            head = new_head;
          }
        }
      }
    }
  };
}
