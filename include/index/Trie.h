//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#ifndef INC_1_TRIE_H
#define INC_1_TRIE_H


#include <array>
#include <string>
#include <vector>

class Trie {
public:
    Trie() { nodes_.push_back(Node{}); }

    int insertAndGetId(const std::string& key) {
        int cur = 0;
        for (unsigned char ch : key) {
            if (ch >= 128) continue;
            int& nxt = nodes_[cur].next[ch];
            if (nxt == -1) {
                nxt = (int)nodes_.size();
                nodes_.push_back(Node{});
            }
            cur = nxt;
        }
        if (nodes_[cur].term_id == -1) nodes_[cur].term_id = next_term_id_++;
        return nodes_[cur].term_id;
    }

    int findId(const std::string& key) const {
        int cur = 0;
        for (unsigned char ch : key) {
            if (ch >= 128) return -1;
            int nxt = nodes_[cur].next[ch];
            if (nxt == -1) return -1;
            cur = nxt;
        }
        return nodes_[cur].term_id;
    }

    int termCount() const { return next_term_id_; }

private:
    struct Node {
        std::array<int, 128> next;
        int term_id = -1;
        Node() { next.fill(-1); }
    };

    std::vector<Node> nodes_;
    int next_term_id_ = 0;
};

#endif //INC_1_TRIE_H