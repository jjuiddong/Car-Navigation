//
// 2018-01-16, jjuiddong
// Information View
//
#pragma once


class cInformationView : public framework::cDockWindow
{
public:
	cInformationView(const StrId &name);
	virtual ~cInformationView();

	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


public:
	Vector3 m_fogColor;
	float m_fogDistanceRate;
};
