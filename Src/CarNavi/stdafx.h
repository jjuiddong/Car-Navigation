#pragma once


#include "../../../Common/Common/common.h"
using namespace common;
#include "../../../Common/Graphic11/graphic11.h"
#include "../../../Common/Framework11/framework11.h"
#include "../../../Common/Network2/network2.h"
#include "../gis/gis.h"

#include "lib/utility.h"
#include "lib/obd2.h"
#include "lib/gpsclient.h"
#include "lib/touch.h"
#include "path/pathfile.h"
#include "path/binpathfile.h"
#include "path/pathrenderer.h"
#include "path/pathcompare.h"
#include "path/landmark.h"
#include "optimize/historyfile.h"
#include "optimize/qtreegraph.h"
#include "optimize/optimizepath.h"
#include "global.h"

#include "../Protocol/Src/gps_Protocol.h"
#include "../Protocol/Src/gps_ProtocolData.h"
#include "../Protocol/Src/gps_ProtocolHandler.h"


extern cGlobal *g_global;
extern framework::cGameMain2 * g_application;
