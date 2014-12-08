/*
 * Copyright (C) 2013 Daniel Nicoletti <dantti12@gmail.com>
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

#include "authentication_p.h"

#include "authenticationstore.h"
#include "context.h"
#include "session.h"

#include <QLoggingCategory>

using namespace Cutelyst;

Q_LOGGING_CATEGORY(C_AUTHENTICATION, "cutelyst.plugin.authentication")

Authentication::Authentication(QObject *parent) :
    Plugin(parent),
    d_ptr(new AuthenticationPrivate)
{
    qRegisterMetaType<User>();
    qRegisterMetaTypeStreamOperators<User>();
}

Authentication::~Authentication()
{
    delete d_ptr;
}

bool Authentication::setup(Context *ctx)
{
    Q_D(Authentication);
    d->ctx = ctx;
    return true;
}

void Authentication::addRealm(Authentication::Realm *realm)
{
    addRealm(QStringLiteral("default"), realm, true);
}

void Authentication::addRealm(const QString &name, Authentication::Realm *realm, bool defaultRealm)
{
    Q_D(Authentication);
    if (defaultRealm) {
        d->defaultRealm = name;
    }
    d->realms.insert(name, realm);
    d->realmsOrder.append(name);
}

Authentication::User Authentication::authenticate(const QString &username, const QString &password, const QString &realm)
{
    CStringHash userinfo;
    userinfo.insert(QStringLiteral("username"), username);
    userinfo.insert(QStringLiteral("password"), password);
    return authenticate(userinfo, realm);
}

Authentication::User Authentication::authenticate(const CStringHash &userinfo, const QString &realm)
{
    Q_D(Authentication);

    Authentication::Realm *realmPtr = d->realm(realm);
    if (realmPtr) {
        User user = realmPtr->authenticate(d->ctx, userinfo);
        if (!user.isNull()) {
            setAuthenticated(user, realm);
        }

        return user;
    }

    qCWarning(C_AUTHENTICATION) << "Could not find realm" << realm;
    return User();
}

Authentication::User Authentication::findUser(const CStringHash &userinfo, const QString &realm)
{
    Q_D(Authentication);

    Authentication::Realm *realmPtr = d->realm(realm);
    if (realmPtr) {
        return realmPtr->findUser(d->ctx, userinfo);
    }

    qCWarning(C_AUTHENTICATION)   << "Could not find realm" << realm;
    return User();
}

Authentication::User Authentication::user()
{
    Q_D(Authentication);
    QVariant user = pluginProperty(d->ctx, QStringLiteral("user"));
    if (user.isNull()) {        
        return restoreUser(QVariant(), QString());
    }
    return user.value<User>();
}

void Authentication::setUser(const Authentication::User &user)
{
    Q_D(Authentication);
    if (user.isNull()) {
        setPluginProperty(d->ctx, QStringLiteral("user"), QVariant());
    } else {
        setPluginProperty(d->ctx, QStringLiteral("user"), QVariant::fromValue(user));
    }
}

bool Authentication::userExists()
{
    return !user().isNull();
}

bool Authentication::userInRealm(const QString &realm)
{
    Q_D(Authentication);
    QVariant user = pluginProperty(d->ctx, QStringLiteral("user"));
    if (user.isNull()) {
        return !restoreUser(QVariant(), realm).isNull();
    }
    return false;
}

void Authentication::logout()
{
    Q_D(Authentication);
    setUser(User());

    Authentication::Realm *realm = findRealmForPersistedUser();
    if (realm) {
        realm->removePersistedUser(d->ctx);
    }
}

void Authentication::setAuthenticated(const User &user, const QString &realmName)
{
    Q_D(Authentication);

    setUser(user);

    Authentication::Realm *realmPtr = d->realm(realmName);
    if (!realmPtr) {
        qCWarning(C_AUTHENTICATION) << "Called with invalid realm" << realmName;
    }
    // TODO implement a user class
//    $user->auth_realm($realm->name);

    persistUser(user, realmName);
}

void Authentication::persistUser(const User &user, const QString &realmName)
{
    Q_D(Authentication);

    if (userExists()) {
        Session *session = d->ctx->plugin<Session*>();
        if (session && session->isValid()) {
            session->setValue("Authentication::userRealm", realmName);
        }

        Authentication::Realm *realmPtr = d->realm(realmName);
        if (realmPtr) {
            realmPtr->persistUser(d->ctx, user);
        }
    }
}

Authentication::User Authentication::restoreUser(const QVariant &frozenUser, const QString &realmName)
{
    Q_D(Authentication);

    Authentication::Realm *realmPtr = d->realm(realmName);
    if (!realmPtr) {
        realmPtr = findRealmForPersistedUser();
    }

    if (!realmPtr) {
        return User();
    }

    User user = realmPtr->restoreUser(d->ctx, frozenUser);

    setUser(user);
    // TODO
    // $user->auth_realm($realm->name) if $user;

    return user;
}

Authentication::Realm *Authentication::findRealmForPersistedUser()
{
    Q_D(Authentication);

    Authentication::Realm *realm;

    Session *session = d->ctx->plugin<Session*>();
    if (session &&
            session->isValid() &&
            !session->value("Authentication::userRealm").isNull()) {
        QString realmName = session->value("Authentication::userRealm").toString();
        realm = d->realms.value(realmName);
        if (realm && !realm->userIsRestorable(d->ctx).isNull()) {
            return realm;
        }
    } else {
        // we have no choice but to ask each realm whether it has a persisted user.
        Q_FOREACH (const QString &realmName, d->realmsOrder) {
            Authentication::Realm *realm = d->realms.value(realmName);
            if (realm && !realm->userIsRestorable(d->ctx).isNull()) {
                return realm;
            }
        }
    }

    return 0;
}

Authentication::Realm::Realm(AuthenticationStore *store, Authentication::Credential *credential) :
    m_store(store),
    m_credential(credential)
{

}

Authentication::User Authentication::Realm::findUser(Context *ctx, const CStringHash &userinfo)
{
    User ret = m_store->findUser(ctx, userinfo);

    if (ret.isNull()) {
        if (m_store->canAutoCreateUser()) {
            ret = m_store->autoCreateUser(ctx, userinfo);
        }
    } else if (m_store->canAutoUpdateUser()) {
        ret = m_store->autoUpdateUser(ctx, userinfo);
    }

    return ret;
}

Authentication::User Authentication::Realm::authenticate(Context *ctx, const CStringHash &authinfo)
{
    return m_credential->authenticate(ctx, this, authinfo);
}

void Authentication::Realm::removePersistedUser(Context *ctx)
{
    Session *session = ctx->plugin<Session*>();
    if (session && session->isValid()) {
        session->deleteValue("Authentication::user");
        session->deleteValue("Authentication::userRealm");
    }
}

Authentication::User Authentication::Realm::persistUser(Context *ctx, const Authentication::User &user)
{
    Session *session = ctx->plugin<Session*>();
    if (session && session->isValid()) {
        session->setValue("Authentication::user",
                          m_store->forSession(ctx, user));
    }

    return user;
}

Authentication::User Authentication::Realm::restoreUser(Context *ctx, const QVariant &frozenUser)
{
    QVariant _frozenUser = frozenUser;
    if (_frozenUser.isNull()) {
        _frozenUser = userIsRestorable(ctx);
    }

    if (_frozenUser.isNull()) {
        return User();
    }

    User user = m_store->fromSession(ctx, _frozenUser);

    if (!user.isNull()) {
        ctx->plugin<Authentication*>()->setUser(user);

        // Sets the realm the user originated in
        user.setAuthRealm(this);
    } else {
        qCWarning(C_AUTHENTICATION) << "Store claimed to have a restorable user, but restoration failed. Did you change the user's id_field?";
    }

    return user;
}

QVariant Authentication::Realm::userIsRestorable(Context *ctx)
{
    Session *session = ctx->plugin<Session*>();
    if (session && session->isValid()) {
        return session->value(QLatin1String("Authentication::user"));
    }
    return QVariant();
}

Authentication::Realm *AuthenticationPrivate::realm(const QString &realmName) const
{
    QString name = realmName;
    if (name.isNull()) {
        name = defaultRealm;
    }
    return realms.value(name);
}

Authentication::User::User()
{

}

Authentication::User::User(const QString &id) :
    m_id(id)
{

}

Authentication::User::~User()
{
}

QString Authentication::User::id() const
{
    return m_id;
}

void Authentication::User::setId(const QString &id)
{
    m_id = id;
}

bool Authentication::User::isNull() const
{
    return m_id.isNull();
}

Authentication::Realm *Authentication::User::authRealm()
{
    return m_realm;
}

void Authentication::User::setAuthRealm(Authentication::Realm *authRealm)
{
    m_realm = authRealm;
}

bool Authentication::User::checkPassword(const QString &password) const
{
    Q_UNUSED(password)
    return false;
}

QDataStream &operator<<(QDataStream &out, const Authentication::User &user)
{
    out << user.id() << static_cast<CStringHash>(user);
    return out;
}

QDataStream &operator>>(QDataStream &in, Authentication::User &user)
{
    QString id;
    CStringHash hash;
    in >> id >> hash;
    user.setId(id);
    user.swap(hash);
    return in;
}