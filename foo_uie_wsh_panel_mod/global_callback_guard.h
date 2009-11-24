#pragma once

class global_callback_guard
{
public:
	global_callback_guard()
	{
		m_scope = true;
	}

	~global_callback_guard()
	{
		m_scope = false;
	}

	static inline bool is_safe()
	{
		return !m_scope;
	}

private:
	static volatile bool m_scope;
};

FOOGUIDDECL volatile bool global_callback_guard::m_scope = false;

#define RETURN_IF_GLOBAL_CALLBACK_NOT_SAFE(code) \
	if (!global_callback_guard::is_safe()) return (code);
