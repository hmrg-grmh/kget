/* This file is part of the KDE project

   Copyright (C) 2004 Dario Massarin <nekkar@libero.it>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; version 2
   of the License.
*/

#include <qdom.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kio/job.h>
#include <kdebug.h>

#include "transferKio.h"

TransferKio::TransferKio(TransferGroup * parent, TransferFactory * factory,
                         Scheduler * scheduler, const KURL & source, const KURL & dest,
                         const QDomElement * e)
    : Transfer(parent, factory, scheduler, source, dest, e),
      m_copyjob(0)
{

}

void TransferKio::start()
{
    if(!m_copyjob)
        createJob();

    kdDebug() << "TransferKio::start" << endl;

    setStatus(Job::Running, i18n("Connecting.."), SmallIcon("connect_creating"));
    setTransferChange(Tc_Status, true);
}

void TransferKio::stop()
{
    if(status() == Stopped)
        return;

    if(m_copyjob)
    {
        m_copyjob->kill(true);
        m_copyjob=0;
    }

    kdDebug() << "Stop" << endl;
    setStatus(Job::Stopped, i18n("Stopped"), SmallIcon("stop"));
    m_speed = 0;
    setTransferChange(Tc_Status | Tc_Speed, true);
}

int TransferKio::elapsedTime() const
{
    return -1; //TODO
}

int TransferKio::remainingTime() const
{
    return -1; //TODO
}

bool TransferKio::isResumable() const
{
    return true;
}

void TransferKio::load(QDomElement e)
{
    Transfer::load(e);
}

void TransferKio::save(QDomElement e)
{
    Transfer::save(e);
}


//NOTE: INTERNAL METHODS

void TransferKio::createJob()
{
    if(!m_copyjob)
    {
        m_copyjob = KIO::file_copy(m_source, m_dest, -1, false, false, false);

        connect(m_copyjob, SIGNAL(result(KIO::Job *)), 
                this, SLOT(slotResult(KIO::Job *)));
        connect(m_copyjob, SIGNAL(infoMessage(KIO::Job *, const QString &)), 
                this, SLOT(slotInfoMessage(KIO::Job *, const QString &)));
        connect(m_copyjob, SIGNAL(connected(KIO::Job *)), 
                this, SLOT(slotConnected(KIO::Job *)));
        connect(m_copyjob, SIGNAL(percent(KIO::Job *, unsigned long)), 
                this, SLOT(slotPercent(KIO::Job *, unsigned long)));
        connect(m_copyjob, SIGNAL(totalSize(KIO::Job *, KIO::filesize_t)), 
                this, SLOT(slotTotalSize(KIO::Job *, KIO::filesize_t)));
        connect(m_copyjob, SIGNAL(processedSize(KIO::Job *, KIO::filesize_t)), 
                this, SLOT(slotProcessedSize(KIO::Job *, KIO::filesize_t)));
        connect(m_copyjob, SIGNAL(speed(KIO::Job *, unsigned long)), 
                this, SLOT(slotSpeed(KIO::Job *, unsigned long)));
    }
}

void TransferKio::slotResult( KIO::Job * kioJob )
{
    kdDebug() << "slotResult" << endl;
    switch (kioJob->error())
    {
        case 0:                            //The download has finished
        case KIO::ERR_FILE_ALREADY_EXIST:  //The file has already been downloaded.
            setStatus(Job::Finished, i18n("Finished"), SmallIcon("ok"));
            m_percent = 100;
            m_speed = 0;
            m_processedSize = m_totalSize;
            setTransferChange(Tc_Percent | Tc_Speed);
            break;
        default:
            //There has been an error
            setStatus(Job::Aborted, i18n("Aborted"), SmallIcon("stop"));
            kdDebug() << "--  E R R O R  (" << kioJob->error() << ")--" << endl;
            break;
    }
    // when slotResult gets called, the m_copyjob has already been deleted!
    m_copyjob=0;
    setTransferChange(Tc_Status, true);
}

void TransferKio::slotInfoMessage( KIO::Job * kioJob, const QString & msg )
{
  Q_UNUSED(kioJob);
    m_log.append(QString(msg));
}

void TransferKio::slotConnected( KIO::Job * kioJob )
{
//     kdDebug() << "CONNECTED" <<endl;

  Q_UNUSED(kioJob);
    setStatus(Job::Running, i18n("Downloading.."), SmallIcon("tool_resume"));
    setTransferChange(Tc_Status, true);
}

void TransferKio::slotPercent( KIO::Job * kioJob, unsigned long percent )
{
    kdDebug() << "slotPercent" << endl;
    Q_UNUSED(kioJob);
    m_percent = percent;
    setTransferChange(Tc_Percent, true);
}

void TransferKio::slotTotalSize( KIO::Job * kioJob, KIO::filesize_t size )
{
    kdDebug() << "slotTotalSize" << endl;

    slotConnected(kioJob);

    m_totalSize = size;
    setTransferChange(Tc_TotalSize, true);
}

void TransferKio::slotProcessedSize( KIO::Job * kioJob, KIO::filesize_t size )
{
    kdDebug() << "slotProcessedSize" << endl; 

    if(status() != Job::Running)
        slotConnected(kioJob);

    m_processedSize = size;
    setTransferChange(Tc_ProcessedSize, true);
}

void TransferKio::slotSpeed( KIO::Job * kioJob, unsigned long bytes_per_second )
{
//     kdDebug() << "slotSpeed" << endl;

    if(status() != Job::Running)
        slotConnected(kioJob);

    m_speed = bytes_per_second;
    setTransferChange(Tc_Speed, true);
}

#include "transferKio.moc"
