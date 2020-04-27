
#include "../../../Common/Common/common.h"
using namespace common;
using namespace std;

bool g_isLoop = true;
int g_portNum = 6;


BOOL CtrlHandler(DWORD fdwCtrlType)
{
	g_isLoop = false;
	return TRUE;
}


int MainThreadFunction()
{
	cSerialAsync serial;
	const bool result = serial.Open(g_portNum, 9600, '\n');
	if (result)
		cout << "Success Connect Serial Port " << g_portNum << endl;
	else
		cout << "Fail!! Connect Serial Port " << g_portNum << endl;

	Str512 recvStr;
	while (g_isLoop)
	{
		if (!serial.IsOpen())
			break;

		const uint len = serial.RecvData((BYTE*)recvStr.m_str
			, recvStr.SIZE);
		if (len > 0)
		{
			if (recvStr.SIZE > len)
				recvStr.m_str[len] = NULL;
			cout << recvStr.c_str();
		}

		Sleep(1);
	}

	return 1;
}


int main(char argc, char *argv[])
{
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		cout << "SetConsoleCtrlHandler failed, code : " << GetLastError() << endl;
		return -1;
	}

	if (argc > 1)
		g_portNum = atoi(argv[1]);

	std::thread thread = std::thread(MainThreadFunction);
	thread.join();

	return 0;
}
