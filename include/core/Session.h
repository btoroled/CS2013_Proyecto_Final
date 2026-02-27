//
// Created by Benjamin Toro Leddihn on 26/02/26.
//

#ifndef SESSION_H
#define SESSION_H

#include <stdexcept>
#include "./UserStore.h"

class Session final {
public:
    static Session& instance();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // Regla del enunciado: si ya hay usuario activo, login debe FALLAR
    bool login(UserStore& users, int slot);
    void logout();

    bool isLoggedIn() const;
    int activeSlot() const;

    User& user();
    const User& user() const;

private:
    Session(); // singleton

    struct State {
        virtual ~State() = default;
        virtual bool login(Session&, UserStore&, int) = 0;
        virtual void logout(Session&) = 0;
        virtual bool logged() const = 0;
    };

    struct LoggedOutState;
    struct LoggedInState;

    const State* state_;
    UserStore* users_;
    int slot_;

    void setState(const State& s) { state_ = &s; }

    friend struct LoggedOutState;
    friend struct LoggedInState;
};

#endif