// MIT License
// 
// Copyright(c) 2016 Michael J. Gleason
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include "Analysis.h"
#include "afxcmn.h"

// CAnalysisDlg dialog

class CAnalysisDlg : public CDialogEx
{
   DECLARE_DYNAMIC(CAnalysisDlg)

public:
   enum EAnalysisState
   {
      STATE_RUNNING,
      STATE_PAUSING,
      STATE_PAUSED,
      STATE_STOPPING,
      STATE_COMPLETED
   };

   CAnalysisDlg(CWnd* pParent = NULL);   // standard constructor
   virtual ~CAnalysisDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
   enum { IDD = IDD_ANALYSIS };
#endif

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();
   void UpdateDisplay();

   DECLARE_MESSAGE_MAP()
public:
   CString m_sStatus;
   CString m_sCurrentStatus;
   CString m_sProgressSummary;
   CProgressCtrl m_vProgress;
   afx_msg void OnBnClickedOK();
   afx_msg void OnBnClickedCancel();
   afx_msg void OnTimer(UINT_PTR nIDEvent);

   CEngineSettings m_vEngineSettings;
   int m_iGamesWithErrors;
   CArray<HANDLE, HANDLE> m_ahChildStdInRead;
   CArray<HANDLE, HANDLE> m_ahChildStdInWrite;
   CArray<HANDLE, HANDLE> m_ahChildStdOutRead;
   CArray<HANDLE, HANDLE> m_ahChildStdOutWrite;
   CArray<HANDLE, HANDLE> m_ahChildStdErrRead;
   CArray<HANDLE, HANDLE> m_ahChildStdErrWrite;
   CArray<HANDLE, HANDLE> m_ahProcesses;
   CArray<CGame, CGame> m_avGames;
   CString m_sStatusHistory;
   bool m_bShowResults;
   bool m_bCancelled;
   bool m_bStopped;
   bool m_bStatusChanged;
   int m_iProgressPercent;
   int m_iTargetThreads;
   int m_iMaxThreads;
   CString m_sConvertedPGN;
   CString m_sSavedResultsPath;
   CString m_sSavedPGNPath;
   CString m_sAutoSaveError;
   CString m_sAutoSavePGNError;
   CString m_sInputFilePath;
   EAnalysisState m_eState;
   void UpdateThreadControlButtons();
   CString GetStateLabel() const;
   void UpdateProgressDisplay(const CString& sPhase, int iCompletedGames, int iTotalGames, int iActiveProcesses, const CString& sLastEvent = _T(""));
   void RequestStop();
   void CancelActiveProcesses();
   bool AutoSaveOutputs(bool bPartialResults);
   bool ProcessGames();
   bool LaunchAnalyser(CGamePGN vGamePGN, int iCurThread);
   void ReadFromThread(int iThread, CString IN OUT &rsResults, bool IN OUT &rbError);
   bool ProcessOutput(CString sOutput);
   afx_msg void OnBnClickedDecreasethreads();
   afx_msg void OnBnClickedIncreasethreads();
   afx_msg void OnBnClickedPauseresume();
   afx_msg void OnBnClickedStopanalysis();
};
