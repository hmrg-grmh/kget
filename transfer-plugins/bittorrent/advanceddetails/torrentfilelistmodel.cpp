/** IMPORTANT: please keep this file in sync with ktorrent! ****************/

/***************************************************************************
 *   Copyright (C) 2007 by Joris Guisson and Ivan Vasic                    *
 *   joris.guisson@gmail.com                                               *
 *   ivasic@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#include "torrentfilelistmodel.h"

#include <klocale.h>
#include <kicon.h>
#include <kmimetype.h>
#include <QTreeView>
#include <interfaces/torrentinterface.h>
#include <interfaces/torrentfileinterface.h>
#include <util/functions.h>

using namespace bt;

namespace kt
{
	

	TorrentFileListModel::TorrentFileListModel(bt::TorrentInterface* tc,DeselectMode mode,QObject* parent) 
	: TorrentFileModel(tc,mode,parent)
	{
	}


	TorrentFileListModel::~TorrentFileListModel()
	{}
	
	int TorrentFileListModel::rowCount(const QModelIndex & parent) const
	{	
		if (!parent.isValid())
			return tc->getStats().multi_file_torrent ? tc->getNumFiles() : 1;
		else
			return 0;
	}
	
	int TorrentFileListModel::columnCount(const QModelIndex & parent) const
	{
		if (!parent.isValid())
			return 2;
		else
			return 0;
	}
	
	QVariant TorrentFileListModel::headerData(int section, Qt::Orientation orientation,int role) const
	{
		if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
			return QVariant();
		 
		switch (section)
		{
			case 0: return i18n("File");
			case 1: return i18n("Size");
			default:
				return QVariant();
		}
	}
	
	QVariant TorrentFileListModel::data(const QModelIndex & index, int role) const
	{
		if (!index.isValid())
			return QVariant();
			
			
		int r = index.row();
		int nfiles = rowCount(QModelIndex());
		bool multi = tc->getStats().multi_file_torrent;
		if (r < 0 || r >= nfiles)
			return QVariant();

		const TorrentStats & s = tc->getStats();
		if (role == Qt::DisplayRole)
		{
			switch (index.column())
			{
				case 0: return multi ? tc->getTorrentFile(r).getPath() : s.torrent_name;
				case 1: 
					if (multi)
						return BytesToString(tc->getTorrentFile(r).getSize());
					else
						return BytesToString(s.total_bytes);
				default: return QVariant();
			}
		}
		else if (role == Qt::DecorationRole && index.column() == 0)
		{
			// if this is an empty folder then we are in the single file case
			if (multi)
				return KIcon(KMimeType::findByPath(tc->getTorrentFile(r).getPath())->iconName());
			else
				return KIcon(KMimeType::findByPath(s.torrent_name)->iconName());
		}
		else if (role == Qt::CheckStateRole && index.column() == 0)
		{
			if (multi)
				return tc->getTorrentFile(r).doNotDownload() ? Qt::Unchecked : Qt::Checked;;
		}
		
		return QVariant();
	}
	
	QModelIndex TorrentFileListModel::parent(const QModelIndex & index) const
	{
		return QModelIndex();
	}
	
	QModelIndex TorrentFileListModel::index(int row,int column,const QModelIndex & parent) const
	{
		if (!hasIndex(row, column, parent))
			return QModelIndex();
		else
		{
			bt::TorrentFileInterface* f = &tc->getTorrentFile(row);
			return createIndex(row,column,f);
		}
	}
	
	Qt::ItemFlags TorrentFileListModel::flags(const QModelIndex & index) const
	{
		if (!index.isValid())
			return 0;
		else if (tc->getStats().multi_file_torrent)
			return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
		else
			return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	}
	
	bool TorrentFileListModel::setData(const QModelIndex & index, const QVariant & value, int role) 
	{
		if (!index.isValid() || role != Qt::CheckStateRole)
			return false;
		
		Qt::CheckState newState = static_cast<Qt::CheckState>(value.toInt());
		bt::TorrentFileInterface & file = tc->getTorrentFile(index.row());
		if (newState == Qt::Checked)
		{
			if (file.getPriority() == ONLY_SEED_PRIORITY)
				file.setPriority(NORMAL_PRIORITY);
			else
				file.setDoNotDownload(false);
		}
		else
		{
			if (mode == KEEP_FILES)
				file.setPriority(ONLY_SEED_PRIORITY);
			else
				file.setDoNotDownload(true);
		}
		dataChanged(createIndex(index.row(),0),createIndex(index.row(),columnCount(index) - 1));
		checkStateChanged();
		return true;
	}

	void TorrentFileListModel::checkAll()
	{
		if (tc->getStats().multi_file_torrent)
		{
			for (Uint32 i = 0;i < tc->getNumFiles();i++)
				setData(index(i,0,QModelIndex()),Qt::Checked,Qt::CheckStateRole);
		}
	}

	void TorrentFileListModel::uncheckAll()
	{
		if (tc->getStats().multi_file_torrent)
		{
			for (Uint32 i = 0;i < tc->getNumFiles();i++)
				setData(index(i,0,QModelIndex()),Qt::Unchecked,Qt::CheckStateRole);
		}
	}

	void TorrentFileListModel::invertCheck()
	{
		if (!tc->getStats().multi_file_torrent)
			return;

		for (Uint32 i = 0;i < tc->getNumFiles();i++)
			invertCheck(index(i,0,QModelIndex()));
	}

	void TorrentFileListModel::invertCheck(const QModelIndex & idx)
	{
		if (tc->getTorrentFile(idx.row()).doNotDownload())
			setData(idx,Qt::Checked,Qt::CheckStateRole);
		else
			setData(idx,Qt::Unchecked,Qt::CheckStateRole);
	}

	bt::Uint64 TorrentFileListModel::bytesToDownload()
	{
		if (tc->getStats().multi_file_torrent)
		{
			bt::Uint64 ret = 0;
			for (Uint32 i = 0;i < tc->getNumFiles();i++)
			{
				const bt::TorrentFileInterface & file = tc->getTorrentFile(i);
				if (!file.doNotDownload())
					ret += file.getSize();
			}
			return ret;
		}
		else
			return tc->getStats().total_bytes;
	}

	bt::TorrentFileInterface* TorrentFileListModel::indexToFile(const QModelIndex & idx)
	{
		if (!idx.isValid())
			return 0;
		
		int r = idx.row();
		if (r < 0 || r >= rowCount(QModelIndex()))
			return 0;
		else
			return &tc->getTorrentFile(r);
	}
	
	QString TorrentFileListModel::dirPath(const QModelIndex & idx)
	{
		if (!idx.isValid())
			return QString();
		
		int r = idx.row();
		if (r < 0 || r >= rowCount(QModelIndex()))
			return QString();
		else
			return tc->getTorrentFile(r).getPath();
	}
	
	void TorrentFileListModel::changePriority(const QModelIndexList & indexes,bt::Priority newpriority)
	{
		foreach (QModelIndex idx,indexes)
		{
			setData(idx,newpriority,Qt::UserRole);
		}
	}
}

#include "torrentfilelistmodel.moc"
