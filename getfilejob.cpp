/***************************************************************************
                          getfilejob.cpp  -  description
                             -------------------
    begin                   : Wed May 8 2002
    copyright            : (C) 2002 by Patrick Charbonnier
    email                  : pch@freeshell.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "getfilejob.h"
using namespace KIO;

GetFileJob::GetFileJob(const KURL & m_src, const KURL & m_dest):FileCopyJob(m_src, m_dest, 0, false, false, false, false)
{
}

GetFileJob::~GetFileJob()
{
}

/** No descriptions */
bool GetFileJob::getCanResume()
{
    return m_canResume;
}
