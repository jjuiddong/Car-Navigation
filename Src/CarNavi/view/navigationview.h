//
// 2019-05-25, jjuiddong
// navigation view
//
#pragma once


class cNavigationView : public framework::cDockWindow
{
public:
	cNavigationView(const StrId &name);
	virtual ~cNavigationView();

	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


public:
	vector<std::pair<UINT, string>> m_ports;
	Str512 m_comboStr;
	int m_comboIdx;
	float m_serialConnectTime;
};
