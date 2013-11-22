// XGS Win32 Code

#define APPNAME "XGS"

#define WIN32_LEAN_AND_MEAN

// C RunTime Header Files

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

// Local Header Files
#include "xgs.h"
#include "memory.h"
#include "adb.h"
#include "adb-drv.h"
#include "disks.h"
#include "emul.h"
#include "sound.h"
#include "video.h"
#include "vid-drv.h"
#include "snd-drv.h"

#include "main.h"

// Makes it easier to determine appropriate code paths:
#if defined (WIN32)
	#define IS_WIN32 TRUE
#else
	#define IS_WIN32 FALSE
#endif
#define IS_NT      IS_WIN32 && (BOOL)(GetVersion() < 0x80000000)
#define IS_WIN32S  IS_WIN32 && (BOOL)(!(IS_NT) && (LOBYTE(LOWORD(GetVersion()))<4))
#define IS_WIN95 (BOOL)(!(IS_NT) && !(IS_WIN32S)) && IS_WIN32

// Global Variables:

HINSTANCE	hInst;					// current instance
char		szAppName[] = APPNAME;	// The name of this application
char		szTitle[]   = VERSION;	// The title bar text
HWND		hWnd;
HANDLE		hAccelTable;

char		*MyArgs[] = {
							"xgs",
							"-smpt0", "hd1.2mg",
							"-smpt1", "hd2.2mg"
						};
char        **dargv;
int			dargc;

BOOL			InitApplication(HINSTANCE,int);
long FAR PASCAL	WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CenterWindow (HWND hwnd);
BOOL APIENTRY CLDlgProc (HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
void UpdateMyControl(int, int, HWND, char *, FILE **);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	lpCmdLine = lpCmdLine;
	hPrevInstance = hPrevInstance;

	if (!InitApplication(hInstance,nCmdShow)) return FALSE;

	hAccelTable = LoadAccelerators (hInstance, szAppName);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	if (dargc == 0) return 0;
	if (EMUL_init(dargc, dargv)) return 0;

//	snd_enable = 0;

	EMUL_run();

	return 0;
}

BOOL InitApplication(HINSTANCE hInstance,int nCmdShow)
{
    WNDCLASS	wc;
	FILE		*hf;
	int			i;
	// For Dialog Box stuff...
	int		    retval, temp;
	struct stat buf;

	hInst = hInstance;

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon( hInstance, IDI_APPLICATION );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = NULL;
	wc.lpszMenuName = szAppName;
	wc.lpszClassName = szAppName;
	RegisterClass(&wc);

    hWnd = CreateWindowEx(
        WS_EX_TOPMOST,
        szAppName,
        szTitle,
        WS_POPUP,
        0, 0,
        GetSystemMetrics( SM_CXSCREEN ),
        GetSystemMetrics( SM_CYSCREEN ),
        NULL,
        NULL,
        hInstance,
		NULL );
	
	if (!hWnd) return FALSE;

	hf = fopen("bootlog.txt","w");
	*stdout = *hf;
	i = setvbuf( stdout, NULL, _IONBF, 0 );

	// Check for XGS dir
	retval = stat( XGS_DIR, &buf );
	if (retval) {
		printf ("XGS directory %s was not found.\n", XGS_DIR);
		return FALSE;
	}

	// Now do dialog box stuff...
	dargv = (char **)GlobalAlloc (GPTR, 35*sizeof(char*));
	printf("Popping up dialog...\n");
	dargc = DialogBoxParam (hInstance, "CL", NULL, CLDlgProc, (LPARAM) &dargv);
	
	if (dargc == -1) {
		dargc = GetLastError();
		printf ("Unable to create dialog: %d\n", dargc);
		return FALSE;
	}
	
	if (dargc > 0) {
		printf("Commandline was: ");
		for (temp = 0; temp < dargc; temp++)
			printf("%s ",dargv[temp]);
		printf("\n");
		printf("There were %d args.\n",dargc);
	}
	else
		printf("The cancel button was pressed.\n");

	// NOTE: The memory for the command line is never set free!!!

	return TRUE;
}

long FAR PASCAL WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	extern soundinfo mybuffers[];
	int i, done;

	switch (message) { 

		case MM_WOM_DONE:
			if (((LPWAVEHDR) lParam)->dwFlags == WHDR_DONE | WHDR_PREPARED) {
				i = done = 0;
				while ((i < SOUNDBUFFERS) && (!done)) {
					if ((LPWAVEHDR) lParam == mybuffers[i].lpWaveHdr) {
						mybuffers[i].busy = FALSE;
						done = 1;
					}
					i++;
				}	
			}
			break;


		case WM_CHAR:
			ADB_inputKeyDown((int) wParam, 0);
			break;

		case WM_KEYDOWN:
			ADB_inputKeyDown((int) wParam, 1);
			break;

		case WM_KEYUP:
			ADB_inputKeyRelease((int) wParam, 1);
			break;

		case WM_LBUTTONDOWN:
			ADB_inputLeftMouseDown();
			break;

		case WM_LBUTTONUP:
			ADB_inputLeftMouseUp();
			break;

		case WM_RBUTTONDOWN:
			ADB_inputRightMouseDown();
			break;

		case WM_RBUTTONUP:
			ADB_inputRightMouseUp();
			break;

		case WM_MOUSEMOVE:
			ADB_inputMotionNotify(LOWORD(lParam), HIWORD(lParam));
			break;

		case WM_PAINT:
			break;        

		case WM_COMMAND:
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			switch (wmId) {

				case IDM_ABOUT:
					break;

				case IDM_EXIT:
					DestroyWindow (hWnd);
					break;

				default:
					return (DefWindowProc(hWnd, message, wParam, lParam));
			}
			break;

		case WM_NCRBUTTONUP:
			if (IS_WIN95 && SendMessage(hWnd, WM_NCHITTEST, 0, lParam) == HTSYSMENU)
			{
				return DefWindowProc(hWnd, message, wParam, lParam);
			} else {
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
            break;

		case WM_SYSKEYUP:
			DefWindowProc(hWnd, message, wParam, lParam);
			break;

		case WM_SYSKEYDOWN:
			DefWindowProc(hWnd, message, wParam, lParam);
			break;

		case WM_DESTROY:
			EMUL_shutdown();
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// The following is for the dialog box stuff...
// A quick little routine that will center a window on the screen.
// Handy for dialog boxes
BOOL CenterWindow (HWND hwnd)
{
    RECT    rect;
    int     w, h;
    int     wScreen, hScreen, xNew, yNew;
    HDC     hdc;

    GetWindowRect (hwnd, &rect);
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;

    hdc = GetDC (hwnd);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwnd, hdc);

    xNew = wScreen/2 - w/2;
    yNew = hScreen/2 - h/2;

    return SetWindowPos (hwnd, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}


// Create a data structure that will hold the strings for the combo boxes
// we have in our dialog. This just illustrates 'a' way to do this, not
// necessarily the best.

typedef struct tagDlgCtrls {
    int ctrlId;
    int def;
	int type;
    char str[25];
    char opt[10];
} DlgCtrls;

DlgCtrls dlgctrls[] = {
	{ 420, FALSE, 2, "Trace unchecked", "trace"},
	
	{ 421, FALSE, 1, "1MB RAM", "ram 1"},
	{ 421, TRUE, 1, "2MB RAM", "ram 2"},
	{ 421, FALSE, 1, "3MB RAM", "ram 3"},
	{ 421, FALSE, 1, "4MB RAM", "ram 4"},
	{ 421, FALSE, 1, "5MB RAM", "ram 5"},
	{ 421, FALSE, 1, "6MB RAM", "ram 6"},
	{ 421, FALSE, 1, "7MB RAM", "ram 7"},
	{ 421, FALSE, 1, "8MB RAM", "ram 8"},

	{ 422, TRUE, 3, "C:\\XGS", ""},

	{ 423, TRUE, 4, "<None>", "s5d1"},
	{ 424, TRUE, 4, "<None>", "s5d2"},
	{ 425, TRUE, 4, "<None>", "s6d1"},
	{ 426, TRUE, 4, "<None>", "s6d2"},
	{ 427, TRUE, 4, "<None>", "smpt0"},
	{ 428, TRUE, 4, "<None>", "smpt1"},
	{ 429, TRUE, 4, "<None>", "smpt2"},
	{ 430, TRUE, 4, "<None>", "smpt3"},
	{ 431, TRUE, 4, "<None>", "smpt4"},
	{ 432, TRUE, 4, "<None>", "smpt5"},
	{ 433, TRUE, 4, "<None>", "smpt6"},

    { 0, 0}  // End Of List
};


BOOL APIENTRY CLDlgProc (HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int wmId;
    static char ***pargv;
    static char **argv;

    int i, j, item, index, iCtrl, argc;
    char *cmd;
    char *cmdline;
	char *tokenPtr;
	char *tempstring;
	char tempstring2[512];
	static char ImagePath[256];
	char f;
	FILE *fp1;
	int memval;

    switch (msg) {
        case WM_INITDIALOG:
            // We need to initialize stuff in the dialog box...

            pargv = (char ***)lParam;
            argv = *pargv;
            CenterWindow (hdlg);

			strcpy(ImagePath,XGS_DIR);
			strcpy(tempstring2, XGS_DIR);
			strcat(tempstring2, "\\xgs.ini");
			fp1 = fopen(tempstring2,"r");
            iCtrl = i = 0;
			memval = -1;
            while (dlgctrls[i].ctrlId) {
				iCtrl = dlgctrls[i].ctrlId;
				switch (dlgctrls[i].type) {
					case 1:
						index = SendDlgItemMessage (hdlg, iCtrl, CB_ADDSTRING, 0, (DWORD)(LPSTR)dlgctrls[i].str);
						SendDlgItemMessage (hdlg, iCtrl, CB_SETITEMDATA, index, i);
						if ((dlgctrls[i].def) && (fp1 == NULL)) {
							SendDlgItemMessage (hdlg, dlgctrls[i].ctrlId, CB_SETCURSEL, index, 0);
						}
						if (fp1 != NULL) {
							if (memval == -1) {
								fscanf(fp1, "%d", &memval);
								while ((f = getc(fp1)) != '\n');
							}
							if (memval == i) SendDlgItemMessage (hdlg, dlgctrls[i].ctrlId, CB_SETCURSEL, index, 0);
						}
						break;
					case 2:
						SendDlgItemMessage (hdlg, iCtrl, BM_SETCHECK, dlgctrls[i].def, 0);
						if (fp1 != NULL) {
							fscanf(fp1, "%d", &j);
							if (j) SendDlgItemMessage (hdlg, iCtrl, BM_SETCHECK, 1, 0);
							while ((f = getc(fp1)) != '\n');
						}
						break;
					case 3:
						SendDlgItemMessage (hdlg, iCtrl, WM_SETTEXT, 0, (DWORD)(LPSTR)dlgctrls[i].str);
						if (fp1 != NULL) {
							j = 0;
							while ((f = getc(fp1)) != '?') tempstring2[j++] = f;
							tempstring2[j] = 0;
							SendDlgItemMessage (hdlg, iCtrl, WM_SETTEXT, 0, (DWORD)(LPSTR)tempstring2);
							strcpy(ImagePath, tempstring2);
							while ((f = getc(fp1)) != '\n');
						}
						break;
					case 4:
						UpdateMyControl(i, iCtrl, hdlg, ImagePath, &fp1);
						break;
				}
                i++;
            }
			if (fp1 != NULL) fclose(fp1);
            return (TRUE);

        case WM_DESTROY:
            break;

        case WM_COMMAND:
            wmId = LOWORD(wParam);
            switch (wmId) {

                case IDOK:
                    cmd = cmdline = (char *)GlobalAlloc (GPTR, 2048);
					tempstring = (char *)GlobalAlloc (GPTR, 512);
                    argv[0] = cmdline;
                    argc = 0;
					
					wsprintf ((LPSTR)cmd, "XGS"); 
					cmd += strlen(cmd);
					cmd[0] = 0;
					argv[++argc] = ++cmd;

                    if (cmdline) {
                        iCtrl = i = 0;
                        while (dlgctrls[i].ctrlId) {
                            if (dlgctrls[i].ctrlId != iCtrl) {
                                iCtrl = dlgctrls[i].ctrlId;
								switch (dlgctrls[i].type) {
									case 1:
										index = SendDlgItemMessage(hdlg, iCtrl, CB_GETCURSEL, 0, 0);
										if (index) {
											item = SendDlgItemMessage (hdlg, iCtrl, CB_GETITEMDATA, index, 0);
											strcpy(tempstring, dlgctrls[item].opt);
											tokenPtr = strtok(tempstring, " ");
											wsprintf ((LPSTR)cmd, "-");
											cmd++;
											while (tokenPtr != NULL) {
												wsprintf ((LPSTR)cmd, "%s", (LPSTR)tokenPtr);
												cmd += strlen(cmd);
												cmd[0] = 0;
												argv[++argc] = ++cmd;
												tokenPtr = strtok(NULL, " ");
											}
										}
										break;
									case 2:
										if (SendDlgItemMessage (hdlg, iCtrl, BM_GETCHECK, 0, 0)) {
											strcpy(tempstring, dlgctrls[i].opt);
											tokenPtr = strtok(tempstring, " ");
											wsprintf ((LPSTR)cmd, "-");
											cmd++;
											while (tokenPtr != NULL) {
												wsprintf ((LPSTR)cmd, "%s", (LPSTR)tokenPtr);
												cmd += strlen(cmd);
												cmd[0] = 0;
												argv[++argc] = ++cmd;
												tokenPtr = strtok(NULL, " ");
											}
										}
										break;
									case 3:
/*										if (SendDlgItemMessage (hdlg, iCtrl, WM_GETTEXT, (DWORD)511, (DWORD)tempstring)) {
											tokenPtr = strtok(tempstring, " ");
											wsprintf ((LPSTR)cmd, "-%s", (LPSTR)dlgctrls[i].opt);
											cmd += strlen(cmd);
											cmd[0] = 0;
											argv[++argc] = ++cmd;
											while (tokenPtr != NULL) {
												wsprintf ((LPSTR)cmd, "%s", (LPSTR)tokenPtr);
												cmd += strlen(cmd);
												cmd[0] = 0;
												argv[++argc] = ++cmd;
												tokenPtr = strtok(NULL, " ");
											} 
										} */
										break;
									case 4:
										if (SendDlgItemMessage (hdlg, iCtrl, WM_GETTEXT, (DWORD)511, (DWORD)tempstring2)) {
											if (tempstring2[0] != '<') {
												tokenPtr = strtok(tempstring2, "-");
												tokenPtr++;
												// Add the following line back in to use a leading quotation mark
												// strcpy(tempstring, "\"");
												// Remove the following lines if you want a trailing quotation mark
												tokenPtr = strtok(tokenPtr, "\"");
												strcpy(tempstring, "");

												strcat(tempstring, ImagePath);
												strcat(tempstring, "\\");
												strcat(tempstring, tokenPtr);
												tokenPtr = strtok(tempstring, " ");
												wsprintf ((LPSTR)cmd, "-%s", (LPSTR)dlgctrls[i].opt);
												cmd += strlen(cmd);
												cmd[0] = 0;
												argv[++argc] = ++cmd;
												while (tokenPtr != NULL) {
													wsprintf ((LPSTR)cmd, "%s", (LPSTR)tokenPtr);
													cmd += strlen(cmd);
													cmd[0] = 0;
													argv[++argc] = ++cmd;
													tokenPtr = strtok(NULL, " ");
												}
											}
										}
										break;
								}
                            }
                            i++;
                        }

                    } // if (cmdline)...

                    EndDialog(hdlg, argc);
                    return (TRUE);

                case IDCANCEL:
                    EndDialog(hdlg, 0);
                    return (TRUE);

				case 500:  // Save
					iCtrl = i = 0;
					
					strcpy(tempstring2, XGS_DIR);
					strcat(tempstring2, "\\xgs.ini");
					if ((fp1 = fopen(tempstring2,"w")) != NULL) {
						while (dlgctrls[i].ctrlId) {
							if (dlgctrls[i].ctrlId != iCtrl) { // Starting a new list
								iCtrl = dlgctrls[i].ctrlId;
								switch (dlgctrls[i].type) {
									case 1:
										index = SendDlgItemMessage(hdlg, iCtrl, CB_GETCURSEL, 0, 0);
										item = SendDlgItemMessage (hdlg, iCtrl, CB_GETITEMDATA, index, 0);
										fprintf(fp1, "%d\n", item);
										break;
									case 2:
										if (SendDlgItemMessage (hdlg, iCtrl, BM_GETCHECK, 0, 0))
											fprintf(fp1, "1\n");
										else
											fprintf(fp1, "0\n");
										break;
									case 3:
										SendDlgItemMessage (hdlg, iCtrl, WM_GETTEXT, (DWORD)511, (DWORD)tempstring2);
										fprintf(fp1, "%s?\n", tempstring2);
										break;
									case 4:
										SendDlgItemMessage (hdlg, iCtrl, WM_GETTEXT, (DWORD)511, (DWORD)tempstring2);
										fprintf(fp1, "%s?\n", tempstring2);
										break;
								}
							}
							i++;
						}
					}
					fclose(fp1);
					return (TRUE);

				case 501:  // Apply
					if (SendDlgItemMessage (hdlg, 422, WM_GETTEXT, (DWORD)511, (DWORD)tempstring2)) {
						j = 0;
						while (tempstring2[j] != 0) j++;
						while ((tempstring2[j] == 0) || (tempstring2[j] == 32) || (tempstring2[j] == 92)) j--;
						tempstring2[j+1] = 0;
						strncpy(ImagePath, tempstring2, 255);
					}
					iCtrl = i = 0;
					while (dlgctrls[i].ctrlId) {
						iCtrl = dlgctrls[i].ctrlId;
						if (dlgctrls[i].type == 4) UpdateMyControl(i, iCtrl, hdlg, ImagePath, NULL);
						i++;
					}
					return (TRUE);
            }
            break;
    }
    return (FALSE);

    lParam; // unreferenced formal parameter
}

void UpdateMyControl(int i, int iCtrl, HWND hdlg, char *ImagePath, FILE **preffile) {
	char SearchPath[256];
	HANDLE hSearch;
	WIN32_FIND_DATA FileData;
	int index;
	BOOL fFinished;
	FILE *fp1;
	char FileName[35];
	char FileLabel[512];
	char FullFileName[512];
	char tempstring2[512];
	int j;
	char f;

	strcpy(SearchPath,ImagePath);
	strcat(SearchPath,"\\*.2mg");
	hSearch = FindFirstFile(SearchPath, &FileData);
	SendDlgItemMessage (hdlg, iCtrl, CB_RESETCONTENT, 0, 0);
	index = SendDlgItemMessage (hdlg, iCtrl, CB_ADDSTRING, 0, (DWORD)(LPSTR)dlgctrls[i].str);
	SendDlgItemMessage (hdlg, iCtrl, CB_SETITEMDATA, index, 0);
	SendDlgItemMessage (hdlg, dlgctrls[i].ctrlId, CB_SETCURSEL, index, 0);
	if (preffile != NULL) 
		if (*preffile != NULL) {
			j = 0;
			while ((f = getc(*preffile)) != '?') tempstring2[j++] = f;
			tempstring2[j] = 0;
			while ((f = getc(*preffile)) != '\n');
		}
	if (hSearch != INVALID_HANDLE_VALUE) {
		fFinished = FALSE;
		while (!fFinished) {
			strcpy(FileLabel,"\"");
			strcat(FileLabel,FileData.cFileName);
			strcat(FileLabel,"\" - \"");
			strcpy(FullFileName,ImagePath);
			strcat(FullFileName,"\\");
			strcat(FullFileName,FileData.cFileName);
			if ((fp1 = fopen(FullFileName,"r")) != NULL) {
				for (j = 1; j < 10; j++) getc(fp1);
				for (j = 0; j < 31; j++)
				FileName[j] = getc(fp1);
				FileName[31] = 0;
			}
			fclose(fp1);
			strcat(FileLabel,FileName);
			strcat(FileLabel,"\"");
			index = SendDlgItemMessage (hdlg, iCtrl, CB_ADDSTRING, 0, (DWORD)(LPSTR)FileLabel);
			if (preffile != NULL)
				if (*preffile != NULL) {
					if (strcmp(tempstring2, FileLabel) == 0)
					SendDlgItemMessage (hdlg, dlgctrls[i].ctrlId, CB_SETCURSEL, index, 0);
				}
			if (!FindNextFile(hSearch, &FileData))
				if (GetLastError() == ERROR_NO_MORE_FILES) fFinished = TRUE;
		}
	}
}

char *EMUL_expandPath(const char *path)
{
	if ((path[0] == '\\') || (path[1] == ':')) {
		strcpy(emul_path,path);
	} else {
		sprintf(emul_path,"%s\\%s",XGS_DIR,path);
	}
	return emul_path;
}

void APP_shutdown()
{
	ExitProcess(0);
}

long EMUL_getCurrentTime()
{
	return (long) GetCurrentTime();
}
