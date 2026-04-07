// MIT License
// 
// Copyright(c) 2016 Michael J. Gleason
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the _T("Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED _T("AS IS"), WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "stdafx.h"
#include "PGN Spy.h"
#include "PGN SpyDlg.h"
#include <afxvisualmanagerwindows.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPGNSpyApp

BEGIN_MESSAGE_MAP(CPGNSpyApp, CWinApp)
   ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CPGNSpyApp construction

CPGNSpyApp::CPGNSpyApp()
{
   // TODO: add construction code here,
   // Place all significant initialization in InitInstance
}


// The one and only CPGNSpyApp object

CPGNSpyApp theApp;


// CPGNSpyApp initialization

BOOL CPGNSpyApp::InitInstance()
{
   HMODULE hUser32 = ::GetModuleHandle(_T("user32.dll"));
   if (hUser32 != NULL)
   {
      typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(HANDLE);
      SetProcessDpiAwarenessContextProc pSetProcessDpiAwarenessContext =
         (SetProcessDpiAwarenessContextProc)::GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
      if (pSetProcessDpiAwarenessContext != NULL)
         pSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
   }

   // InitCommonControlsEx() is required on Windows XP if an application
   // manifest specifies use of ComCtl32.dll version 6 or later to enable
   // visual styles.  Otherwise, any window creation will fail.
   INITCOMMONCONTROLSEX InitCtrls;
   InitCtrls.dwSize = sizeof(InitCtrls);
   // Set this to include all the common control classes you want to use
   // in your application.
   InitCtrls.dwICC = ICC_WIN95_CLASSES | ICC_PROGRESS_CLASS;
   InitCommonControlsEx(&InitCtrls);

   CWinApp::InitInstance();

   AfxEnableControlContainer();
   CMFCVisualManagerWindows::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

   // Standard initialization
   // If you are not using these features and wish to reduce the size
   // of your final executable, you should remove from the following
   // the specific initialization routines you do not need
   // Change the registry key under which our settings are stored
   // TODO: You should modify this string to be something appropriate
   // such as the name of your company or organization
   SetRegistryKey(_T("PGNSpy"));
   LoadAppLanguageFromRegistry();

   FindDataFolder();

   CPGNSpyDlg dlg;
   m_pMainWnd = &dlg;
   INT_PTR nResponse = dlg.DoModal();

   // Since the dialog has been closed, return FALSE so that we exit the
   //  application, rather than start the application's message pump.
   return FALSE;
}

CString CPGNSpyApp::FindDataFolder()
{
   HMODULE hModule;
   if (AfxGetApp())
      hModule = AfxGetInstanceHandle();
   else
      hModule = GetModuleHandle(NULL);

   TCHAR szAppPath[_MAX_PATH] = { 0 };
   GetModuleFileName(hModule, szAppPath, _MAX_PATH);
   while (_tcslen(szAppPath) > 0 && szAppPath[_tcslen(szAppPath) - 1] != _T('\\'))
      szAppPath[_tcslen(szAppPath) - 1] = _T('\0');
   m_sDataFolder = CString(szAppPath);
   return m_sDataFolder;
}

CString GetConverterFilePath()
{
   return theApp.m_sDataFolder + _T("pgn-extract.exe");
}

CString GetAnalyserFilePath()
{
   return theApp.m_sDataFolder + _T("uci-analyser.exe");
}

CString GetConvertedPGNFilePath()
{
   TCHAR sTempPath[500] = { 0 };
   TCHAR sFilePath[500] = { 0 };
   GetTempPath(500, sTempPath);
   GetTempFileName(sTempPath, _T("PGN"), 0, sFilePath);
   return CString(sFilePath);
//    CTime vTime = CTime::GetCurrentTime();
//    return theApp.m_sDataFolder + vTime.Format(_T("Temp %y%m%d%H%M%S.pgn");
}

CString GetTemporaryPGNFilePath(int i)
{
   TCHAR sTempPath[500] = { 0 };
   TCHAR sFilePath[500] = { 0 };
   GetTempPath(500, sTempPath);
   GetTempFileName(sTempPath, _T("PGN"), 0, sFilePath);
   return CString(sFilePath);
//    CTime vTime = CTime::GetCurrentTime();
//    CString sCounter;
//    sCounter.Format(_T(" %i.pgn"), i);
//    return theApp.m_sDataFolder + vTime.Format(_T("Temp %y%m%d%H%M%S") + sCounter;
}

CString GetDefaultAnalysisResultsFilePath(const CString& sInputFilePath)
{
   CString sBaseFolder = sInputFilePath;
   int iLastSlash = sBaseFolder.ReverseFind('\\');
   if (iLastSlash != -1)
      sBaseFolder = sBaseFolder.Left(iLastSlash + 1);
   else
      sBaseFolder = theApp.m_sDataFolder;

   CString sFileName = sInputFilePath;
   if (iLastSlash != -1)
      sFileName = sFileName.Mid(iLastSlash + 1);
   int iExtension = sFileName.ReverseFind('.');
   if (iExtension != -1)
      sFileName = sFileName.Left(iExtension);

   SYSTEMTIME vTime;
   GetLocalTime(&vTime);
   CString sTimestamp;
   sTimestamp.Format(_T("%04i-%02i-%02i %02i-%02i-%02i"),
      vTime.wYear, vTime.wMonth, vTime.wDay, vTime.wHour, vTime.wMinute, vTime.wSecond);

   CString sBasePath = sBaseFolder + sFileName + _T(" analysis ") + sTimestamp;
   CString sCandidatePath = sBasePath + _T(".xml");
   int iCounter = 2;
   while (PathFileExists(sCandidatePath))
   {
      sCandidatePath.Format(_T("%s (%i).xml"), sBasePath, iCounter);
      iCounter++;
   }
   return sCandidatePath;
}



