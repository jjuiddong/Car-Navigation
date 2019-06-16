//
// 2017-12-12, jjuiddong
//
#pragma once


class cMapView;
class cObserverView;
class cInformationView;
class cNavigationView;

class cViewer : public framework::cGameMain2
{
public:
	cViewer();
	virtual ~cViewer();

	virtual bool OnInit() override;
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnEventProc(const sf::Event &evt) override;


public:
	cMapView *m_mapView;
	cObserverView *m_observerView;
	cInformationView *m_infoView;
	cNavigationView *m_naviView;
};
