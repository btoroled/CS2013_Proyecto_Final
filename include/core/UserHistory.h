//
// Created by Benjamin Toro Leddihn on 26/02/26.
//

#ifndef USER_HISTORY_H
#define USER_HISTORY_H

#include <vector>
#include "./UserStore.h"

class UserHistory {
public:
    void clear() { undo_.clear(); redo_.clear(); }

    void checkpoint(const UserStore& store) {
        undo_.push_back(store.snapshot());
        redo_.clear();
        if (undo_.size() > max_) undo_.erase(undo_.begin());
    }

    bool undo(UserStore& store) {
        if (undo_.empty()) return false;
        redo_.push_back(store.snapshot());
        store.restore(undo_.back());
        undo_.pop_back();
        return true;
    }

    bool redo(UserStore& store) {
        if (redo_.empty()) return false;
        undo_.push_back(store.snapshot());
        store.restore(redo_.back());
        redo_.pop_back();
        return true;
    }

private:
    std::vector<UserStore::Memento> undo_;
    std::vector<UserStore::Memento> redo_;
    size_t max_ = 50;
};

#endif