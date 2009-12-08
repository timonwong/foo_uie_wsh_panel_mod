#include "stdafx.h"
#include "simple_thread.h"


/*static*/ simple_thread_manager simple_thread_manager::sm_instance;

void simple_thread::start()
{
	close();

	const int priority = GetThreadPriority(GetCurrentThread());
	const bool overridePriority = (priority != THREAD_PRIORITY_NORMAL);
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, g_entry, reinterpret_cast<void*>(this), overridePriority ? CREATE_SUSPENDED : 0, &m_tid);

	if (thread == NULL)
		pfc::throw_exception_with_message<pfc::exception>("Could not create thread");

	simple_thread_manager::instance().add(this);

	if (overridePriority)
	{
		SetThreadPriority(thread, priority);
		ResumeThread(thread);
	}

	m_thread = thread;
}

void simple_thread::close()
{
	if (is_active())
	{
		WaitForSingleObject(m_thread, INFINITE);
		CloseHandle(m_thread);
		m_thread = NULL;
	}
}

unsigned int CALLBACK simple_thread::g_entry(void* p_instance)
{
	simple_thread * thread_ptr = reinterpret_cast<simple_thread *>(p_instance);

	try
	{
		thread_ptr->thread_proc();
	}
	catch (std::exception &)
	{

	}

	simple_thread_manager::instance().remove(thread_ptr);
	delete thread_ptr;
	return 0;
}
