#include <Windows.h>
#include <Windowsx.h>
#include <Tchar.h>
#include "resource.h"

#define APP_NAME _TEXT("Mini Clock")
#define APP_VER  _TEXT("0.0.0.1")

#define FONT_NAME _TEXT("MS UI Gothic")

TCHAR AppTitle[64];

HFONT hFont;

int tzid;
TIME_ZONE_INFORMATION tzinfo;

void SetMyFont(HWND hwndDlg)
{
	HWND hWnd = NULL;
	LOGFONT fnt;
	RtlZeroMemory(&fnt,sizeof(LOGFONT));
	fnt.lfHeight   = 25;
	fnt.lfWidth    = 0;
	fnt.lfWeight   = FW_BOLD;
	fnt.lfCharSet  = ANSI_CHARSET;
	RtlCopyMemory(fnt.lfFaceName,FONT_NAME,lstrlen(FONT_NAME)*sizeof(TCHAR));
	hFont = CreateFontIndirect(&fnt);
	hWnd = GetDlgItem(hwndDlg,IDC_EDIT1);
	SendMessage(hWnd,WM_SETFONT,(WPARAM)hFont,0);
	return;
}

VOID CALLBACK TimerProc(HWND hWnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	long long int t = 0;
	TCHAR current_time[64] = {0};
	TCHAR adj[32]          = {0};
	TCHAR ftime[32]        = {0};
	TCHAR tmtime[32]       = {0};
	SYSTEMTIME SysTime     = {0};
	FILETIME FileTime      = {0};

	if (BST_CHECKED == IsDlgButtonChecked(hWnd,IDC_LOCALTIME))
		GetLocalTime(&SysTime);
	else
		GetSystemTime(&SysTime);
	if (BST_CHECKED == IsDlgButtonChecked(hWnd,IDC_CHECKBOX1))
		wsprintf(current_time,
				_TEXT("%02d:%02d:%02d %02d"),
				SysTime.wHour,
				SysTime.wMinute,
				SysTime.wSecond,
				SysTime.wMilliseconds / 10
				);
	else
		wsprintf(current_time,
				_TEXT("%s %02d:%02d:%02d %02d"),
				(SysTime.wHour >= 12) ? _TEXT("PM") : _TEXT("AM"),
				(SysTime.wHour >= 12) ? (SysTime.wHour - 12) : (SysTime.wHour),
				SysTime.wMinute,
				SysTime.wSecond,
				SysTime.wMilliseconds / 10
				);
	if (BST_CHECKED == IsDlgButtonChecked(hWnd,IDC_LOCALTIME))
	{
		int bias = tzinfo.Bias;
		if (tzid == TIME_ZONE_ID_UNKNOWN ||
			tzid == TIME_ZONE_ID_STANDARD)
		{
			bias += tzinfo.StandardBias;
		}
		else
		{
			bias += tzinfo.DaylightBias;
		}
		lstrcat(current_time,(bias < 0) ? _TEXT(" UTC +") : _TEXT(" UTC -"));
		if (bias < 0)
			bias = -bias;
		wsprintf(adj,
				_TEXT("%02d:%02d"),
				bias / 60,
				bias % 60
				);
		lstrcat(current_time,adj);
	}
	SystemTimeToFileTime(&SysTime,&FileTime);
	wsprintf(ftime,
			_TEXT("0x%08X%08X"),
			FileTime.dwHighDateTime,
			FileTime.dwLowDateTime
			);
	t = FileTime.dwHighDateTime + 0xFE624E21;
	t <<= 32;
	t |= (FileTime.dwLowDateTime + 0x2AC18000);
	t /= 10000000;
	wsprintf(tmtime,
			_TEXT("0x%08X%08X"),
			(unsigned)(t >> 32),
			(unsigned)(t & 0x7FFFFFFF)
			);
	SetDlgItemText(hWnd,IDC_EDIT2,ftime);
	SetDlgItemText(hWnd,IDC_EDIT3,tmtime);
	SendMessage(GetDlgItem(hWnd,IDC_EDIT1),WM_SETTEXT,0,(LPARAM)current_time);
	return;
}

void CopyToClipboard(HWND hWnd)
{
	int len = 0;
	unsigned char *s = NULL;
	unsigned char *d = NULL;
	if (OpenClipboard(hWnd))
	{
		EmptyClipboard();
		len = GetWindowTextLength(hWnd);
		s = GlobalAlloc(GMEM_MOVEABLE,(len + 1)*sizeof(TCHAR));
		if (s)
		{
			d = GlobalLock(s);
			RtlZeroMemory(d,(len + 1)*sizeof(TCHAR));
			GetWindowText(hWnd,d,len + 1);
			GlobalUnlock(s);
		#ifdef UNICODE
			SetClipboardData(CF_UNICODETEXT,s);
		#else
			SetClipboardData(CF_TEXT,s);
		#endif
		}
		CloseClipboard();
	}
	return;
}

INT_PTR CALLBACK DlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	INT_PTR msgProcessed = FALSE;
	switch(uMsg)
	{
	case WM_CLOSE:
		if (hFont)
			DeleteObject(hFont);
		KillTimer(hwndDlg,1);
		EndDialog(hwndDlg,0);
		msgProcessed = TRUE;
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDM_COPY1:
			CopyToClipboard(GetDlgItem(hwndDlg,IDC_EDIT2));
			break;
		case IDM_COPY2:
			CopyToClipboard(GetDlgItem(hwndDlg,IDC_EDIT3));
			break;
		case IDM_EXIT:
			SendMessage(hwndDlg,WM_CLOSE,0,0);
			msgProcessed = TRUE;
			break;
		}
		break;
	case WM_CTLCOLORSTATIC:
		{
			HDC hDC = (HDC)wParam;
			if (lParam == GetDlgItem(hwndDlg,IDC_EDIT1))
			{
				SetBkMode(hDC,TRANSPARENT);
				SetTextColor(hDC,RGB(0,255,0));
				return GetStockObject(BLACK_BRUSH);
			}
		}
		break;
	case WM_INITDIALOG:
		tzid = GetTimeZoneInformation(&tzinfo);
		if (tzid != TIME_ZONE_ID_INVALID)
		{
			if (tzid == TIME_ZONE_ID_UNKNOWN ||
				tzid == TIME_ZONE_ID_STANDARD)
			{
				SetDlgItemTextW(hwndDlg,IDC_ZONEINFO,tzinfo.StandardName);
			}
			else
			{
				SetDlgItemTextW(hwndDlg,IDC_ZONEINFO,tzinfo.DaylightName);
			}
		}
		wsprintf(AppTitle,_TEXT("%s version: %s"),APP_NAME,APP_VER);
		SetWindowText(hwndDlg,AppTitle);
		SendMessage(hwndDlg,
					WM_SETICON,
					(WPARAM)ICON_BIG,
					(LPARAM)LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1))
					);
		SetMyFont(hwndDlg);
		Button_SetCheck(GetDlgItem(hwndDlg,IDC_CHECKBOX1),BST_CHECKED);
		Button_SetCheck(GetDlgItem(hwndDlg,IDC_LOCALTIME),BST_CHECKED);
		SetTimer(hwndDlg,1,10,TimerProc);
		break;
	}
	return msgProcessed;
}

int CALLBACK _tWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPTSTR lpCmdLine,int nCmdShow)
{
	return DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,DlgProc);
}