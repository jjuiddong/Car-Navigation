#pragma once


#include "../../../Common/Common/common.h"
using namespace common;
#include "../../../Common/Network2/network2.h"


#include "../Protocol/Src/gps_Protocol.h"
#include "../Protocol/Src/gps_ProtocolData.h"
#include "../Protocol/Src/gps_ProtocolHandler.h"

#include "database/dbinterface.h"
#include "database/mysql.h"
#include "database/mongodb.h"
#include "database/dbmgr.h"

extern string g_configFileName;
