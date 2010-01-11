#pragma once

// NOTE: Do not delete thread in your own code
class simple_thread 
{
public:
	simple_thread() : m_thread(NULL), m_tid(0) { }

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

	explicit simple_thread_manager() : m_list_ptr(NULL) {}

	~simple_thread_manager()
	{
		if (m_list_ptr)
		{
			delete m_list_ptr;
			m_list_ptr = NULL;
		}
	}

	inline void add(simple_thread * p_thread)
	{
		ensure_thread_list_exists_();
		m_list_ptr->add_item(p_thread);
	}

	inline void remove(simple_thread * p_thread)
	{
		ensure_thread_list_exists_();

		m_list_ptr->remove_item(p_thread);
		delete p_thread;
	}

	inline void remove_all()
	{
		ensure_thread_list_exists_();

		for (t_thread_list::iterator iter = m_list_ptr->first(); iter.is_valid(); ++iter)
		{
			delete (*iter);
		}

		m_list_ptr->remove_all();
	}

	inline void safe_remove(simple_thread * p_thread)
	{
		class safe_remove_callback : public main_thread_callback
		{
		public:
			safe_remove_callback(simple_thread * thread) : m_thread(thread) {}

			virtual void callback_run()
			{
				simple_thread_manager::instance().remove(m_thread);
			}

		private:
			simple_thread * m_thread;
		};

		static_api_ptr_t<main_thread_callback_manager>()->add_callback(
			new service_impl_t<safe_remove_callback>(p_thread));
	}

private:
	inline void ensure_thread_list_exists_()
	{
		if (!m_list_ptr)
		{
			m_list_ptr = new t_thread_list();
		}
	}

private:
	t_thread_list * m_list_ptr;
	static simple_thread_manager sm_instance;

	PFC_CLASS_NOT_COPYABLE_EX(simple_thread_manager)
};
