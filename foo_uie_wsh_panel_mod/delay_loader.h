#pragma once

/* WARNING: No thread safety */

class NOVTABLE delay_loader_action : public service_base
{
public:
	virtual void execute() = 0;

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(delay_loader_action);
};

// {8B80CA07-4F7E-4C0A-A23C-C5827558DF87}
FOOGUIDDECL const GUID delay_loader_action::class_guid = 
	{ 0x8b80ca07, 0x4f7e, 0x4c0a, { 0xa2, 0x3c, 0xc5, 0x82, 0x75, 0x58, 0xdf, 0x87 } };

class delay_loader
{
public:
	static inline void g_enqueue(service_ptr_t<delay_loader_action> callback)
	{
		if (!g_ready())
			callbacks_.add_item(callback);
		else
			callback->execute();
	}

	static inline void g_set_ready()
	{
		services_initialized_ = true;

		for (t_size i = 0; i < callbacks_.get_count(); ++i)
			callbacks_[i]->execute();

		callbacks_.remove_all();
	}

	static inline bool g_ready()
	{
		return services_initialized_;
	}

private:
	static service_list_t<delay_loader_action> callbacks_;
	static bool services_initialized_;
};

FOOGUIDDECL bool delay_loader::services_initialized_ = false;
FOOGUIDDECL service_list_t<delay_loader_action> delay_loader::callbacks_;
