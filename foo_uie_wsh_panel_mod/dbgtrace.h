#pragma once

#define NO_TRACK_FUNCTION
//#define NO_TRACK_DISPATCH

#if !defined(NO_TRACK_FUNCTION) || defined(_DEBUG)
#define TRACK_FUNCTION() TRACK_CALL_TEXT(__FUNCTION__)
#define TRACK_CALL_TEXT_FORMAT(fmt, ...) TRACK_CALL_TEXT(uStringPrintf((fmt), __VA_ARGS__))
#else
#define TRACK_FUNCTION() 
#define TRACK_CALL_TEXT_FORMAT(fmt, ...) 
#endif

#if !defined(NO_TRACK_DISPATCH)
#define TRACK_THIS_DISPATCH_CALL(typeinfo, dispid, flag) \
	disp_call_tracker DISPCALLTRACKER__##__LINE__(typeinfo, dispid, flag)
#define PRINT_DISPATCH_TRACK_MESSAGE() disp_call_tracker::print_msg()
#else
#define TRACK_THIS_DISPATCH_CALL(typeinfo, dispid, flag) 
#define PRINT_DISPATCH_TRACK_MESSAGE() 
#endif

#if !defined(NO_TRACK_DISPATCH)
class disp_call_tracker
{
public:
	disp_call_tracker(ITypeInfo * p_typeinfo, DISPID p_dispid, WORD p_flag)
	{
		sm_last_typeinfo = p_typeinfo;
		sm_last_dispid = p_dispid;
		sm_last_flag = p_flag;
	}

	~disp_call_tracker()
	{
		sm_last_typeinfo = NULL;
	}

	static void print_msg()
	{
		BSTR name = NULL;
		UINT name_count = 0;
		TYPEATTR * ptype_attr = NULL;
		GUID guid = IID_NULL;

		if (sm_last_typeinfo)
		{
			if (SUCCEEDED(sm_last_typeinfo->GetNames(sm_last_dispid, &name, 1, &name_count)))
			{
				pfc::string_formatter formatter;

				formatter << "Additional Info:\n" << "==>Last DispFunc: (";

				switch (sm_last_flag)
				{
				case DISPATCH_METHOD:
					formatter << "method";
					break;

				case DISPATCH_PROPERTYGET:
					formatter << "property-get";
					break;

				case DISPATCH_PROPERTYPUT:
					formatter << "property-put";
					break;

				case DISPATCH_PROPERTYPUTREF:
					formatter << "property-put-ref";
					break;
				}

				if (SUCCEEDED(sm_last_typeinfo->GetTypeAttr(&ptype_attr)))
				{
					guid = ptype_attr->guid;

					sm_last_typeinfo->ReleaseTypeAttr(ptype_attr);
				}

				formatter << ") \"" << pfc::stringcvt::string_utf8_from_wide(name)
					<< "\" of interface uuid(\"" << pfc::print_guid(guid) << "\")";

				SysFreeString(name);

				console::info(formatter);
			}
		}
	}

private:
	static ITypeInfo   *sm_last_typeinfo;
	static DISPID       sm_last_dispid;
	static WORD         sm_last_flag;
};

FOOGUIDDECL ITypeInfo * disp_call_tracker::sm_last_typeinfo = NULL;
FOOGUIDDECL DISPID disp_call_tracker::sm_last_dispid = 0;
FOOGUIDDECL WORD disp_call_tracker::sm_last_flag = 0;
#endif
