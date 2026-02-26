//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#ifndef INC_1_NGRAMINDEX_H
#define INC_1_NGRAMINDEX_H

#include <algorithm>
#include <string>
#include <vector>
#include "./Trie.h"

class NgramIndex {
public:
    explicit NgramIndex(int n=3) : n_(n) {}

    int n() const { return n_; }

    void addTextCompact(const std::string& compact, int movie_id) {
        if ((int)compact.size() < n_) return;
        for (int i = 0; i + n_ <= (int)compact.size(); i++) {
            std::string ng = compact.substr(i, n_);
            int term_id = trie_.insertAndGetId(ng);
            if ((int)postings_.size() <= term_id) postings_.resize(term_id + 1);
            postings_[term_id].push_back(movie_id);
        }
    }

    void addNgramList(const std::string& ngram, const std::vector<int>& movie_ids) {
        if ((int)ngram.size() != n_) return;
        int term_id = trie_.insertAndGetId(ngram);
        if ((int)postings_.size() <= term_id) postings_.resize(term_id + 1);
        auto& v = postings_[term_id];
        v.insert(v.end(), movie_ids.begin(), movie_ids.end());
    }

    void finalize() {
        for (auto& v : postings_) {
            std::sort(v.begin(), v.end());
            v.erase(std::unique(v.begin(), v.end()), v.end());
        }
    }

    const std::vector<int>* lookup(const std::string& ngram) const {
        int term_id = trie_.findId(ngram);
        if (term_id < 0 || term_id >= (int)postings_.size()) return nullptr;
        return &postings_[term_id];
    }

    std::vector<int> candidatesForCompactQuery(const std::string& q_compact) const {
        if ((int)q_compact.size() < n_) return {};

        std::vector<std::vector<int>> lists;
        for (int i = 0; i + n_ <= (int)q_compact.size(); i++) {
            std::string ng = q_compact.substr(i, n_);
            const auto* lst = lookup(ng);
            if (!lst) return {};
            lists.push_back(*lst);
        }
        if (lists.empty()) return {};

        std::sort(lists.begin(), lists.end(),
                  [](const auto& a, const auto& b){ return a.size() < b.size(); });

        std::vector<int> inter = lists[0];
        for (size_t k = 1; k < lists.size() && !inter.empty(); k++) {
            inter = intersectSorted(inter, lists[k]);
        }
        return inter;
    }

private:
    int n_;
    Trie trie_;
    std::vector<std::vector<int>> postings_;

    static std::vector<int> intersectSorted(const std::vector<int>& a, const std::vector<int>& b) {
        std::vector<int> out;
        out.reserve(std::min(a.size(), b.size()));
        size_t i = 0, j = 0;
        while (i < a.size() && j < b.size()) {
            if (a[i] == b[j]) { out.push_back(a[i]); i++; j++; }
            else if (a[i] < b[j]) i++;
            else j++;
        }
        return out;
    }
};

#endif //INC_1_NGRAMINDEX_H