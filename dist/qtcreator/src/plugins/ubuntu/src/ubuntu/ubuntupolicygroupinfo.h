/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 */

#ifndef UBUNTUPOLICYGROUPINFO_H
#define UBUNTUPOLICYGROUPINFO_H

#include <QObject>
#include "ubuntuprocess.h"

namespace Ubuntu {
namespace Internal {

class UbuntuPolicyGroupInfo : public QObject
{
    Q_OBJECT

public:
    explicit UbuntuPolicyGroupInfo(QObject *parent = 0);
    QString info() { return m_replies; }
    bool isLocal() { return m_bLocal; }

public slots:
    void getInfo(const QString &, const QString &policyVersion);
    void onMessage(QString);
    void onFinished(QString, int);

signals:
    void infoReady(bool);

protected:
    UbuntuProcess m_process;
    QString m_replies;
    QString m_policyGroup;
    QString m_policyVersion;
    bool m_bLocal;

};

}
}

#endif // UBUNTUPOLICYGROUPINFO_H
