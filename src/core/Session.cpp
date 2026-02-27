//
// Created by Benjamin Toro Leddihn on 26/02/26.
//

#include "../../include/core/Session.h"


struct Session::LoggedOutState : Session::State {
    static const LoggedOutState& inst() { static LoggedOutState s; return s; }

    bool login(Session& ses, UserStore& users, int slot) override {
        if (slot < 0 || slot >= UserStore::MAX_USERS) return false;
        if (!users.has(slot)) return false;

        ses.users_ = &users;
        ses.slot_  = slot;
        ses.setState(LoggedInState::inst());
        return true;
    }

    void logout(Session& ses) override {
        ses.users_ = nullptr;
        ses.slot_  = -1;
    }

    bool logged() const override { return false; }
};

struct Session::LoggedInState : Session::State {
    static const LoggedInState& inst() { static LoggedInState s; return s; }

    bool login(Session&, UserStore&, int) override {
        // Aquí se cumple la regla: NO se permite 2do login en la misma sesión
        return false;
    }

    void logout(Session& ses) override {
        ses.users_ = nullptr;
        ses.slot_  = -1;
        ses.setState(LoggedOutState::inst());
    }

    bool logged() const override { return true; }
};

Session::Session()
    : state_(&LoggedOutState::inst()), users_(nullptr), slot_(-1) {}

Session& Session::instance() {
    static Session s;
    return s;
}

bool Session::login(UserStore& users, int slot) {
    return state_->login(*this, users, slot);
}

void Session::logout() {
    state_->logout(*this);
}

bool Session::isLoggedIn() const {
    return state_->logged();
}

int Session::activeSlot() const {
    return slot_;
}

User& Session::user() {
    if (!isLoggedIn() || !users_) throw std::runtime_error("No hay usuario logueado.");
    return users_->get(slot_);
}

const User& Session::user() const {
    if (!isLoggedIn() || !users_) throw std::runtime_error("No hay usuario logueado.");
    return users_->get(slot_);
}