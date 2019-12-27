//
// 2019-12-24, jjuiddong
// path compare
//
#pragma once


class cPathCompare
{
public:
	cPathCompare();
	virtual ~cPathCompare();

	bool Compare(const StrPath &pathDirectory);
};
