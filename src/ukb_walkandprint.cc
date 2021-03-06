#include "globalVars.h"
#include "walkandprint.h"
#include <string>
#include <iostream>
#include <fstream>

/*
  #include "wdict.h"
  #include "common.h"
  #include "kbGraph.h"
  #include "disambGraph.h"
*/

// Basename & friends
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

// Program options

#include <boost/program_options.hpp>

using namespace ukb;
using namespace std;
using namespace boost;

static void print_ctx(const vector<string> & emited_words) {

	if(emited_words.size() > 1) {
		vector<string>::const_iterator it = emited_words.begin();
		vector<string>::const_iterator end = emited_words.end();
		if (it != end) {
			--end;
			for(;it != end; ++it) {
				cout << *it << " ";
			}
			cout << *end << "\n";
		}
	}
}

int main(int argc, char *argv[]) {

	string kb_binfile("");

	string cmdline("!! -v ");
	cmdline += glVars::ukb_version;
	for (int i=0; i < argc; ++i) {
		cmdline += " ";
		cmdline += argv[i];
	}

	vector<string> input_files;
	string N_str;
	size_t N;
	ifstream input_ifs;

	glVars::input::filter_pos = false;
	size_t opt_bucket_size = 0;
	bool opt_deepwalk = false;
	size_t opt_deepwalk_gamma = 80; // as for (Perozzi et al., 2014)
	size_t opt_deepwalk_t = 10; // as for (Perozzi et al., 2014)

	using namespace boost::program_options;

	const char desc_header[] = "ukb_walkandprint: generate contexts for creating neural network embeddings\n"
		"Usage examples:\n"
		"ukb_embedding -D dict.txt -K graph.bin --dict_weigth 10000 -> Create context for 10000 random walks\n"

		"Options";

	//options_description po_desc(desc_header);

	options_description po_desc("General");
	string seed_word;

	po_desc.add_options()
		("help,h", "This page")
		("version", "Show version.")
		("verbose,v", "Be verbose.")
		("kb_binfile,K", value<string>(), "Binary file of KB (see compile_kb).")
		("dict_file,D", value<string>(), "Dictionary text file.")
		;

	options_description po_desc_waprint("walk and print options");
	po_desc_waprint.add_options()
		("deepwalk", "Use deepwalk algorithm.")
		("deepwalk_walks", value<size_t>(), "Deekwalk walks per vertex (gamma).")
		("deepwalk_length", value<size_t>(), "Deekwalk walk length (t).")
		("vsample", "Sample vertices according to static prank.")
		("buckets", value<size_t>(), "Number of buckets used in vertex sampling (default is 10).")
		("wemit_prob", value<float>(), "Probability to emit a word when on an vertex (emit vertex name instead). Default is 1.0 (always emit word).")
		("seed_word", value<string>(), "Select concepts associate to the word to start the random walk. Default is start at any vertex at random.")
		;

	options_description po_desc_prank("pageRank general options");
	po_desc_prank.add_options()
		("prank_damping", value<float>(), "Set damping factor in PageRank equation. Default is 0.85.")
		;

	options_description po_desc_dict("Dictionary options");
	po_desc_dict.add_options()
		("dict_weight", "Use weights when linking words to concepts (dict file has to have weights).")
		("smooth_dict_weight", value<float>(), "Smoothing factor to be added to every weight in dictionary concepts. Default is 1.")
		("dict_strict", "Be strict when reading the dictionary and stop when any error is found.")
		;

	options_description po_visible(desc_header);
	po_visible.add(po_desc).add(po_desc_waprint).add(po_desc_prank).add(po_desc_dict);

	options_description po_hidden("Hidden");
	po_hidden.add_options()
		("test,t", "(Internal) Do a test.")
		("N",value<string>(), "Number of RW.")
		;
	options_description po_all("All options");
	po_all.add(po_visible).add(po_hidden);

	positional_options_description po_optdesc;
	po_optdesc.add("N", 1);
	//    po_optdesc.add("output-file", 1);

	try {

		variables_map vm;
		store(command_line_parser(argc, argv).
			  options(po_all).
			  positional(po_optdesc).
			  run(), vm);

		notify(vm);

		// If asked for help, don't do anything more

		if (vm.count("help")) {
			cout << po_visible << endl;
			exit(0);
		}

		if (vm.count("version")) {
			cout << glVars::ukb_version << endl;
			exit(0);
		}

		if (vm.count("wemit_prob")) {
			glVars::wap::wemit_prob = vm["wemit_prob"].as<float>();
			if (glVars::wap::wemit_prob < 0.0f || glVars::wap::wemit_prob > 1.0f) {
				cerr << "Error: invalid wemit_prob value:" << glVars::wap::wemit_prob << "\n";
				exit(1);
			}
		}

		if (vm.count("prank_damping")) {
			float dp = vm["prank_damping"].as<float>();
			if (dp <= 0.0 || dp > 1.0) {
				cerr << "Error: invalid prank_damping value " << dp << "\n";
				exit(1);
			}
			glVars::prank::damping = dp;
		}

		if (vm.count("dict_file")) {
			glVars::dict::text_fname = vm["dict_file"].as<string>();
		}

		if (vm.count("dict_strict")) {
			glVars::dict::swallow = false;
		}

		if (vm.count("dict_weight")) {
			glVars::dict::use_weight = true;
		}

		if (vm.count("smooth_dict_weight")) {
			glVars::dict::weight_smoothfactor = vm["smooth_dict_weight"].as<float>();
		}

		if (vm.count("deepwalk")) {
			opt_deepwalk = true;
		}

		if (vm.count("deepwalk_walks")) {
			opt_deepwalk_gamma = vm["deepwalk_walks"].as<size_t>();
			if (!opt_deepwalk_gamma) {
				cerr << "Error: --deepwalk_walks has to be possitive.\n";
				exit(1);
			}
		}

		if (vm.count("deepwalk_length")) {
			opt_deepwalk_t = vm["deepwalk_length"].as<size_t>();
			if (!opt_deepwalk_t) {
				cerr << "Error: --deepwalk_length has to be possitive.\n";
				exit(1);
			}
		}

		if (vm.count("vsample")) {
			opt_bucket_size = 10 ; // default value
		}

		if (vm.count("buckets")) {
			opt_bucket_size = vm["buckets"].as<size_t>();
		}

		if (vm.count("kb_binfile")) {
			kb_binfile = vm["kb_binfile"].as<string>();
		}

		if (vm.count("seed_word")) {
			seed_word = vm["word"].as<string>();
		}

		if (vm.count("N")) {
			N_str = vm["N"].as<string>();
		}

		if (vm.count("verbose")) {
			glVars::verbose = 1;
			glVars::debug::warning = 1;
		}

		if (vm.count("test")) {
			//opt_do_test = true;
		}
	} catch(std::exception& e) {
		cerr << e.what() << "\n";
		exit(-1);
	}

	glVars::rnd::init_random_device();

	vector<string> ctx;

	if(opt_deepwalk) {
		Kb::create_from_binfile(kb_binfile);
		cout << cmdline << "\n";
		DeepWalk walker(opt_deepwalk_gamma, opt_deepwalk_t);
		while(walker.next(ctx))
			print_ctx(ctx);
		exit(0);
	}

	Kb::create_from_binfile(kb_binfile);
	cout << cmdline << "\n";

	if (!N_str.size()) {
		cout << po_visible << endl;
		cout << "Please specify the number of random walks" << endl;
		exit(-1);
	}
	N = lexical_cast<size_t>(N_str);

	if (seed_word.size()) {
		WapWord walker(seed_word, N);
		while(walker.next(ctx)) {
			if(ctx.size() > 1) {
				cout << seed_word << "\t";
				print_ctx(ctx);
			}
		}
	} else {
		Wap walker(N, opt_bucket_size);
		while(walker.next(ctx))
			print_ctx(ctx);
	}

}
