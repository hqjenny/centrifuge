#include <cstdio>
#include <cstdlib>
#include <vector>
#include <list>
#include <set>
#include <numeric>
#include <cstddef>
#include <chrono>
#include "../../BCL.hpp"

#include "kmer_t.hpp"
#include "read_kmers.hpp"

int main(int argc, char **argv) {
  BCL::init();
  std::string kmer_fname = "test.dat";
  int n_kmers = line_count(kmer_fname);

  // Load factor of 0.7
  int hash_table_size = n_kmers * (1.0 / 0.7);
  int local_table_size = (hash_table_size + BCL::nprocs() - 1) / BCL::nprocs();

  if (BCL::rank() == 0) {
    printf("Initializing hash table of size %d for %d kmers.\n", hash_table_size,
      n_kmers);
    printf("local_table_size %d\n", local_table_size);
    fflush(stdout);
  }

  BCL::GlobalPtr <kmer_pair> my_table = BCL::alloc <kmer_pair> (local_table_size);

  if (my_table == nullptr) {
    fprintf(stderr, "error: out of memory.\n");
    BCL::finalize();
    exit(1);
  }

  for (int i = 0; i < local_table_size; i++) {
    new (&my_table.local()[i]) kmer_pair();
  }

  std::vector <BCL::GlobalPtr <kmer_pair>> kmer_hash(BCL::nprocs(), nullptr);
  kmer_hash[BCL::rank()] = my_table;

  for (int rank = 0; rank < BCL::nprocs(); rank++) {
    kmer_hash[rank] = BCL::broadcast(kmer_hash[rank], rank);
  }

  std::vector <kmer_pair> kmers = read_kmers(kmer_fname, BCL::nprocs(), BCL::rank());

  BCL::barrier();

  if (BCL::rank() == 0) {
    printf("Finished reading kmers.\n");
    fflush(stdout);
  }

  BCL::barrier();

  auto start = std::chrono::high_resolution_clock::now();

  for (auto &kmer : kmers) {
    int hash = kmer.hash(hash_table_size);
    int slot, node, node_slot;
    int probe = 0;
    int used;
    do {
      slot = (hash + probe++) % hash_table_size;
      node = slot / local_table_size;
      node_slot = slot - node*local_table_size;
      BCL::GlobalPtr <int> kmer_used = BCL::GlobalPtr <int> (BCL::GlobalPtr <char>
        (kmer_hash[node] + node_slot) + offsetof(kmer_pair, used));
      used = BCL::fetch_and_op <int> (kmer_used, 1, BCL::plus <int> ());
      if (used != 0) {
        BCL::fetch_and_op <int> (kmer_used, -1, BCL::plus <int> ());
      }
    } while (used != 0 && (hash + probe) % hash_table_size != hash);
    if (!used) {
      BCL::rput((char *) &kmer, BCL::GlobalPtr <char> (kmer_hash[node] + node_slot), sizeof(kmer_pair) - sizeof(int));
    } else {
      fprintf(stderr, "error! Hash table is full\n");
      break;
    }
  }
  kmers.clear();
  auto end_insert = std::chrono::high_resolution_clock::now();
  BCL::barrier();

  if (BCL::rank() == 0) {
    double insert_time = std::chrono::duration <double> (end_insert - start).count();
    printf("Finished inserting in %lf\n", insert_time);
    fflush(stdout);
  }
  BCL::barrier();

  auto start_read = std::chrono::high_resolution_clock::now();

  // For now, each node gets kmers bucketed to its portion of hash table.
  std::vector <kmer_pair> start_nodes;
  for (int i = 0; i < local_table_size; i++) {
    if (my_table.local()[i].used && my_table.local()[i].backwardExt() == 'F') {
      start_nodes.push_back(my_table.local()[i]);
    }
  }

  std::list <std::list <kmer_pair>> contigs;
  for (const auto &start_kmer : start_nodes) {
    std::list <kmer_pair> contig;
    contig.push_back(start_kmer);
    while (contig.back().forwardExt() != 'F') {
      int hash = contig.back().next_kmer().hash(hash_table_size);
      int slot, node, node_slot;
      int probe = 0;
      kmer_pair my_kmer;
      do {
        slot = (hash + probe++) % hash_table_size;
        node = slot / local_table_size;
        node_slot = slot - node*local_table_size;
        BCL::GlobalPtr <int> kmer_used = BCL::GlobalPtr <int> (BCL::GlobalPtr <char>
          (kmer_hash[node] + node_slot) + offsetof(kmer_pair, used));
        int used = BCL::rget(kmer_used);
        if (!used) {
          continue;
        }
        my_kmer = BCL::rget(kmer_hash[node] + node_slot);
      } while(my_kmer.kmer != contig.back().next_kmer() && (hash + probe) % hash_table_size != hash);
      if (my_kmer.kmer == contig.back().next_kmer()) {
        contig.push_back(my_kmer);
      } else {
        printf("Error! Key not found.\n");
        break;
      }
    }
    contigs.push_back(contig);
  }

  auto end_read = std::chrono::high_resolution_clock::now();
  BCL::barrier();
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration <double> read = end_read - start_read;
  std::chrono::duration <double> insert = end_insert - start;
  std::chrono::duration <double> total = end - start;

  int numKmers = std::accumulate(contigs.begin(), contigs.end(), 0,
    [] (int sum, const std::list <kmer_pair> &contig) {
      return sum + contig.size();
    });

  printf("Rank %d reconstructed %d contigs with %d nodes from %d start nodes. (%lf read, %lf insert, %lf total)\n",
    BCL::rank(), contigs.size(), numKmers, start_nodes.size(), read.count(), insert.count(), total.count());

  fflush(stdout);
  BCL::barrier();

  if (BCL::rank() == 0) {
    printf("Assembled in %lf total.\n", total.count());
  }

  BCL::finalize();
  return 0;
}
