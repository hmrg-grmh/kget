/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *
 *   Copyright (C) 2007 by Javier Goday <jgoday@gmail.com>
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "panelgraph.h"

#include <plasma/widgets/progressbar.h>
#include <plasma/layouts/boxlayout.h>

PanelGraph::PanelGraph(Plasma::Applet *parent)
    : TransferGraph(parent)
{
    m_layout = dynamic_cast<Plasma::BoxLayout *>(parent->layout());
    if (m_layout) {
        m_bar = new Plasma::ProgressBar(m_applet);
        m_bar->setMinimumSize(QSizeF(80, 40));
        m_bar->setValue(0);

        m_layout->addItem(m_bar);
    }
}

PanelGraph::~PanelGraph()
{
    delete m_bar;
}

void PanelGraph::setTransfers(const QVariantMap &transfers)
{
    int totalSize = 0;
    int completedSize = 0;

    TransferGraph::setTransfers(transfers);

    foreach(QString key, transfers.keys()) {
        QVariantList attributes = transfers [key].toList();

        // only show the percent of the active transfers
        if(attributes.at(3).toUInt() == 1) {
            totalSize += attributes.at(2).toInt();
            completedSize += (attributes.at(1).toInt() * attributes.at(2).toInt()) / 100.0;
        }
    }

    if(totalSize > 0) {
        m_bar->setValue((int) ((completedSize * 100) / totalSize));
    }
}