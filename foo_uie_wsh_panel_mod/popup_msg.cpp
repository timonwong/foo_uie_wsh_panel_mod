#include "stdafx.h"
#include "popup_msg.h"


bool popup_msg::sm_popup_service_initialized = false;
popup_msg::t_popup_pendings * popup_msg::sm_popup_pendings = NULL;


