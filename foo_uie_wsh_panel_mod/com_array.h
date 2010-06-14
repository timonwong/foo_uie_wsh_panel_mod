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

		~com_array_reader()
		{
			reset();
		}

		inline SAFEARRAY * get_ptr()
		{
			return m_psa;
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

		inline bool get_item(long idx, VARIANT & dest)
		{
			if (!m_psa) return false;
			if (idx < m_lbound || idx > m_ubound) return false;

			return SUCCEEDED(SafeArrayGetElement(m_psa, &idx, &dest));
		}

		inline VARIANT operator[](long idx)
		{
			_variant_t var;

			if (!get_item(idx, var))
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

	// 1D
	template <bool managed = false>
	class com_array_writer
	{
	public:
		com_array_writer() : m_psa(NULL)
		{
			reset();
		}

		~com_array_writer()
		{
			if (managed)
			{
				reset();
			}
		}

		inline SAFEARRAY * get_ptr()
		{
			return m_psa;
		}

		inline long get_count()
		{
			return m_count;
		}

		inline bool create(long count)
		{
			reset();

			m_psa = SafeArrayCreateVector(VT_VARIANT, 0, count);
			m_count = count;
			return (m_psa != NULL);
		}

		inline HRESULT put(long idx, VARIANT & pVar)
		{
			if (idx >= m_count) return E_INVALIDARG;
			if (!m_psa) return E_POINTER;

			HRESULT hr = SafeArrayPutElement(m_psa, &idx, &pVar);
			return hr;
		}

	public:
		void reset()
		{
			m_count = 0;

			if (m_psa)
			{
				SafeArrayDestroy(m_psa);
				m_psa = NULL;
			}
		}

	private:
		SAFEARRAY * m_psa;
		long m_count;
	};

	// 2D
	//template <bool managed = false>
	//class com_array_writer_2d
	//{
	//public:
	//	com_array_writer_2d() : m_psa(NULL)
	//	{
	//		reset();
	//	}

	//	~com_array_writer_2d()
	//	{
	//		if (managed)
	//		{
	//			reset();
	//		}
	//	}

	//	inline SAFEARRAY * get_ptr()
	//	{
	//		return m_psa;
	//	}

	//	inline long get_dim1() { return m_dim1; }
	//	inline long get_dim2() { return m_dim2; }

	//	inline bool create(long dim1, long dim2)
	//	{
	//		reset();
	//		
	//		SAFEARRAYBOUND bound[2];
	//		bound[0].lLbound = 0;
	//		bound[0].cElements = dim1;
	//		bound[1].lLbound = 0;
	//		bound[1].cElements = dim2;

	//		m_psa = SafeArrayCreate(VT_VARIANT, 2, bound);
	//		m_dim1 = dim1;
	//		m_dim2 = dim2;
	//		return (m_psa != NULL);
	//	}

	//	inline HRESULT put(long i1, long i2, VARIANT & pVar)
	//	{
	//		if (i1 >= m_dim1 || i2 >= m_dim2) return E_INVALIDARG;
	//		if (!m_psa) return E_POINTER;

	//		long idx[2];
	//		idx[0] = i1;
	//		idx[1] = i2;

	//		HRESULT hr = SafeArrayPutElement(m_psa, idx, &pVar);
	//		return hr;
	//	}

	//public:
	//	void reset()
	//	{
	//		m_dim1 = m_dim2 = 0;

	//		if (m_psa)
	//		{
	//			SafeArrayDestroy(m_psa);
	//			m_psa = NULL;
	//		}
	//	}

	//private:
	//	long m_dim1, m_dim2;
	//	SAFEARRAY * m_psa;
	//};
}
