/* Torsion - http://torsionim.org/
 * Copyright (C) 2010, John Brooks <john.brooks@dereferenced.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 *    * Neither the names of the copyright owners nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "main.h"
#include "MainWindow.h"
#include "core/UserIdentity.h"
#include "core/IncomingRequestManager.h"
#include "core/OutgoingContactRequest.h"
#include "core/IdentityManager.h"
#include "tor/TorControlManager.h"
#include "ui/AvatarImageProvider.h"
#include "ContactsModel.h"
#include "ui/ConversationModel.h"
#include <QtQml>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QMessageBox>
#include <QPushButton>

MainWindow *uiMain = 0;

MainWindow::MainWindow(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!uiMain);
    uiMain = this;

    qml = new QQmlApplicationEngine(this);

    qmlRegisterUncreatableType<ContactUser>("org.torsionim.torsion", 1, 0, "ContactUser", QString());
    qmlRegisterUncreatableType<UserIdentity>("org.torsionim.torsion", 1, 0, "UserIdentity", QString());
    qmlRegisterUncreatableType<ContactsManager>("org.torsionim.torsion", 1, 0, "ContactsManager", QString());
    qmlRegisterUncreatableType<IncomingRequestManager>("org.torsionim.torsion", 1, 0, "IncomingRequestManager", QString());
    qmlRegisterUncreatableType<IncomingContactRequest>("org.torsionim.torsion", 1, 0, "IncomingContactRequest", QString());
    qmlRegisterUncreatableType<Tor::TorControlManager>("org.torsionim.torsion", 1, 0, "TorControlManager", QString());
    qmlRegisterType<ConversationModel>("org.torsionim.torsion", 1, 0, "ConversationModel");

    Q_ASSERT(!identityManager->identities().isEmpty());
    qml->rootContext()->setContextProperty(QLatin1String("userIdentity"), identityManager->identities()[0]);
    qml->rootContext()->setContextProperty(QLatin1String("torManager"), torManager);

    qml->addImageProvider(QLatin1String("avatar"), new AvatarImageProvider);

    createContactsModel();

    qml->load(QUrl(QLatin1String("qrc:/ui/main.qml")));

#if 0
    /* Saved geometry */
    restoreGeometry(config->value("ui/main/windowGeometry").toByteArray());

    /* Old config values */
    config->remove("ui/main/windowSize");
    config->remove("ui/main/windowPosition");
#endif

    /* Other things */
    connect(identityManager, SIGNAL(incomingRequestAdded(IncomingContactRequest*,UserIdentity*)), SLOT(updateContactRequests()));
    connect(identityManager, SIGNAL(incomingRequestRemoved(IncomingContactRequest*,UserIdentity*)), SLOT(updateContactRequests()));
    connect(identityManager, SIGNAL(outgoingRequestAdded(OutgoingContactRequest*,UserIdentity*)),
            SLOT(outgoingRequestAdded(OutgoingContactRequest*)));

    foreach (UserIdentity *identity, identityManager->identities())
    {
        foreach (ContactUser *user, identity->contacts.contacts())
        {
            if (user->isContactRequest())
                outgoingRequestAdded(OutgoingContactRequest::requestForUser(user));
        }
    }

    updateContactRequests();

    connect(torManager, SIGNAL(statusChanged(int,int)), this, SLOT(updateTorStatus()));
    updateTorStatus();
}

MainWindow::~MainWindow()
{
}

#if 0
void MainWindow::closeEvent(QCloseEvent *ev)
{
    config->setValue("ui/main/windowGeometry", saveGeometry());
    QWidget::closeEvent(ev);
}
#endif

void MainWindow::createContactsModel()
{
    UserIdentity *identity = identityManager->identities()[0];
    ContactsModel *model = new ContactsModel(identity, this);
    qml->rootContext()->setContextProperty(QLatin1String("contactsModel"), model);
}

void MainWindow::showNotification(const QString &message, QObject *receiver, const char *slot)
{
    qCritical("XXX-UI showNotification is not implemented");
}

void MainWindow::updateContactRequests()
{
    /* This will likely be redesigned to emphasize the identity tied to the request when real
     * support for multiple identities is done. */
    int numRequests = 0;
    foreach (UserIdentity *identity, identityManager->identities())
        numRequests += identity->contacts.incomingRequests.requests().size();

    qCritical("XXX-UI updateContactRequests is not implemented");
}

void MainWindow::showContactRequest()
{
    /* This cannot iterate a list because it is technically possible for that list to change during the loop */
    for (;;)
    {
        IncomingContactRequest *request = 0;
        foreach (UserIdentity *identity, identityManager->identities())
        {
            const QList<IncomingContactRequest*> &reqs = identity->contacts.incomingRequests.requests();
            if (!reqs.isEmpty())
            {
                request = reqs.first();
                break;
            }
        }

        if (!request)
            break;

        qCritical("XXX-UI Would show contact request dialog here");
        //ContactRequestDialog *dialog = new ContactRequestDialog(request, this);

        /* Allow the user a way out of a loop of requests by cancelling */
        //if (dialog->exec() == ContactRequestDialog::Cancelled)
            break;

        /* Accept or reject would remove the request from the list, so it will not come up again. */
    }

    updateContactRequests();
}

void MainWindow::outgoingRequestAdded(OutgoingContactRequest *request)
{
    connect(request, SIGNAL(statusChanged(int,int)), SLOT(updateOutgoingRequest()));
    updateOutgoingRequest(request);
}

void MainWindow::updateOutgoingRequest(OutgoingContactRequest *request)
{
    if (!request && !(request = qobject_cast<OutgoingContactRequest*>(sender())))
        return;

    qCritical("XXX-UI updateOutgoingRequest is not implemented");
}

void MainWindow::updateTorStatus()
{
    qCritical("XXX-UI updateTorStatus is not implemented");
}

void MainWindow::uiRemoveContact(ContactUser *user)
{
    QMessageBox msg;
    msg.setWindowTitle(tr("Remove Contact"));
    msg.setText(tr("Are you sure you want to permanently remove <b>%1</b> from your contacts?").arg(user->nickname().toHtmlEscaped()));
    msg.setIcon(QMessageBox::Question);
    QAbstractButton *deleteBtn = msg.addButton(tr("Remove"), QMessageBox::DestructiveRole);
    msg.addButton(QMessageBox::Cancel);
    msg.setDefaultButton(QMessageBox::Cancel);

    msg.exec();
    if (msg.clickedButton() != deleteBtn)
        return;

    user->deleteContact();
}
