/* This file is part of the KDE project

   Copyright (C) 2006 Manolo Valdes <nolis71cu@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; version 2
   of the License.
*/

#ifndef MULTISEGKIO_H
#define MULTISEGKIO_H

#include <kio/filejob.h>
#include <kio/job.h>

#include <QQueue>

namespace KIO
{

#define	MIN_SIZE	1024*100

class MultiSegData
{
   public:
      KUrl src;
      KIO::fileoffset_t offset;
      KIO::filesize_t bytes;
};

class GetJobManager;

class KIO_EXPORT MultiSegmentCopyJob : public Job
{
   Q_OBJECT

   public:

      /**
      * Do not create a MultiSegmentCopyJob directly. Use KIO::MultiSegfile_copy() instead.
      * @param src the source URL
      * @param dest the destination URL
      * @param permissions the permissions of the resulting resource
      * @param showProgressInfo true to show progress information to the user
      * @param segments number of segments
      */
      MultiSegmentCopyJob( const KUrl& src, const KUrl& dest, int permissions, bool showProgressInfo, uint segments);

      /**
      * Do not create a MultiSegmentCopyJob directly. Use KIO::MultiSegfile_copy() instead.
      * @param src the source URL
      * @param dest the destination URL
      * @param permissions the permissions of the resulting resource
      * @param showProgressInfo true to show progress information to the user
      * @param ProcessedSize
      * @param totalSize
      * @param segments a QList with segments data to resume a started copy
      */
      MultiSegmentCopyJob( const KUrl& src, const KUrl& dest,
                           int permissions, bool showProgressInfo,
                           qulonglong ProcessedSize,
                           KIO::filesize_t totalSize,
                           QList<MultiSegData> segments);

      ~MultiSegmentCopyJob() {};
      void addGetJobManagers( QList<struct MultiSegData> segments);
      void addFisrtGetJobManager( FileJob* job, MultiSegData segment);
      QList<struct MultiSegData> getSegmentsData();

   public Q_SLOTS:
      void slotStart();
      void slotOpen( KIO::Job * );
      void slotClose( KIO::Job * );
      void slotDataReq( GetJobManager *jobManager);
      void slotWritten( KIO::Job * ,KIO::filesize_t bytesWritten);
      void slotaddGetJobManager(MultiSegData segment);

   Q_SIGNALS:
      void updateSegmentsData();
      void canWrite();

   protected Q_SLOTS:
        /**
         * Called whenever a job Manager finishes.
	 * @param jobManager the job Manager that emitted this signal
         */
      virtual void slotGetJobManagerResult( GetJobManager *jobManager );

        /**
         * Called whenever a subjob finishes.
	 * @param job the job that emitted this signal
         */
      virtual void slotResult( KJob *job );

        /**
         * Forward signal from subjob
	 * @param job the job that emitted this signal
	 * @param size the processed size in bytes
         */
//         void slotProcessedSize( KJob *job, qulonglong size );
        /**
         * Forward signal from subjob
	 * @param job the job that emitted this signal
	 * @param size the total size
         */
      void slotTotalSize( KJob *job, qulonglong size );
        /**
         * Forward signal from subjob
	 * @param job the job that emitted this signal
	 * @param pct the percentage
         */
      void slotPercent( KJob *job, unsigned long pct );
        /**
         * Forward signal from subjob
	 * @param job the job that emitted this signal
	 * @param bytes_per_second the speed
         */
      void slotSpeed( KIO::Job*, unsigned long bytes_per_second );
   protected:

      KUrl m_src;
      KUrl m_dest;
      KUrl m_dest_part;
      int m_permissions;
      qulonglong m_ProcessedSize;
      KIO::filesize_t m_totalSize;
      uint m_segments;
      bool blockWrite;
      bool bcopycompleted;
      bool bstartsaved;
      FileJob* m_putJob;
      GetJobManager* m_getJob;
      QList <GetJobManager*> m_jobs;
      QHash <KIO::Job*, unsigned long>speedHash;
   private:
      bool checkLocalFile();
};

class GetJobManager :public QObject
{
   Q_OBJECT

   public:
      GetJobManager();
      QByteArray getData();
      void emitResult();

   public Q_SLOTS:
      void slotOpen( KIO::Job * );
      void slotData( KIO::Job *, const QByteArray &data);
      void slotCanWrite();
      void slotResult( KJob *job );

   Q_SIGNALS:
      void hasData ( GetJobManager * );
      void result ( GetJobManager * );
      void segmentData (MultiSegData);

   public:
      FileJob *job;
      struct MultiSegData data;
      QQueue<QByteArray> chunks;
      bool    restarting;
};

   MultiSegmentCopyJob *MultiSegfile_copy( const KUrl& src, const KUrl& dest, int permissions, bool showProgressInfo, uint segments);

   MultiSegmentCopyJob *MultiSegfile_copy(
                           const KUrl& src,
                           const KUrl& dest,
                           int permissions,
                           bool showProgressInfo,
                           qulonglong ProcessedSize,
                           KIO::filesize_t totalSize,
                           QList<MultiSegData> segments);

}

#endif //MULTISEGKIO_H
