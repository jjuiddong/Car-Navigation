#pragma once


#include "../../../Common/Common/common.h"
using namespace common;
#include "../../../Common/Graphic11/graphic11.h"
#include "../../../Common/Framework11/framework11.h"
#include "../../../Common/Network2/network2.h"
#include "../gis/gis.h"

#include "utility.h"
#include "obd2.h"
#include "gpsclient.h"
#include "path.h"
#include "pathrenderer.h"
#include "pathcompare.h"
#include "landmark.h"
#include "touch.h"
#include "global.h"

#include "../Protocol/Src/gps_Protocol.h"
#include "../Protocol/Src/gps_ProtocolData.h"
#include "../Protocol/Src/gps_ProtocolHandler.h"


extern cGlobal *g_global;
extern framework::cGameMain2 * g_application;
