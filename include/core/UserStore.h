//
// Created by Benjamin Toro Leddihn on 21/02/26.
//

#ifndef INC_1_USERSTORE_H
#define INC_1_USERSTORE_H

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include "./User.h"
#include "../text/TextUtils.h"



class UserStore {
public:

    static constexpr int MAX_USERS = 4;

    bool load(const std::string& path) {
        std::ifstream in(path);
        if (!in) return false;

        reset();

        std::string tok;
        int version = 0;

        if (!(in >> tok) || tok != "VERSION") return false;
        if (!(in >> version) || version != 1) return false;

        if (!(in >> tok) || tok != "SLOTS") return false;
        int slots = 0;
        if (!(in >> slots) || slots != MAX_USERS) return false;

        while (in >> tok) {
            if (tok != "SLOT") return false;
            int slot = -1;
            if (!(in >> slot) || slot < 0 || slot >= MAX_USERS) return false;

            if (!(in >> tok) || tok != "USED") return false;
            int usedFlag = 0;
            if (!(in >> usedFlag)) return false;

            used_[slot] = (usedFlag == 1);
            users_[slot] = User{};

            if (used_[slot]) {
                if (!(in >> tok) || tok != "NAME") return false;
                if (!(in >> std::quoted(users_[slot].name))) return false;

                if (!(in >> tok) || tok != "LIKED") return false;
                size_t nLiked = 0;
                if (!(in >> nLiked)) return false;
                for (size_t i = 0; i < nLiked; i++) {
                    std::string id; if (!(in >> id)) return false;
                    users_[slot].liked.insert(id);
                }

                if (!(in >> tok) || tok != "WATCH") return false;
                size_t nWatch = 0;
                if (!(in >> nWatch)) return false;
                users_[slot].watch_later.reserve(nWatch);
                for (size_t i = 0; i < nWatch; i++) {
                    std::string id; if (!(in >> id)) return false;
                    users_[slot].watch_later.push_back(id);
                }
            }

            if (!(in >> tok) || tok != "END") return false;
        }
        return true;
    }

    bool save(const std::string& path) const {
        std::ofstream out(path);
        if (!out) return false;

        out << "VERSION 1\n";
        out << "SLOTS " << MAX_USERS << "\n";

        for (int i = 0; i < MAX_USERS; i++) {
            out << "SLOT " << i << " USED " << (used_[i] ? 1 : 0) << "\n";
            if (used_[i]) {
                out << "NAME " << std::quoted(users_[i].name) << "\n";

                std::vector<std::string> liked_sorted(users_[i].liked.begin(), users_[i].liked.end());
                std::sort(liked_sorted.begin(), liked_sorted.end());
                out << "LIKED " << liked_sorted.size();
                for (auto& id : liked_sorted) out << " " << id;
                out << "\n";

                out << "WATCH " << users_[i].watch_later.size();
                for (auto& id : users_[i].watch_later) out << " " << id;
                out << "\n";
            }
            out << "END\n";
        }
        return true;
    }

    bool has(int slot) const { return slot >= 0 && slot < MAX_USERS && used_[slot]; }
    bool canCreate() const { return countUsers() < MAX_USERS; }

    int countUsers() const {
        int c = 0;
        for (bool b : used_) c += (b ? 1 : 0);
        return c;
    }

    void create(int slot, std::string name) {
        if (slot < 0 || slot >= MAX_USERS) return;
        if (!canCreate()) return;

        text::trim_in_place(name);
        if (name.empty()) name = "Usuario" + std::to_string(countUsers() + 1);

        users_[slot] = User{std::move(name), {}, {}};
        used_[slot] = true;
    }

    void remove(int slot) {
        if (slot < 0 || slot >= MAX_USERS) return;
        users_[slot] = User{};
        used_[slot] = false;
    }

    User& get(int slot) { return users_.at(slot); }
    const User& get(int slot) const { return users_.at(slot); }

    struct Memento {
        std::array<User, MAX_USERS> users;
        std::array<bool, MAX_USERS> used;
    };

    Memento snapshot() const { return Memento{users_, used_}; }
    void restore(const Memento& m) { users_ = m.users; used_ = m.used; }
private:
    std::array<User, MAX_USERS> users_{};
    std::array<bool, MAX_USERS> used_{false, false, false, false};

    void reset() {
        for (int i = 0; i < MAX_USERS; i++) { users_[i] = User{}; used_[i] = false; }
    }
};




#endif //INC_1_USERSTORE_H