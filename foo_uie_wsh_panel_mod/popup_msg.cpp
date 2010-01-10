#include "stdafx.h"
#include "popup_msg.h"


bool popup_msg::sm_popup_service_initialized = false;
pfc::list_t<popup_msg::t_popup_pending> * popup_msg::sm_popup_pendings = NULL;


