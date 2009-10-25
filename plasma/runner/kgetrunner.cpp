/*
 *   This file is part of the KDE project.
 *
 *   Copyright (C) 2009 Tomas Van Verrewegen <tomasvanverrewegen@telenet.be>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published
 *   by the Free Software Foundation; either version 2 of the License,
 *   or (at your option) any later version.
 */

#include "kgetrunner.h"
#include "kgetrunner_protocols.h"
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusConnectionInterface>
#include <QTimer>
#include <KNotification>
#include <KIconLoader>


KGetRunner::KGetRunner(QObject* parent, const QVariantList& args)
    : Plasma::AbstractRunner(parent, args), m_icon("kget")
{
    setObjectName("KGet");
    addSyntax(Plasma::RunnerSyntax(":q:", i18n("Find all links in :q: and download them with KGet.")));
    reloadConfiguration();
}


KGetRunner::~KGetRunner()
{
}


void KGetRunner::reloadConfiguration()
{
    //  For now, the list of protocols is configurable, but this will change when KGet offers
    //  some kind of supportedProtocols() call.
    m_protocols = config().readEntry("protocols", KGETRUNNER_PROTOCOLS).split(" ", QString::SkipEmptyParts);
}


void KGetRunner::match(Plasma::RunnerContext& context)
{
    QString query = context.query();
    m_urls = parseUrls(context.query());
    if(!m_urls.isEmpty()) {
        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::PossibleMatch);
        match.setRelevance(0.9);
        match.setIcon(m_icon);
        if(m_urls.size() == 1) {
            match.setText(i18n("Download %1 with KGet.", KUrl(m_urls.first()).prettyUrl()));
        }
        else {
            match.setText(i18np("Download %1 link with KGet.", "Download %1 links with KGet.", m_urls.size()));
        }
        context.addMatch(query, match);
    }
}


void KGetRunner::run(const Plasma::RunnerContext& /*context*/, const Plasma::QueryMatch& /*match*/)
{
    QDBusConnectionInterface* connection = QDBusConnection::sessionBus().interface();
    if(connection->isServiceRegistered("org.kde.kget.main"))  {
        //  KGet is running. Make the call immediately.
        showNewTransferDialog();
        return;
    }
    
    //  KGet is not running. Ask DBus to start it.
    connection->startService("org.kde.kget.main");
    if(connection->lastError().type() != QDBusError::NoError) {
        KNotification::event(KNotification::Error,
                i18n("<p>KGet Runner could not communicate with KGet.</p><p style=\"font-size: small;\">Response from DBus:<br/>%1</p>", connection->lastError().message()),
                KIcon("dialog-warning").pixmap(KIconLoader::SizeSmall)/*, 0, KNotification::Persistant*/);
        return;
    }
    
    //  Set a timer to make the call when KGet has been started.
    //  This might still fail if it takes too long to start KGet.
    //  For me, the 1000ms delay is mooooore than sufficient.
    QTimer::singleShot(1000, this, SLOT(showNewTransferDialog()));
}

void KGetRunner::showNewTransferDialog()
{
    QDBusInterface iface("org.kde.kget", "/KGet", "org.kde.kget.main");
    
    QDBusPendingCall call = iface.asyncCall("showNewTransferDialog", m_urls);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(callFinished(QDBusPendingCallWatcher*)));
                      
    m_urls.clear();
}

void KGetRunner::callFinished(QDBusPendingCallWatcher* call)
{
    QDBusPendingReply<> reply = *call;
    
    //  TODO Remove the check for QDBusError::NoReply when NewTransferDialog is fixed to show asynchronously.
    if(!reply.isValid() && (reply.error().type() != QDBusError::NoReply)) {
        //  Send a notification about the error to the user.
        KNotification::event(KNotification::Error,
                i18n("<p>KGet Runner could not communicate with KGet.</p><p style=\"font-size: small;\">Response from DBus:<br/>%1</p>", reply.error().message()),
                KIcon("dialog-warning").pixmap(KIconLoader::SizeSmall)/*, 0, KNotification::Persistant*/);
    }
}

QStringList KGetRunner::parseUrls(const QString& text) const
{
    QStringList urls;
    //  We could use QString::split() or similar, but a simple split on whitespace will
    //  keep us from finding many possible matches (eg: "http://... or =http://...).
    //  So, for now, let's traverse the query with a simple regexp.
    QRegExp re("\\b\\S+");
    int i = re.indexIn(text);
    while(i != -1) {
        //  We check if the match is a valid URL, if the protocol is handled by KGet,
        //  and if the host is not empty, otherwise "http://" would also be matched.
        KUrl url(re.cap());
        if(url.isValid() && url.hasHost() && m_protocols.contains(url.protocol())) {
            urls << url.url();
            
            //  continue searching after last match...
            i = re.indexIn(text, i + re.matchedLength());
        }
        else {
            //  if the match is not a URL, continue searching from next character...
            i = re.indexIn(text, i + 1);
        }
    }
    return urls;
}


#include "kgetrunner.moc"
