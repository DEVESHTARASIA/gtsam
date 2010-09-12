/**
 * @file    GaussianISAM2
 * @brief   Full non-linear ISAM
 * @author  Michael Kaess
 */

#include <gtsam/slam/GaussianISAM2.h>

using namespace std;
using namespace gtsam;

// Explicitly instantiate so we don't have to include everywhere
#include <gtsam/inference/ISAM2-inl.h>

template class ISAM2<GaussianConditional, simulated2D::Config>;
template class ISAM2<GaussianConditional, planarSLAM::Config>;

namespace gtsam {

/* ************************************************************************* */
void optimize2(const GaussianISAM2::sharedClique& clique, double threshold,
		set<Symbol>& changed, const set<Symbol>& replaced, VectorConfig& delta) {
	// if none of the variables in this clique (frontal and separator!) changed
	// significantly, then by the running intersection property, none of the
	// cliques in the children need to be processed
	bool process_children = false;

	// parents are assumed to already be solved and available in result
	GaussianISAM2::Clique::const_reverse_iterator it;
	for (it = clique->rbegin(); it!=clique->rend(); it++) {
		GaussianConditional::shared_ptr cg = *it;

		// is this variable part of the top of the tree that has been redone?
		bool redo = (replaced.find(cg->key()) != replaced.end());

		// only solve if at least one of the separator variables changed
		// significantly, ie. is in the set "changed"
		bool found = true;
		if (!redo && cg->nrParents()>0) {
			found = false;
			BOOST_FOREACH(const Symbol& key, cg->parents()) {
				if (changed.find(key)!=changed.end()) {
					found = true;
				}
			}
		}
		if (found) {
			// Solve for that variable
			Vector d = cg->solve(delta);
			// have to process children; only if none of the variables in the
			// clique were affected, and none of the variables in the clique
			// had a variable in the separator that changed significantly
			// can we be sure that the subtree is not affected
			process_children = true;

			// we change the delta unconditionally if redo, otherwise
			// conditioned on the change being above the threshold
			if (!redo) {
				// change is measured against the previous delta!
				if (delta.contains(cg->key())) {
					Vector d_old = delta[cg->key()];
					if (max(abs(d-d_old)) >= threshold) {
						redo = true;
					}
				} else {
					redo = true; // never created before, so we simply add it
				}
			}

			// replace current entry in delta vector
			if (redo) {
				changed.insert(cg->key());
				if (delta.contains(cg->key())) {
					delta[cg->key()] = d; // replace existing entry
				} else {
					delta.insert(cg->key(), d); // insert new entry
				}
			}

		}
	}
	if (process_children) {
		BOOST_FOREACH(const GaussianISAM2::sharedClique& child, clique->children_) {
			optimize2(child, threshold, changed, replaced, delta);
		}
	}
}

/* ************************************************************************* */
// fast full version without threshold
void optimize2(const GaussianISAM2::sharedClique& clique, boost::shared_ptr<VectorConfig> delta) {
	// parents are assumed to already be solved and available in result
	GaussianISAM2::Clique::const_reverse_iterator it;
	for (it = clique->rbegin(); it!=clique->rend(); it++) {
		GaussianConditional::shared_ptr cg = *it;
		Vector d = cg->solve(*delta);
		// store result in partial solution
		delta->insert(cg->key(), d);
	}
	BOOST_FOREACH(const GaussianISAM2::sharedClique& child, clique->children_) {
		optimize2(child, delta);
	}
}

/* ************************************************************************* */
boost::shared_ptr<VectorConfig> optimize2(const GaussianISAM2::sharedClique& root) {
	boost::shared_ptr<VectorConfig> delta(new VectorConfig);
	set<Symbol> changed;
	// starting from the root, call optimize on each conditional
	optimize2(root, delta);
	return delta;
}

/* ************************************************************************* */
void optimize2(const GaussianISAM2::sharedClique& root, double threshold, const set<Symbol>& keys, VectorConfig& delta) {
	set<Symbol> changed;
	// starting from the root, call optimize on each conditional
	optimize2(root, threshold, changed, keys, delta);
}

/* ************************************************************************* */
void nnz_internal(const GaussianISAM2::sharedClique& clique, int& result) {
	// go through the conditionals of this clique
	GaussianISAM2::Clique::const_reverse_iterator it;
	for (it = clique->rbegin(); it!=clique->rend(); it++) {
		GaussianConditional::shared_ptr cg = *it;
		int dimSep = 0;
		for (GaussianConditional::const_iterator matrix_it = cg->parentsBegin(); matrix_it != cg->parentsEnd(); matrix_it++) {
			dimSep += matrix_it->second.size2();
		}
		int dimR = cg->dim();
		result += ((dimR+1)*dimR)/2 + dimSep*dimR;
	}
	// traverse the children
	BOOST_FOREACH(const GaussianISAM2::sharedClique& child, clique->children_) {
		nnz_internal(child, result);
	}
}

/* ************************************************************************* */
int calculate_nnz(const GaussianISAM2::sharedClique& clique) {
	int result = 0;
	// starting from the root, add up entries of frontal and conditional matrices of each conditional
	nnz_internal(clique, result);
	return result;
}

} /// namespace gtsam
