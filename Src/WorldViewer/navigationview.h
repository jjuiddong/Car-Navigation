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
};
