#ifndef MNNCORRECT_AUTOMATICORDER_HPP
#define MNNCORRECT_AUTOMATICORDER_HPP

#include "utils.hpp"
#include "knncolle/knncolle.hpp"
#include "find_mutual_nns.hpp"
#include "fuse_nn_results.hpp"
#include "correct_target.hpp"
#include "ReferencePolicy.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <memory>
#include <vector>

namespace mnncorrect {

template<typename Float>
Float compute_total_variance(int nd, size_t n, const Float* ptr, bool as_rss) {
    std::vector<Float> mean(nd);
    Float total = 0;
    for (size_t i = 0; i < n; ++i) {
        auto mIt = mean.begin();
        for (int d = 0; d < nd; ++d, ++mIt, ++ptr) {
            const double delta=*ptr - *mIt;
            *mIt += delta/(i + 1);
            total += delta*(*ptr - *mIt);
        }
    }
    if (!as_rss) {
        total /= n - 1;
    }
    return total;
}

template<typename Float>
std::vector<Float> compute_total_variance(int nd, const std::vector<size_t>& no, const std::vector<const Float*>& batch, bool as_rss) {
    std::vector<Float> vars(no.size());
    for (size_t b = 0; b < no.size(); ++b) {
        vars[b] = compute_total_variance<Float>(nd, no[b], batch[b], as_rss);
    }
    return vars;
}

template<typename Index, typename Float, class Builder>
class AutomaticOrder {
public:
    AutomaticOrder(int nd, std::vector<size_t> no, std::vector<const Float*> b, Float* c, Builder bfun, int k, ReferencePolicy first) :
        ndim(nd), 
        nobs(std::move(no)), 
        batches(std::move(b)),
        builder(bfun),
        num_neighbors(k),
        indices(batches.size()),
        neighbors_ref(batches.size()), 
        neighbors_target(batches.size()), 
        corrected(c)
    {
        if (nobs.size() != batches.size()) {
            throw std::runtime_error("length of 'no' and 'b' must be equal");
        }
        if (!nobs.size()) {
            return;
        }

#ifndef MNNCORRECT_CUSTOM_PARALLEL
        #pragma omp parallel for
        for (size_t b = 0; b < nobs.size(); ++b) {
#else
        MNNCORRECT_CUSTOM_PARALLEL(nobs.size(), [&](size_t start, size_t end) -> void {
        for (size_t b = start; b < end; ++b) {
#endif

            indices[b] = bfun(ndim, nobs[b], batches[b]);

#ifndef MNNCORRECT_CUSTOM_PARALLEL
        }
#else
        }
        });
#endif

        // Different policies to pick the first batch. The default is to use
        // the first input batch, so first == Input is already covered.
        size_t ref = 0;
        if (first == MaxSize) {
            ref = std::max_element(nobs.begin(), nobs.end()) - nobs.begin();
        } else if (first == MaxVariance) {
            auto vars = compute_total_variance(ndim, nobs, batches, false);
            ref = std::max_element(vars.begin(), vars.end()) - vars.begin();
        } else if (first == MaxRss) {
            auto vars = compute_total_variance(ndim, nobs, batches, true);
            ref = std::max_element(vars.begin(), vars.end()) - vars.begin();
        }

        const size_t rnum = nobs[ref];
        const Float* rdata = batches[ref];
        std::copy(rdata, rdata + ndim * rnum, corrected);
        ncorrected += rnum;
        order.push_back(ref);

        for (size_t b = 0; b < nobs.size(); ++b) {
            if (b == ref) {
                continue;
            }
            remaining.insert(b);
            neighbors_target[b] = quick_find_nns(nobs[b], batches[b], indices[ref].get(), num_neighbors);
            neighbors_ref[b] = quick_find_nns(rnum, rdata, indices[b].get(), num_neighbors);
        }
        return;
    }

protected:
    void update(size_t latest) {
        size_t lnum = nobs[latest]; 
        const Float* ldata = corrected + ncorrected * ndim;

        order.push_back(latest);
        auto previous_ncorrected = ncorrected;
        ncorrected += lnum;

        remaining.erase(latest);
        if (remaining.empty()) {
            return;
        }

        // Updating all statistics with the latest batch added to the corrected reference.
        indices[latest] = builder(ndim, lnum, ldata);
        const auto& lindex = indices[latest];

        for (auto b : remaining){
            auto& rneighbors = neighbors_ref[b];
            rneighbors.resize(ncorrected);
            const auto& tindex = indices[b];

#ifndef MNNCORRECT_CUSTOM_PARALLEL
            #pragma omp parallel for
            for (size_t l = 0; l < lnum; ++l) {
#else
            MNNCORRECT_CUSTOM_PARALLEL(lnum, [&](size_t start, size_t end) -> void {
            for (size_t l = start; l < end; ++l) {
#endif

                rneighbors[previous_ncorrected + l] = tindex->find_nearest_neighbors(ldata + ndim * l, num_neighbors);

#ifndef MNNCORRECT_CUSTOM_PARALLEL
            }
#else
            }
            });
#endif 

            const size_t tnum = nobs[b];
            const Float* tdata = batches[b];
            auto& tneighbors = neighbors_target[b];

#ifndef MNNCORRECT_CUSTOM_PARALLEL
            #pragma omp parallel for
            for (size_t t = 0; t < tnum; ++t) {
#else
            MNNCORRECT_CUSTOM_PARALLEL(tnum, [&](size_t start, size_t end) -> void {
            for (size_t t = start; t < end; ++t) {
#endif

                auto alt = lindex->find_nearest_neighbors(tdata + ndim * t, num_neighbors);
                fuse_nn_results(tneighbors[t], alt, num_neighbors, static_cast<Index>(previous_ncorrected));

#ifndef MNNCORRECT_CUSTOM_PARALLEL
            }
#else
            }
            });
#endif
        }

        return;
    }

    std::pair<size_t, MnnPairs<Index> > choose() const {
        MnnPairs<Index> output;
        size_t chosen = 0;
        for (auto b : remaining) {
            auto tmp = find_mutual_nns(neighbors_ref[b], neighbors_target[b]);
            if (tmp.num_pairs > output.num_pairs) {
                output = std::move(tmp);
                chosen = b;
            }
        }
        return std::pair<size_t, MnnPairs<Index> >(chosen, std::move(output));
    }

public:
    void run(Float nmads, int robust_iterations, double robust_trim) {
        while (remaining.size()) {
            auto output = choose();
            auto target = output.first;
            auto tnum = nobs[target];
            auto tdata = batches[target];

            correct_target(
                ndim, 
                ncorrected, 
                corrected, 
                tnum, 
                tdata, 
                output.second, 
                builder,
                num_neighbors,
                nmads,
                robust_iterations,
                robust_trim,
                corrected + ncorrected * ndim);

            update(output.first);
            num_pairs.push_back(output.second.num_pairs);
        }
    }

    const auto& get_order() const { return order; }

    const auto& get_num_pairs() const { return num_pairs; }

protected:
    int ndim;
    std::vector<size_t> nobs;
    std::vector<const Float*> batches;
    std::vector<std::shared_ptr<knncolle::Base<Index, Float> > > indices;

    Builder builder;
    int num_neighbors;
    std::vector<NeighborSet<Index, Float> > neighbors_ref;
    std::vector<NeighborSet<Index, Float> > neighbors_target;

    Float* corrected;
    size_t ncorrected = 0;
    std::vector<int> order;
    std::vector<int> num_pairs;

    std::set<size_t> remaining;
};

}

#endif
