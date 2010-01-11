#include "stdafx.h"
#include "simple_thread.h"


simple_thread_manager simple_thread_manager::sm_instance;

void simple_thread::start()
{
	close();

	// Tracking this
	simple_thread_manager::instance().add(this);

	const int priority = GetThreadPriority(GetCurrentThread());
	const bool overridePriority = (priority != THREAD_PRIORITY_NORMAL);
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, g_entry, reinterpret_cast<void*>(this), overridePriority ? CREATE_SUSPENDED : 0, &m_tid);

	if (thread == NULL)
		pfc::throw_exception_with_message<pfc::exception>("Could not create thread");

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
	// Note this procedure is IN thread.
	simple_thread * thread_ptr = reinterpret_cast<simple_thread *>(p_instance);

	try
	{
		thread_ptr->thread_proc();
	}
	catch (...) {}

	// Destroy this thread class in main_thread_callback.
	simple_thread_manager::instance().safe_remove(thread_ptr);

	return 0;
}
