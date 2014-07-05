/*
 * Copyright (C) 2014 Daniel Nicoletti <dantti12@gmail.com>
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

#ifndef UPLOAD_H
#define UPLOAD_H

#include <QIODevice>
#include <upload_p.h>

namespace Cutelyst {

class UploadPrivate;
class Upload : public QIODevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(Upload)
public:
    Upload(UploadPrivate *prv);

    QString filename() const;
    QByteArray contentType() const;

    bool save(const QString &filename);

    virtual qint64 pos() const;
    virtual qint64 size() const;
    virtual bool seek(qint64 pos);

protected:
    virtual qint64 readData(char *data, qint64 maxlen);
    virtual qint64 readLineData(char *data, qint64 maxlen);
    virtual qint64 writeData(const char * data, qint64 maxSize);

    UploadPrivate *d_ptr;
};

typedef QList<Upload *> Uploads;

}

#endif // UPLOAD_H