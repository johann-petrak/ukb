// -*-C++-*-

#ifndef WALKPRINT_H
#define WALKPRINT_H

#include <string>
#include <vector>
#include <utility> // pair
#include "kbGraph.h"
#include "wdict.h"


namespace ukb {

	// bucket sampling
	struct vsampling_t {

		vsampling_t(size_t buckets = 10);
		int sample();
		void debug();

		std::vector<int> m_idx;
		size_t m_N; // size of graph
		size_t m_bucket_N;
		std::vector<std::pair<int, int> > m_intervals;

	};

	// Walk&Print class
	class Wap {

	public:

		Wap(size_t n, size_t bucket_size) : m_n(n), m_bsize(bucket_size), m_vsampler(bucket_size), m_i(0) {}
		~Wap() {};

		// perform an iteration leaving the result context in C
		// return false if iteration is over

		bool next(std::vector<std::string> & C);

	private:

		size_t m_n;     // number of contexts to produce
		size_t m_bsize; // bucket size
		vsampling_t m_vsampler; // sampling from buckets
		size_t m_i;     // number of context produced so far
	};


	// Deepwalk algorithm
	class DeepWalk {
	public:
		DeepWalk(size_t gamma, size_t t);
		~DeepWalk() {}

		// perform an iteration leaving the result context in C
		// return false if iteration is over

		bool next(std::vector<std::string> & C);

	private:
		size_t m_N;     // number of graph vertices
		size_t m_i;     // number of context produced so far
		float m_gamma;  // number of rw per vertex
		float m_g;      // number of gamma iterations so far
		size_t m_t;     // context size
	};



	// Walk&Print class starting from a word
	class WapWord {

	public:

		WapWord(std::string & hw, size_t n) : m_seed(hw), m_n(n), m_i(0), m_synsets(WDict::instance().get_entries(hw)) {}
		~WapWord() {};

		// perform an iteration leaving the result context in C
		// return false if iteration is over

		bool next(std::vector<std::string> & C);

	private:

		std::string & m_seed;     // seed word
		size_t m_n;     // number of contexts to produce
		size_t m_i;     // number of context produced so far
		WDict_entries m_synsets;
	};

	// Walk&Print starting from a word
	void wap_do_mc_word(const std::string & hw, size_t n);

}


#endif
