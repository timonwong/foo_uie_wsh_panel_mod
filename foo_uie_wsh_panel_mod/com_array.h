#pragma once

namespace helpers
{
	// 1D
	// VBArray remains, convert JScript array to VBArray
	class com_array_reader
	{
	public:
		com_array_reader() : m_psa(NULL)
		{
			reset();
		}

		com_array_reader(VARIANT * pVarSrc) : m_psa(NULL)
		{
			convert(pVarSrc);
		}

		inline long get_lbound()
		{
			return m_lbound;
		}

		inline long get_ubound()
		{
			return m_ubound;
		}

		inline int get_count()
		{
			return get_ubound() - get_lbound() + 1;
		}

		bool get_item(long idx, VARIANT * dest)
		{
			if (!m_psa || !dest) return false;
			if (idx < m_lbound || idx > m_ubound) return false;

			LONG n = idx;

			return SUCCEEDED(SafeArrayGetElement(m_psa, &n, dest));
		}

		inline VARIANT operator[](long idx)
		{
			_variant_t var;

			if (!get_item(idx, &var))
			{
				throw std::out_of_range("Out of range");
			}

			return var;
		}

	public:
		bool convert(VARIANT * pVarSrc);

		void reset()
		{
			m_ubound = -1;
			m_lbound = 0;

			if (m_psa)
			{
				SafeArrayDestroy(m_psa);
				m_psa = NULL;
			}
		}

	private:
		void calc_bound();
		bool convert_jsarray(IDispatch * pdisp);

	private:
		SAFEARRAY * m_psa;
		long m_lbound, m_ubound;
	};
}
