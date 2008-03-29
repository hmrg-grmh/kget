/* This file is part of the KDE project

   Copyright (C) 2007 by Lukas Appelhans <l.appelhans@gmx.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

#ifndef TRANSFERHISTORY_H
#define TRANSFERHISTORY_H

#include <QList>
#include <KDialog>

#include "ui_transferhistory.h"

class QFileSystemWatcher;
class QLineEdit;
class QModelIndex;
class QProgressBar;
class KPushButton;
class TransferHistoryStore;
class TransferHistoryItem;
class TransferHistoryCategorizedView;

class TransferHistory : public KDialog, Ui::TransferHistory
{
    Q_OBJECT

    public:
        TransferHistory(QWidget *parent = 0);
        ~TransferHistory();

    private:
        enum RangeType {
            Date = 0,
            Size = 1,
            Host = 2
        };
        void hideEvent(QHideEvent *event);
        bool save;
        QFileSystemWatcher *watcher;
        int m_rangeType;
        QWidget *m_view;
        QProgressBar *m_progressBar;
        QVBoxLayout *m_verticalLayout;
        QComboBox *m_rangeTypeCombobox;
        QHBoxLayout *m_hboxLayout;
        QLineEdit *m_searchBar;
        QAction *m_actionDelete_Selected;
        QAction *m_actionClear;
        QAction *m_actionDownload;
        QAction *m_openFile;
        QPushButton *m_clearButton;
        KPushButton *m_iconView;
        KPushButton *m_listView;
        bool m_iconModeEnabled;
        TransferHistoryStore *m_store;

    public slots:
        void slotDeleteTransfer(const QString &url, const QModelIndex &index = QModelIndex());

    private slots:
        void slotDeleteTransfer();
        void slotAddTransfers();
        void slotClear();
        void slotWriteDefault();
        void slotDownload();
        void slotOpenFile();
        void contextMenuEvent(QContextMenuEvent *event);
        void slotLoadRangeType(int type);
        void slotSetListMode();
        void slotSetIconMode();
        void slotElementLoaded(int number, int total, const TransferHistoryItem &item);
        void slotLoadFinished();
};

#endif