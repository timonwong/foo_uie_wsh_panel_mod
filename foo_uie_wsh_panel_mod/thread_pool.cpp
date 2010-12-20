#include "stdafx.h"
#include "thread_pool.h"
#include "user_message.h"

simple_thread_pool simple_thread_pool::instance_;

void simple_thread_worker::threadProc()
{
	ULONGLONG last_tick = GetTickCount64();

	while (WaitForSingleObject(simple_thread_pool::instance().exiting_, 0) == WAIT_TIMEOUT)
	{
		simple_thread_task * task = simple_thread_pool::instance().acquire_task();

		if (task)
		{
			task->run();
			simple_thread_pool::instance().untrack(task);
			last_tick = GetTickCount64();
		}
		else
		{
			if (GetTickCount64() - last_tick >= 10000)
			{
				insync(simple_thread_pool::instance().cs_);
				
				if (simple_thread_pool::instance().is_queue_empty())
				{
					simple_thread_pool::instance().remove_worker_(this);
					return;
				}
			}
		}
	}

	simple_thread_pool::instance().remove_worker_(this);
}

bool simple_thread_pool::enqueue(simple_thread_task * task)
{
	core_api::ensure_main_thread();

	if (WaitForSingleObject(exiting_, 0) == WAIT_OBJECT_0)
		return false;

	insync(cs_);
	int max_count = pfc::getOptimalWorkerThreadCount();
	track(task);

	if (num_workers_ < max_count)
	{
		simple_thread_worker * worker = new simple_thread_worker;
		add_worker_(worker);
		worker->start();
	}

	return true;
}

bool simple_thread_pool::is_queue_empty()
{
	insync(cs_);
	return task_list_.get_count() == 0;
}

void simple_thread_pool::track(simple_thread_task * task)
{
	insync(cs_);
	task_list_.add_item(task);
}

void simple_thread_pool::untrack(simple_thread_task * task)
{
	insync(cs_);

	if (core_api::is_main_thread())
	{
		untrack_(task);
	}
	else
	{
		static_api_ptr_t<main_thread_callback_manager>()->add_callback(
			new service_impl_t<safe_untrack_callback>(task));
	}
}

void simple_thread_pool::join()
{
	core_api::ensure_main_thread();
	
	SetEvent(exiting_);
	
	// Because the tasks may use blocking SendMessage() function, it should be avoid using
	//   infinite wait here, or both main thread and worker thread will bock, and it's also
	//   important to dispatch windows messages here.
	while (WaitForSingleObject(empty_worker_, 0) == WAIT_TIMEOUT)
	{
		MSG msg;
		
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	for (t_task_list::iterator iter = task_list_.first(); iter.is_valid(); ++iter)
	{
		task_list_.remove(iter);
		delete *iter;
	}
}

simple_thread_task * simple_thread_pool::acquire_task()
{
	insync(cs_);

	t_task_list::iterator iter = task_list_.first();

	if (iter.is_valid())
	{
		task_list_.remove(iter);
		return *iter;
	}

	return NULL;
}

void simple_thread_pool::untrack_(simple_thread_task * task)
{
	core_api::ensure_main_thread();
	insync(cs_);
	task_list_.remove_item(task);
	delete task;
}

void simple_thread_pool::add_worker_(simple_thread_worker * worker)
{
	core_api::ensure_main_thread();
	insync(cs_);
	InterlockedIncrement(&num_workers_);
	ResetEvent(empty_worker_);
}

class simple_thread_worker_remover : public main_thread_callback
{
public:
	simple_thread_worker_remover(simple_thread_worker * worker) : worker_(worker) {}

	virtual void callback_run()
	{
		delete worker_;
	}

private:
	simple_thread_worker * worker_;
};

void simple_thread_pool::remove_worker_(simple_thread_worker * worker)
{
	insync(cs_);
	InterlockedDecrement(&num_workers_);

	if (num_workers_ == 0)
	{
		SetEvent(empty_worker_);
	}

	static_api_ptr_t<main_thread_callback_manager>()->add_callback(
		new service_impl_t<simple_thread_worker_remover>(worker));
}
