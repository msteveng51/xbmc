/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#ifdef HAS_PULSEAUDIO

#include "PulseAEEventThread.h"
#include "Utils/AEUtil.h"
#include "utils/log.h"
#include "threads/SingleLock.h"

CPulseAEEventThread::CPulseAEEventThread(CPulseAEStream *stream) :
  m_run   (true  ),
  m_stream(stream),
  m_thread(this  ),
  m_cbFunc(NULL  ),
  m_arg   (NULL  )
{
  m_thread.Create();
}

CPulseAEEventThread::~CPulseAEEventThread()
{
  /* tell the thread we are terminating */
  CSingleLock lock(m_lock);
  m_run = false;
  lock.Leave();

  /* trigger the event incase it is blocking */
  Trigger();

  /* wait for the thread to exit */
  m_thread.StopThread(true);
}

void CPulseAEEventThread::SetCallback(IAEStream::AECBFunc *cbFunc, void *arg)
{
  CSingleLock lock(m_lock);
  m_cbFunc = cbFunc;
  m_arg    = arg;
}

void CPulseAEEventThread::Trigger()
{
  CSingleLock lock(m_lockEvent);
  m_event.Set();
}

void CPulseAEEventThread::Run()
{
/*
  this thread has to be VERY careful not to leave m_lock locked
  when m_cbFunc is called, otherwise signals generated by calls
  in the callback function WILL cause a deadlock when the next
  signal signal arrives
*/

  CSingleLock lock(m_lock);
  while(m_run)
  {
    lock.Leave();

    /* wait on the event */
    m_event.Wait();
    lock.Enter();
    if (!m_run)
      return;

    /* fire the callback */
    if (m_cbFunc)
      m_cbFunc(m_stream, m_arg, m_stream->GetSpace());

    /* reset the event */
    CSingleLock lockEvent(m_lockEvent);
    m_event.Reset();
    lockEvent.Leave(); 
  }
}

#endif
