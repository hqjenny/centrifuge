#include <cstdio>
#include <cstdlib>
#include <vector>
#include <list>
#include <limits>
#include <numeric>

#include "../../BCL.hpp"
#include "../../containers/HashMap.hpp"

#include "kmer_t.hpp"
#include "read_kmers.hpp"

int main (int argc, char **argv) {
  BCL::init(1536);
  std::string kmer_fname = "test.dat";
  int n_kmers = line_count(kmer_fname);

  // Load factor of 0.7
  int hash_table_size = n_kmers * (1.0 / 0.7);

  BCL::HashMap <std::string, std::string> kmer_hash(hash_table_size);

  if (BCL::rank() == 0) {
    printf("Initializing hash table of size %d for %d kmers.\n", hash_table_size,
      n_kmers);
    fflush(stdout);
  }

  std::vector <kmer_pair> kmers = read_kmers(kmer_fname, BCL::nprocs(), BCL::rank());

  BCL::barrier();

  if (BCL::rank() == 0) {
    printf("Finished reading kmers.\n");
    fflush(stdout);
  }

  BCL::barrier();

  auto start = std::chrono::high_resolution_clock::now();
  std::vector <kmer_pair> start_nodes;

  for (const auto &kmer : kmers) {
    kmer_hash.insert(kmer.kmer_str(), kmer.fb_ext_str());
    if (kmer.backwardExt() == 'F') {
      start_nodes.push_back(kmer);
    }
  }
  auto end_insert = std::chrono::high_resolution_clock::now();
  BCL::barrier();

  if (BCL::rank() == 0) {
    double insert_time = std::chrono::duration <double> (end_insert - start).count();
    printf("Finished inserting in %lf\n", insert_time);
    fflush(stdout);
  }

  auto start_read = std::chrono::high_resolution_clock::now();

  std::list <std::list <kmer_pair>> contigs;
  for (const auto &start_kmer : start_nodes) {
    std::list <kmer_pair> contig;
    contig.push_back(start_kmer);
    while (contig.back().forwardExt() != 'F') {
      std::string next_kmer = contig.back().next_kmer().get();
      std::string fb_ext = kmer_hash.find(next_kmer);
      contig.push_back(kmer_pair(next_kmer, fb_ext));
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
    printf("Assembled in %lf total\n", total.count());
  }

  BCL::finalize();
  return 0;
}
