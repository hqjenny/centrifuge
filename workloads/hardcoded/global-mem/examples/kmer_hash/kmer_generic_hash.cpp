#include <cstdio>
#include <cstdlib>
#include <vector>
#include <list>
#include <limits>
#include <numeric>

#include <unordered_set>

#include <BCL.hpp>

#include "kmer_t.hpp"
#include "read_kmers.hpp"

#include <containers/HashMap.hpp>
#include <containers/HashMapBuffer.hpp>

struct fb_ext {
  char ext[2];
  int used = 0;

  fb_ext(const char ext[2]) {
    init(ext);
  }

  fb_ext(const fb_ext &ext) {
    this->used = ext.used;
    init(ext.ext);
  }

  fb_ext(const std::string ext) {
    init(ext.data());
  }

  fb_ext operator=(const fb_ext &ext) {
    this->used = ext.used;
    init(ext.ext);
    return *this;
  }

  fb_ext() {}

  std::string get() const noexcept {
    return std::string(ext, 2);
  }

  char backwardExt() const noexcept {
    return ext[0];
  }

  char forwardExt() const noexcept {
    return ext[1];
  }

  void init(const char ext[2]) {
    for (int i = 0; i < 2; i++)
      this->ext[i] = ext[i];
  }
};

namespace std {
  template <>
  struct hash <pkmer_t> {
    size_t operator()(const pkmer_t &pkmer) const noexcept {
      return pkmer.hash();
    }
  };
}

size_t end_start_used = 0;
size_t end_local_used = 0;
size_t end_used = 0;
size_t end_xf = 0;
size_t end_no_match = 0;
size_t end_not_found = 0;

std::list <std::list <kmer_pair>> walk_right(BCL::HashMap <pkmer_t, fb_ext,
    BCL::identity_serialize <pkmer_t>,
    BCL::identity_serialize <fb_ext>> &kmer_hash, std::vector <kmer_pair> &start_nodes) {
  std::list <std::list <kmer_pair>> contigs;
  for (const auto &start_kmer : start_nodes) {
    fb_ext fb;
    size_t slot;
    bool success = kmer_hash.find(start_kmer.kmer, fb, slot);
    if (!success) {
      throw std::runtime_error("Could not find start_kmer " + start_kmer.kmer.get() + " in hash table.");
      continue;
    }
    BCL::GlobalPtr <fb_ext> fb_ptr = decay_container(kmer_hash.val_ptr(slot));
    BCL::GlobalPtr <int> used_ptr = pointerto(used, fb_ptr);
    int used_val = BCL::int_compare_and_swap(used_ptr, 0, 1);
    // int used_val = 0;
    if (used_val != 0) {
      end_start_used++;
      // throw std::runtime_error("Used start_kmer at " + fb.used.str() + " was not 0! was " + std::to_string(used_val));
      continue;
    }
    std::list <kmer_pair> contig;
    contig.push_back(start_kmer);
    std::unordered_set <pkmer_t> used_kmers;
    used_kmers.insert(pkmer_t(canonicalize(start_kmer.kmer_str())));
    while (success && contig.back().forwardExt() != 'F'
           && contig.back().forwardExt() != 'X') {
      if (contig.back().next_kmer().get() != contig.back().kmer_str().substr(1) + contig.back().forwardExt()) {
        throw std::runtime_error("Error: next kmer wrong!!! " + contig.back().next_kmer().get() + 
            " != " + contig.back().kmer_str().substr(1) + contig.back().forwardExt());
      }
      pkmer_t next_kmer = pkmer_t(canonicalize(contig.back().next_kmer().get()));
      if (used_kmers.find(next_kmer) != used_kmers.end()) {
        end_local_used++;
        break;
      }
      fb_ext fb;
      size_t slot;
      success = kmer_hash.find(next_kmer, fb, slot);
      if (success) {
        BCL::GlobalPtr <fb_ext> fb_ptr = decay_container(kmer_hash.val_ptr(slot));
        BCL::GlobalPtr <int> used_ptr = pointerto(used, fb_ptr);
        int used_val = BCL::int_compare_and_swap(used_ptr, 0, 1);
        // int used_val = 0;
        if (used_val != 0) {
          end_used++;
          success = false;
        } else {
          if (next_kmer.get() != contig.back().next_kmer().get()) {
            std::string new_exts = rcomplement(fb.get());
            fb_ext new_fb(new_exts);
            fb.init(new_fb.ext);
          }
          if (contig.back().kmer_str()[0] == fb.backwardExt()) {
            used_kmers.insert(next_kmer);
            contig.push_back(kmer_pair(contig.back().next_kmer().get(), fb.get()));
          } else {
            end_no_match++;
            int used_val = BCL::int_compare_and_swap(used_ptr, 1, 0);
            // int used_val = 1;
            // start_nodes.push_back(kmer_pair(contig.back().next_kmer().get(), fb.get()));
            if (used_val != 1) {
              throw std::runtime_error("used value unset somehow");
            }
            success = false;
          }
        }
      } else {
        end_not_found++;
      }
      if (contig.back().forwardExt() == 'F' || contig.back().forwardExt() != 'X') {
        end_xf++;
      }
    }
    contigs.push_back(contig);
  }
  return contigs;
}

std::list <std::list <kmer_pair>> walk_left(BCL::HashMap <pkmer_t, fb_ext,
    BCL::identity_serialize <pkmer_t>,
    BCL::identity_serialize <fb_ext>> &kmer_hash, std::vector <kmer_pair> &start_nodes) {
  std::list <std::list <kmer_pair>> contigs;
  for (const auto &start_kmer : start_nodes) {
    fb_ext fb;
    size_t slot;
    bool success = kmer_hash.find(start_kmer.kmer, fb, slot);
    if (!success) {
      throw std::runtime_error("Could not find start_kmer " + start_kmer.kmer.get() + " in hash table.");
      continue;
    }
    BCL::GlobalPtr <fb_ext> fb_ptr = decay_container(kmer_hash.val_ptr(slot));
    BCL::GlobalPtr <int> used_ptr = pointerto(used, fb_ptr);
    int used_val = BCL::int_compare_and_swap(used_ptr, 0, 1);
    // int used_val = 0;
    if (used_val != 0) {
      end_start_used++;
      // throw std::runtime_error("Used start_kmer at " + fb.used.str() + " was not 0! was " + std::to_string(used_val));
      continue;
    }
    std::list <kmer_pair> contig;
    contig.push_back(start_kmer);
    std::unordered_set <pkmer_t> used_kmers;
    used_kmers.insert(pkmer_t(canonicalize(start_kmer.kmer_str())));
    while (success && contig.front().backwardExt() != 'F'
           && contig.front().backwardExt() != 'X') {
      pkmer_t last_kmer = pkmer_t(canonicalize(contig.front().last_kmer().get()));
      if (used_kmers.find(last_kmer) != used_kmers.end()) {
        end_local_used++;
        break;
      }
      fb_ext fb;
      size_t slot;
      success = kmer_hash.find(last_kmer, fb, slot);
      if (success) {
        BCL::GlobalPtr <fb_ext> fb_ptr = decay_container(kmer_hash.val_ptr(slot));
        BCL::GlobalPtr <int> used_ptr = pointerto(used, fb_ptr);
        int used_val = BCL::int_compare_and_swap(used_ptr, 0, 1);
        // int used_val = 0;
        if (used_val != 0) {
          end_used++;
          success = false;
        } else {
          if (last_kmer.get() != contig.front().last_kmer().get()) {
            fb_ext new_fb(rcomplement(fb.get()));
            fb.init(new_fb.ext);
          }
          if (contig.front().kmer_str().back() == fb.forwardExt()) {
            used_kmers.insert(last_kmer);
            contig.push_front(kmer_pair(contig.front().last_kmer().get(), fb.get()));
          } else {
            end_no_match++;
            BCL::GlobalPtr <fb_ext> fb_ptr = decay_container(kmer_hash.val_ptr(slot));
            BCL::GlobalPtr <int> used_ptr = pointerto(used, fb_ptr);
            int used_val = BCL::int_compare_and_swap(used_ptr, 1, 0);
            // int used_val = 1;
            // start_nodes.push_back(kmer_pair(contig.front().last_kmer().get(), fb.get()));
            if (used_val != 1) {
              throw std::runtime_error("used value unset somehow");
            }
            success = false;
          }
        }
      } else {
        end_not_found++;
      }
      if (contig.back().backwardExt() == 'F' || contig.back().backwardExt() != 'X') {
        end_xf++;
      }
    }
    contigs.push_back(contig);
  }
  return contigs;
}

int main (int argc, char **argv) {
  BCL::init();
  std::string kmer_fname = "/global/cscratch1/sd/brock/267-dataset/human-new.dat";
  size_t n_kmers = 0;
  if (BCL::rank() == 0) {
    n_kmers = line_count(kmer_fname);
  }

  n_kmers = BCL::allreduce <uint64_t> (n_kmers, std::plus <uint64_t> ());
  BCL::print("Got %llu k-mers\n", n_kmers);

  // Load factor of 0.7
  size_t hash_table_size = n_kmers * (1.0 / 0.5);


  BCL::HashMap <pkmer_t, fb_ext,
    BCL::identity_serialize <pkmer_t>,
    BCL::identity_serialize <fb_ext>> kmer_hash(hash_table_size);

  BCL::print("Initializing hash table of size %d for %d kmers.\n", hash_table_size,
    n_kmers);

  std::vector <kmer_pair> kmers = read_kmers(kmer_fname, BCL::nprocs(), BCL::rank());

  int canonicalized = 0;
  for (int i = 0; i < kmers.size(); i++) {
    if (kmers[i].kmer_str() != canonicalize(kmers[i].kmer_str())) {
      std::string kmer_str = canonicalize(kmers[i].kmer_str());
      std::string fb = rcomplement(kmers[i].fb_ext_str());
      kmers[i] = kmer_pair(kmer_str, fb);
      canonicalized++;
    }
  }

  canonicalized = BCL::allreduce <int> (canonicalized, std::plus <int> ());

  BCL::print("%d k-mers canonicalized\n", canonicalized);

  BCL::print("Finished reading k-mers.\n");

  BCL::barrier();

  std::vector <kmer_pair> right_start_nodes;
  std::vector <kmer_pair> left_start_nodes;

  BCL::HashMapBuffer <pkmer_t, fb_ext,
    BCL::identity_serialize <pkmer_t>,
    BCL::identity_serialize <fb_ext>> kmer_hash_buffer(kmer_hash, 2*(n_kmers / BCL::nprocs()), 200);

  auto start = std::chrono::high_resolution_clock::now();

  for (const auto &kmer : kmers) {
    fb_ext fb(kmer.fb_ext);
    // bool success = kmer_hash.insert(kmer.kmer, fb);
    bool success = kmer_hash_buffer.insert(kmer.kmer, fb);
    if (!success) {
      throw std::runtime_error("BCL HashMapBuffer: not enough space");
    }
    if (kmer.backwardExt() == 'F' || kmer.backwardExt() == 'X') {
      right_start_nodes.push_back(kmer);
    }
    if (kmer.forwardExt() == 'F' || kmer.forwardExt() == 'X') {
      left_start_nodes.push_back(kmer);
    }
  }
  kmer_hash_buffer.flush();
  auto end_insert = std::chrono::high_resolution_clock::now();
  BCL::barrier();

  double insert_time = std::chrono::duration <double> (end_insert - start).count();
  BCL::print("Finished inserting in %lf\n", insert_time);

  using CT = typename std::list <std::list <kmer_pair>>;

  CT contigs;

  auto start_read = std::chrono::high_resolution_clock::now();

  CT right_tigs = walk_right(kmer_hash, right_start_nodes);
  CT left_tigs = walk_left(kmer_hash, left_start_nodes);

  BCL::barrier();
  CT right_wild_tigs;

  for (int i = 0; i < kmer_hash.local_size; i++) {
    if (kmer_hash.bucket_used[BCL::rank()].local()[i] == kmer_hash.ready_flag) {
      BCL::GlobalPtr <fb_ext> fb_ptr = decay_container(kmer_hash.val_ptr(BCL::rank()*kmer_hash.local_size + i));
      if (*pointerto(used, fb_ptr).local() == 0) {
        pkmer_t kmer = kmer_hash.hash_table[BCL::rank()].local()[i].get_key();
        fb_ext fb = kmer_hash.hash_table[BCL::rank()].local()[i].get_val();
        kmer_pair my_kmer(kmer.get(), fb.get());
        if (my_kmer.backwardExt() != 'F' && my_kmer.backwardExt() != 'X') {
          pkmer_t last_kmer(canonicalize(my_kmer.last_kmer().get()));
          bool success = kmer_hash.find(last_kmer, fb);
          if (!success) {
            std::vector <kmer_pair> wild_right_start_node;
            wild_right_start_node.push_back(my_kmer);
            CT wild_tig = walk_right(kmer_hash, wild_right_start_node);
            right_wild_tigs.splice(right_wild_tigs.end(), wild_tig);
          } else {
            if (last_kmer.get() != my_kmer.last_kmer().get()) {
              std::string new_exts = rcomplement(fb.get());
              fb_ext new_fb(new_exts);
              fb.init(new_fb.ext);
            }
            if (my_kmer.kmer_str().back() != fb.forwardExt()) {
              std::vector <kmer_pair> wild_right_start_node;
              wild_right_start_node.push_back(my_kmer);
              CT wild_tig = walk_right(kmer_hash, wild_right_start_node);
              right_wild_tigs.splice(right_wild_tigs.end(), wild_tig);
            }
          }
        }
      }
    }
  }

  CT left_wild_tigs;

  for (int i = 0; i < kmer_hash.local_size; i++) {
    if (kmer_hash.bucket_used[BCL::rank()].local()[i] == kmer_hash.ready_flag) {
      BCL::GlobalPtr <fb_ext> fb_ptr = decay_container(kmer_hash.val_ptr(BCL::rank()*kmer_hash.local_size + i));
      if (*pointerto(used, fb_ptr).local() == 0) {
        pkmer_t kmer = kmer_hash.hash_table[BCL::rank()].local()[i].get_key();
        fb_ext fb = kmer_hash.hash_table[BCL::rank()].local()[i].get_val();
        kmer_pair my_kmer(kmer.get(), fb.get());
        if (my_kmer.forwardExt() != 'F' && my_kmer.forwardExt() != 'X') {
          pkmer_t next_kmer(canonicalize(my_kmer.next_kmer().get()));
          bool success = kmer_hash.find(next_kmer, fb);
          if (!success) {
            std::vector <kmer_pair> wild_left_start_node;
            wild_left_start_node.push_back(my_kmer);
            CT wild_tig = walk_left(kmer_hash, wild_left_start_node);
            left_wild_tigs.splice(left_wild_tigs.end(), wild_tig);
          } else {
            if (next_kmer.get() != my_kmer.next_kmer().get()) {
              std::string new_exts = rcomplement(fb.get());
              fb_ext new_fb(new_exts);
              fb.init(new_fb.ext);
            }
            if (my_kmer.kmer_str()[0] != fb.backwardExt()) {
              std::vector <kmer_pair> wild_left_start_node;
              wild_left_start_node.push_back(my_kmer);
              CT wild_tig = walk_left(kmer_hash, wild_left_start_node);
              left_wild_tigs.splice(left_wild_tigs.end(), wild_tig);
            }
          }
        }
      }
    }
  }

  auto end_read = std::chrono::high_resolution_clock::now();
  BCL::barrier();
  auto end = std::chrono::high_resolution_clock::now();

  size_t n_right_tigs = BCL::allreduce <uint64_t> (right_tigs.size(), std::plus <uint64_t> {});
  size_t n_left_tigs = BCL::allreduce <uint64_t> (left_tigs.size(), std::plus <uint64_t> {});
  size_t n_wright_tigs = BCL::allreduce <uint64_t> (right_wild_tigs.size(), std::plus <uint64_t> {});
  size_t n_wleft_tigs = BCL::allreduce <uint64_t> (left_wild_tigs.size(), std::plus <uint64_t> {});

  contigs.splice(contigs.end(), right_tigs);
  contigs.splice(contigs.end(), left_tigs);
  contigs.splice(contigs.end(), right_wild_tigs);
  contigs.splice(contigs.end(), left_wild_tigs);

  BCL::print("%llu, %llu, %llu, %llu\n", n_right_tigs, n_left_tigs, n_wright_tigs,
    n_wleft_tigs);

  end_not_found = BCL::allreduce <uint64_t> (end_not_found, std::plus <uint64_t> ());
  end_start_used = BCL::allreduce <uint64_t> (end_start_used, std::plus <uint64_t> ());
  end_local_used = BCL::allreduce <uint64_t> (end_local_used, std::plus <uint64_t> ());
  end_used = BCL::allreduce <uint64_t> (end_used, std::plus <uint64_t> ());
  end_no_match = BCL::allreduce <uint64_t> (end_no_match, std::plus <uint64_t> ());
  end_xf = BCL::allreduce <uint64_t> (end_xf, std::plus <uint64_t> ());

  BCL::print("%llu not found, %llu start used, %llu local used, %llu used, %llu no match, %llu xf\n",
    end_not_found, end_start_used, end_local_used, end_used, end_no_match, end_xf);

  std::chrono::duration <double> read = end_read - start_read;
  std::chrono::duration <double> insert = end_insert - start;
  std::chrono::duration <double> total = end - start;

  int numKmers = std::accumulate(contigs.begin(), contigs.end(), 0,
    [] (int sum, const std::list <kmer_pair> &contig) {
      return sum + contig.size();
    });

  printf("Rank %d reconstructed %d contigs with %d nodes from %d start nodes. "
    "(%lf read, %lf insert, %lf total)\n", BCL::rank(), contigs.size(),
    numKmers, right_start_nodes.size() + left_start_nodes.size(), read.count(),
    insert.count(), total.count());

  int numBases = std::accumulate(contigs.begin(), contigs.end(), 0,
    [] (int sum, const std::list <kmer_pair> &contig) {
      return sum + KMER_LEN + contig.size() - 1;
    });

  int totalKmers = BCL::allreduce(numKmers, std::plus <int> ());
  int totalContigs = BCL::allreduce((int) contigs.size(), std::plus <int> ());
  int totalBases = BCL::allreduce(numBases, std::plus <int> ());
  BCL::barrier();
  fflush(stdout);
  BCL::barrier();

  BCL::print("Assembled in %lf total. (%lf, %lf)\n", total.count(), insert.count(),
    read.count());


  /*
  std::ofstream fout("test_" + std::to_string(BCL::rank()) + ".dat");
  for (const auto &contig : contigs) {
    std::string my_contig = extract_contig(contig);
    std::string canonicalized_contig = canonicalize(my_contig.substr(1, my_contig.length()-2));
    if (canonicalized_contig.length() >= KMER_LEN) {
      fout << canonicalized_contig << std::endl;
    }
  }
  fout.close();
  */

  BCL::finalize();
  return 0;
}
