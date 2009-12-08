#pragma once

class simple_thread 
{
public:
	simple_thread() : m_thread(NULL), m_tid(0) {}

	virtual ~simple_thread() { close(); }

	void start();

	void close();

	inline bool is_active() const
	{
		return (m_thread != NULL);
	}

	inline unsigned get_tid() const
	{
		return m_tid;
	}

protected:
	virtual void thread_proc() = 0;

private:
	static unsigned int CALLBACK g_entry(void* p_instance);

	HANDLE m_thread;
	unsigned m_tid;

	PFC_CLASS_NOT_COPYABLE_EX(simple_thread)
};

class simple_thread_manager
{
public:
	typedef pfc::chain_list_v2_t<simple_thread *> t_thread_list;

	static inline simple_thread_manager & instance()
	{
		return sm_instance;
	}

	explicit simple_thread_manager()
	{}

	void add(simple_thread * p_thread)
	{
		m_list.add_item(p_thread);
	}

	void remove(simple_thread * p_thread)
	{
		m_list.remove_item(p_thread);
	}

	void remove_all()
	{
		for (t_thread_list::iterator iter = m_list.first(); iter.is_valid(); ++iter)
		{
			(*iter)->close();
		}
	}

private:
	t_thread_list m_list;
	static simple_thread_manager sm_instance;

	PFC_CLASS_NOT_COPYABLE_EX(simple_thread_manager)
};
