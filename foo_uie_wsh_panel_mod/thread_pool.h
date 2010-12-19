#pragma once

class simple_thread_task
{
public:
	virtual void run() { PFC_ASSERT(!"Should not go here"); }
};

class simple_thread_worker : public pfc::thread
{
public:
	simple_thread_worker() {}
	virtual ~simple_thread_worker() { waitTillDone(); }
	virtual void threadProc();

private:
	PFC_CLASS_NOT_COPYABLE_EX(simple_thread_worker)
};

class simple_thread_pool
{
private:
	class safe_untrack_callback : public main_thread_callback
	{
	public:
		safe_untrack_callback(simple_thread_task * context) : context_(context) {}

		virtual void callback_run()
		{
			simple_thread_pool::instance().untrack_(context_);
		}

	private:
		simple_thread_task * context_;
	};

public:
	static inline simple_thread_pool & instance()
	{
		return instance_;
	}

	inline simple_thread_pool() : num_workers_(0)
	{
		empty_worker_ = CreateEvent(NULL, TRUE, TRUE, NULL);
		exiting_ = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	inline ~simple_thread_pool()
	{
		CloseHandle(empty_worker_);
		CloseHandle(exiting_);
	}

	static inline HANDLE & exiting() { return exiting_; }

	bool queue(simple_thread_task * task);
	void track(simple_thread_task * task);
	void untrack(simple_thread_task * task);
	// Should always called from the main thread
	void join();
	simple_thread_task * acquire_task();

private:
	void untrack_(simple_thread_task * task);
	void add_worker_(simple_thread_worker * worker);
	void remove_worker(simple_thread_worker * worker);

	typedef pfc::chain_list_v2_t<simple_thread_task *> t_task_list;
	t_task_list task_list_;
	critical_section cs_;
	volatile LONG num_workers_;
	HANDLE empty_worker_;

	static HANDLE exiting_;
	static simple_thread_pool instance_;

	friend class simple_thread_worker;

	PFC_CLASS_NOT_COPYABLE_EX(simple_thread_pool)
};
