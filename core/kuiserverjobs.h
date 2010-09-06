/* This file is part of the KDE project

   Copyright (C) 2007 by Javier Goday <jgoday@gmail.com>
   Copyright (C) 2009 by Dario Massarin <nekkar@libero.it>
   Copyright (C) 2010 by Matthias Fuchs <mat69@gmx.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

#ifndef KUISERVERJOBS_H
#define KUISERVERJOBS_H

#include "kgetglobaljob.h"
#include "transfer.h"

#include <kio/job.h>
#include <kio/filejob.h>

#include <QObject>
#include <QList>

class TransferHandler; 
class TransferGroupHandler;

class KUiServerJobs : public QObject
{
    Q_OBJECT
public:
    KUiServerJobs(QObject *parent=0);
    ~KUiServerJobs();
   
    void settingsChanged();

public slots:
    void slotTransferAdded(TransferHandler * transfer, TransferGroupHandler * group);
    void slotTransfersAboutToBeRemoved(const QList<TransferHandler*> &transfer);
    void slotTransfersChanged(QMap<TransferHandler *, Transfer::ChangesFlags> transfers);

private:
    void registerJob(KJob * job, TransferHandler * transfer);
    void unregisterJob(KJob * job, TransferHandler * transfer);
    bool shouldBeShown(TransferHandler * transfer);
    bool existRunningTransfers();
    KGetGlobalJob * globalJob();

private:
    QMap <TransferHandler *, KJob *> m_registeredJobs;
    QList <TransferHandler *> m_invalidTransfers;
    KGetGlobalJob *m_globalJob;
};

#endif
