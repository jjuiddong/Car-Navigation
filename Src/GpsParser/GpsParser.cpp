//
// gps.txt 파일을 읽어서, 위치정보와 시간정보를 가져와서 path.txt로 저장한다.
//

#include "pch.h"
#include <iostream>

using namespace std;

int main()
{
	ifstream ifs("gps.txt");
	if (!ifs.is_open())
	{
		cout << "error read gps.txt" << endl;
		return 0;
	}

	ofstream ofs("gps_path.txt");
	if (!ofs.is_open())
	{
		cout << "error open path.txt" << endl;
		return 0;
	}

	ofs.precision(std::numeric_limits<double>::max_digits10);

	int cnt = 0;
	string line;
	while (getline(ifs, line))
	{
		cnt++;

		gis::sGPRMC gps;
		if (gis::GetGPRMC(line, gps))
		{
			ofs << cDateTime2::GetTimeStr2(gps.date).c_str()
				<< ", " << gps.lonLat.x
				<< ", " << gps.lonLat.y
				<< endl;
		}

		sDateTime date;
		cDateTime2::GetTimeValue(gps.date, date);
		if ((date.month == 6) && (date.day == 8))
		{
			int a = 0;
		}

	}

	return 1;
}
