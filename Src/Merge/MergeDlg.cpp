
#include "stdafx.h"
#include "Merge.h"
#include "MergeDlg.h"
#include "afxdialogex.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const char *g_exts[] = { 
	"bil", "dds", "poi_base", "poi_bound", "facility_build", "xdo", "jpg", "poi", "dat" 
};

struct sFileData {
	uint64 loc; // level + yloc + ext
	uint64 file1; // filename1
	uint64 file2; // filename2

	bool operator < (const sFileData &rhs) const {
		return ((loc == rhs.loc) && (file1 == rhs.file1) && (file2 < rhs.file2))
			|| ((loc == rhs.loc) && (file1 < rhs.file1))
			|| (loc < rhs.loc);
	}
};

int SearchFilesProcess(CMergeDlg *dlg);

void CollectDataFiles(const StrPath &dir, const int type
	, const int depth, const uint64 key
	, INOUT set<uint64> &visits
	, INOUT int &level
	, INOUT int &yloc
	, OUT int &cnt
	, OUT set<sFileData> &out);


CMergeDlg::CMergeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MERGE_DIALOG, pParent)
	, m_progressStr(_T(""))
	, m_curCnt(0)
	, m_totalCnt(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CMergeDlg::~CMergeDlg()
{
}

void CMergeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_TOTAL, m_totalProgress);
	DDX_Text(pDX, IDC_STATIC_PROGRESS, m_progressStr);
}

BEGIN_MESSAGE_MAP(CMergeDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CMergeDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CMergeDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_START, &CMergeDlg::OnBnClickedButtonStart)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CMergeDlg message handlers
BOOL CMergeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	//m_srcDirs.push_back("F:/data/");
	m_dstDir = "D:/Media/data/";

	SetTimer(0, 1000, NULL);

	return TRUE;
}

void CMergeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CMergeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMergeDlg::OnBnClickedOk()
{
	m_isSearchLoop = false;
	if (m_searchThread.joinable())
		m_searchThread.join();
	KillTimer(0);
	CDialogEx::OnOK();
}


void CMergeDlg::OnBnClickedCancel()
{
	m_isSearchLoop = false;
	if (m_searchThread.joinable())
		m_searchThread.join();

	KillTimer(0);
	CDialogEx::OnCancel();
}


void CMergeDlg::OnBnClickedButtonStart()
{
	m_isSearchLoop = false;
	if (m_searchThread.joinable())
		m_searchThread.join();

	// Get Source FolderCount
	m_totalCnt = 0;
	for (auto &str : m_srcDirs)
	{
		set<sFileData> srcFiles1;
		set<uint64> visits1;
		int level1 = 0, yloc1 = 0;
		CollectDataFiles(str.c_str(), 2, 0, 0, visits1, level1, yloc1, m_totalCnt, srcFiles1);
	}

	m_totalProgress.SetRange32(0, m_totalCnt);

	m_isSearchLoop = true;
	m_searchThread = std::thread(SearchFilesProcess, this);
}


void CMergeDlg::OnTimer(UINT_PTR nIDEvent)
{
	m_progressStr.Format(L"%d/%d", m_curCnt, m_totalCnt);
	UpdateData(FALSE);

	CDialogEx::OnTimer(nIDEvent);
}


// filename format: yloc_xloc.ext
// return ext, filename to uint64
// ext: 0: bil
//		1: dds
//		2: poi_base
//		3: poi_bound
//		4: facility_build
//		5: xdo
//		6: jpg
//		7: poi
//		8: dat
std::tuple<int,uint64,uint64> GetFileKey(const char *fileName)
{
	int state = 0;
	int ext = -1;
	uint64 val1 = 0;
	uint64 val2 = 0;
	int cnt = 0;
	const char *c = fileName;
	while (*c)
	{
		if (*c == '.')
		{
			++c;
			for (int i = 0; i < ARRAYSIZE(g_exts); ++i)
			{
				if (!strcmp(c, g_exts[i]))
				{
					ext = i;
					break;
				}
			}

			if (ext < 0)
			{
				assert(0);
				return { -1, 0, 0 };
			}
			break;
		}

		if (cnt < 8)
		{
			val1 <<= 8;
			val1 += *c;
		}
		else
		{
			val2 <<= 8;
			val2 += *c;
		}
		++c;
		++cnt;
	}

	return { ext, val1, val2 };
}


// dir : must contain last character '/'
//		 '/' slash directory separator
// type 0: read mode
//		1: match mode
//		2: count folder
void CollectDataFiles(const StrPath &dir, const int type
	, const int depth, const uint64 key
	, INOUT set<uint64> &visits
	, INOUT int &level
	, INOUT int &yloc
	, OUT int &cnt
	, OUT set<sFileData> &out)
{
	WIN32_FIND_DATAA fd;
	const StrPath searchDir = dir + "*.*";
	HANDLE hFind = FindFirstFileA(searchDir.c_str(), &fd);

	while (1)
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if ((string(".") != fd.cFileName) && (string("..") != fd.cFileName))
			{
				// only one execute
				if (!out.empty())
					return;

				const int n = atoi(fd.cFileName);
				uint64 k = key * 1000000;
				k += n;

				// match mode
				if (type == 1)
				{
					if ((depth == 0) && (level != n))
					{
						if (!FindNextFileA(hFind, &fd))
							break;
						continue;
					}
					else if ((depth == 1) && (yloc != n))
					{
						if (!FindNextFileA(hFind, &fd))
							break;
						continue;
					}
				}

				if ((type == 2) && (depth == 1))
				{
					++cnt;
					if (!FindNextFileA(hFind, &fd))
						break;
					continue;
				}

				if (depth == 0)
					level = n;
				if (depth == 1)
					yloc = n;

				if (depth == 1)
				{
					if (visits.end() == visits.find(k))
					{
						visits.insert(k);
						CollectDataFiles(dir + StrPath(fd.cFileName) + "/", type
							, depth + 1, k
							, visits, level, yloc, cnt, out);
						return;
					}
				}
				else
				{
					CollectDataFiles(dir + StrPath(fd.cFileName) + "/", type
						, depth + 1, k
						, visits, level, yloc, cnt, out);
				}
			}
		}
		else if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
		{
			const auto ret = GetFileKey(fd.cFileName);
			if (std::get<0>(ret) < 0)
			{
				assert(0);
				break;
			}

			sFileData data;
			data.loc = (key * 100) + std::get<0>(ret);
			data.file1 = std::get<1>(ret);
			data.file2 = std::get<2>(ret);
			out.insert(data);
		}

		if (!FindNextFileA(hFind, &fd))
			break;
	}
}


// 원본 파일을 dstDir에 복사한다.
bool CopySrc2Dst(const char *srcDir, const sFileData &file, const char *dstDir)
{
	const uint64 level = file.loc / 100000000;
	const uint64 yloc = (file.loc / 100) % 1000000;
	const uint64 ext = file.loc % 100;

	string fileName;
	fileName.reserve(19);
	uint64 v1 = file.file1;
	uint64 v2 = file.file2;
	
	int state = 0;
	int cnt = 0;
	while (cnt < 16)
	{
		char c = 0;
		if (state == 0)
		{
			c = (v2 & 0xFF);
			v2 >>= 8;
		}
		else
		{
			c = (v1 & 0xFF);
			v1 >>= 8;
		}

		++cnt;
		if (c == NULL)
		{
			if (state == 1)
			{
				break;
			}
			else
			{
				++state;
				continue;
			}
		}

		fileName += c;
	}

	std::reverse(fileName.begin(), fileName.end());
	fileName += '.';
	fileName += g_exts[ext];

	StrPath srcFullFileName;
	srcFullFileName.Format("%s%d/%04d/", srcDir, (int)level, (int)yloc);
	srcFullFileName += fileName.c_str();
	StrPath dstFullFileName;
	dstFullFileName.Format("%s%d/%04d/", dstDir, (int)level, (int)yloc);
	dstFullFileName += fileName.c_str();

	CopyFileA(srcFullFileName.c_str(), dstFullFileName.c_str(), FALSE);

	return true;
}


// 파일 검사
int SearchFilesProcess(CMergeDlg *dlg)
{
	const vector<string> &srcDirs = dlg->m_srcDirs;
	const string &dstDir = dlg->m_dstDir;

	dlg->m_curCnt = 0;
	for (auto &srcDir : srcDirs)
	{
		set<uint64> visits1;
		while (dlg->m_isSearchLoop)
		{
			set<sFileData> srcFiles1;
			int level1 = 0, yloc1 = 0, tmp = 0;
			CollectDataFiles(srcDir.c_str(), 0, 0, 0, visits1, level1, yloc1, tmp, srcFiles1);

			set<uint64> visits2;
			set<sFileData> dstFiles;
			CollectDataFiles(dstDir.c_str(), 1, 0, 0, visits2, level1, yloc1, tmp, dstFiles);

			if (srcFiles1.empty())
				break;

			// copy source to dest
			// make directory
			StrPath dstFolder;
			dstFolder.Format("%s%d/%04d", dstDir.c_str(), level1, yloc1);
			if (!dstFolder.IsFileExist())
				_mkdir(dstFolder.c_str());

			for (auto &file : srcFiles1)
			{
				auto it = dstFiles.find(file);
				if (dstFiles.end() == it)
				{
					CopySrc2Dst(srcDir.c_str(), file, dstDir.c_str());
				}
			}

			if (dlg->m_isSearchLoop)
				dlg->m_totalProgress.SetPos(dlg->m_curCnt++);
		}
	}

	return 0;
}

