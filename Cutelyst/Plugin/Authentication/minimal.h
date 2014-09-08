/*
 * Copyright (C) 2013-2014 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef AUTHENTICATION_STORE_MINIMAL_H
#define AUTHENTICATION_STORE_MINIMAL_H

#include <Cutelyst/Plugin/plugin.h>
#include <Cutelyst/Plugin/authentication.h>

namespace Cutelyst {

namespace Plugin {

class StoreMinimal : public Authentication::Store
{
public:
    StoreMinimal();

    void addUser(const Authentication::User &user);

    Authentication::User findUser(Context *ctx, const CStringHash &userInfo);

    virtual QVariant forSession(Context *ctx, const Authentication::User &user);

    virtual Authentication::User fromSession(Context *ctx, const QVariant &frozenUser);

private:
    QList<Authentication::User> m_users;
};

} // namespace CutelystPlugin

}

#endif // AUTHENTICATION_STORE_MINIMAL_H
