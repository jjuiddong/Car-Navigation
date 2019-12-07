#include "stdafx.h"
#include "gps_Protocol.h"
using namespace gps;

//------------------------------------------------------------------------
// Protocol: GPSInfo
//------------------------------------------------------------------------
void gps::c2s_Protocol::GPSInfo(netid targetId, const string &date, const double &lon, const double &lat)
{
	cPacket packet(m_node->GetPacketHeader());
	packet.SetProtocolId( GetId() );
	packet.SetPacketId( 1288261456 );
	packet << date;
	AddDelimeter(packet);
	packet << lon;
	AddDelimeter(packet);
	packet << lat;
	AddDelimeter(packet);
	packet.EndPack();
	GetNode()->Send(targetId, packet);
}



