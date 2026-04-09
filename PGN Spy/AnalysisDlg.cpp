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

// AnalysisDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGN Spy.h"
#include "AnalysisDlg.h"
#include "afxdialogex.h"


// CAnalysisDlg dialog

IMPLEMENT_DYNAMIC(CAnalysisDlg, CDialogEx)

CAnalysisDlg::CAnalysisDlg(CWnd* pParent /*=NULL*/)
   : CDialogEx(IDD_ANALYSIS, pParent)
   , m_sStatus(_T(""))
   , m_sCurrentStatus(_T(""))
   , m_sProgressSummary(_T(""))
   , m_iGamesWithErrors(0)
   , m_bShowResults(false)
   , m_bCancelled(false)
   , m_bStopped(false)
   , m_bStatusChanged(true)
   , m_iProgressPercent(0)
   , m_iTargetParallelGames(0)
   , m_iMaxParallelGames(0)
   , m_eState(STATE_RUNNING)
{

}

CAnalysisDlg::~CAnalysisDlg()
{
   CancelActiveProcesses();
}

void CAnalysisDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialogEx::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_STATUS, m_sStatus);
   DDX_Text(pDX, IDC_ANALYSISSTATUS, m_sCurrentStatus);
   DDX_Text(pDX, IDC_PROGRESSSUMMARY, m_sProgressSummary);
   DDX_Control(pDX, IDC_ANALYSISPROGRESS, m_vProgress);
}


BEGIN_MESSAGE_MAP(CAnalysisDlg, CDialogEx)
   ON_BN_CLICKED(IDOK, &CAnalysisDlg::OnBnClickedOK)
   ON_BN_CLICKED(IDCANCEL, &CAnalysisDlg::OnBnClickedCancel)
   ON_WM_TIMER()
   ON_BN_CLICKED(IDC_DECREASETHREADS, &CAnalysisDlg::OnBnClickedDecreasethreads)
   ON_BN_CLICKED(IDC_INCREASETHREADS, &CAnalysisDlg::OnBnClickedIncreasethreads)
   ON_BN_CLICKED(IDC_PAUSERESUME, &CAnalysisDlg::OnBnClickedPauseresume)
   ON_BN_CLICKED(IDC_STOPANALYSIS, &CAnalysisDlg::OnBnClickedStopanalysis)
END_MESSAGE_MAP()


// CAnalysisDlg message handlers
BOOL CAnalysisDlg::OnInitDialog()
{
   CDialog::OnInitDialog();
   ApplyDialogTranslations(this, IDD_ANALYSIS);

   m_vProgress.SetRange32(0, 100);
   m_vProgress.SetPos(0);
   m_sCurrentStatus = Loc(_T("Preparing analysis..."), _T("Подготовка анализа..."));
   int iEngineThreads = max(m_vEngineSettings.m_iEngineThreads, 1);
   m_sProgressSummary.Format(
      Loc(_T("0 of 0 games completed. Active analysers: 0. Engine threads per analyser: %i. Total engine threads: 0."),
          _T("Завершено 0 из 0 партий. Активных анализаторов: 0. Потоков движка на анализатор: %i. Всего потоков движка: 0.")),
      iEngineThreads);
   UpdateData(FALSE);
   UpdateThreadControlButtons();

   //use a timer - otherwise the status display doesn't seem to show up
   //maybe use OnPaint instead of OnInitDialog to avoid that?  Whatever, the timer works.
   SetTimer(0, 50, NULL);

   return TRUE;
}

void CAnalysisDlg::OnTimer(UINT_PTR nIDEvent)
{
   KillTimer(nIDEvent);

   CWaitCursor vWaitCursor;

   m_iTargetParallelGames = m_vEngineSettings.m_iParallelGames;

   SYSTEM_INFO vSysInfo;
   GetSystemInfo(&vSysInfo);
   m_iMaxParallelGames = (int)vSysInfo.dwNumberOfProcessors;
   UpdateThreadControlButtons();

   //do stuff
   ProcessGames();

   CDialog::OnTimer(nIDEvent);

   OnOK();//close the dialog
}

void CAnalysisDlg::UpdateThreadControlButtons()
{
   bool bRunningState = m_eState == STATE_RUNNING;
   GetDlgItem(IDC_INCREASETHREADS)->EnableWindow(bRunningState && m_iTargetParallelGames < m_iMaxParallelGames);
   GetDlgItem(IDC_DECREASETHREADS)->EnableWindow(bRunningState && m_iTargetParallelGames > 1);

   CString sPauseResumeText = (m_eState == STATE_PAUSED || m_eState == STATE_PAUSING)
      ? Loc(_T("Resume"), _T("Продолжить"))
      : Loc(_T("Pause"), _T("Пауза"));
   GetDlgItem(IDC_PAUSERESUME)->SetWindowText(sPauseResumeText);
   GetDlgItem(IDC_PAUSERESUME)->EnableWindow(m_eState != STATE_STOPPING && m_eState != STATE_COMPLETED);
   GetDlgItem(IDC_STOPANALYSIS)->EnableWindow(m_eState != STATE_STOPPING && m_eState != STATE_COMPLETED);
}

CString CAnalysisDlg::GetStateLabel() const
{
   switch (m_eState)
   {
   case STATE_PAUSING:
      return Loc(_T("Pausing"), _T("Пауза запрашивается"));
   case STATE_PAUSED:
      return Loc(_T("Paused"), _T("Приостановлено"));
   case STATE_STOPPING:
      return Loc(_T("Stopping"), _T("Остановка"));
   case STATE_COMPLETED:
      return Loc(_T("Completed"), _T("Завершено"));
   default:
      return Loc(_T("Running"), _T("Выполняется"));
   }
}

void CAnalysisDlg::OnBnClickedOK()
{
   //do nothing if they press enter
}

void CAnalysisDlg::OnBnClickedCancel()
{
   OnBnClickedStopanalysis();
}

void CAnalysisDlg::UpdateDisplay()
{
   if (m_bStatusChanged)
   {
      UpdateData(FALSE);
      m_vProgress.SetPos(m_iProgressPercent);
   }
   m_bStatusChanged = false;

   MSG msg;
   while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
   {
      if (!AfxGetApp()->PreTranslateMessage(&msg))
      {
         ::TranslateMessage(&msg);
         ::DispatchMessage(&msg);
      }
   }
}

void CAnalysisDlg::UpdateProgressDisplay(const CString& sPhase, int iCompletedGames, int iTotalGames, int iActiveProcesses, const CString& sLastEvent)
{
   m_sCurrentStatus = GetStateLabel() + _T(": ") + sPhase;
   if (iTotalGames > 0)
      m_iProgressPercent = (iCompletedGames * 100) / iTotalGames;
   else
      m_iProgressPercent = 0;

   int iEngineThreads = max(m_vEngineSettings.m_iEngineThreads, 1);
   int iTotalEngineThreads = iActiveProcesses * iEngineThreads;
   m_sProgressSummary.Format(
      Loc(_T("%i of %i games completed. Active analysers: %i. Engine threads per analyser: %i. Total engine threads: %i."),
          _T("Завершено %i из %i партий. Активных анализаторов: %i. Потоков движка на анализатор: %i. Всего потоков движка: %i.")),
      iCompletedGames, iTotalGames, iActiveProcesses, iEngineThreads, iTotalEngineThreads);
   if (!sLastEvent.IsEmpty())
      m_sProgressSummary += _T(" ") + sLastEvent;

   m_bStatusChanged = true;
   UpdateThreadControlButtons();
   UpdateDisplay();
}

bool CAnalysisDlg::ProcessGames()
{
   CStdioFile vConvertedPGN;
   if (!vConvertedPGN.Open(m_sConvertedPGN, CFile::modeRead))
   {
      CString sMessage = Loc(_T("Failed to open converted PGN for analysis."), _T("Не удалось открыть подготовленный PGN для анализа."));
      MessageBox(sMessage, GetAppTitle(), MB_ICONEXCLAMATION);
      return false;
   }

   m_sStatusHistory = Loc(_T("Reading PGN file..."), _T("Чтение PGN-файла..."));
   m_sStatus = m_sStatusHistory;
   UpdateProgressDisplay(Loc(_T("Reading converted PGN..."), _T("Чтение подготовленного PGN...")), 0, 0, 0);
   //Pull games out of file
   bool bKeepLooping = true;
   CArray<CGamePGN, CGamePGN> avGamePGNs;
   while (bKeepLooping)
   {
      CString sLine;
      if (vConvertedPGN.ReadString(sLine))
      {
         if (avGamePGNs.GetSize() == 0 || sLine.Left(8).CompareNoCase(_T("[Event \")")) == 0)
         {
            //found a new game
            CGamePGN vGamePGN;
            vGamePGN.m_sPGNText = sLine + _T("\n");
            avGamePGNs.Add(vGamePGN);
         }
         else
         {
            //still loading the latest game
            avGamePGNs[avGamePGNs.GetUpperBound()].m_sPGNText.Append(sLine + _T("\n"));
            if (sLine.Left(8).CompareNoCase(_T("[White \"")) == 0)
            {
               //found white player name
               int iLeftQuote, iRightQuote;
               iLeftQuote = sLine.Find(_T("\""), 0);
               iRightQuote = sLine.Find(_T("\""), iLeftQuote + 1);
               avGamePGNs[avGamePGNs.GetUpperBound()].m_sWhite = sLine.Mid(iLeftQuote + 1, iRightQuote - iLeftQuote - 1);
            }
            if (sLine.Left(8).CompareNoCase(_T("[Black \"")) == 0)
            {
               //found black player name
               int iLeftQuote, iRightQuote;
               iLeftQuote = sLine.Find(_T("\""), 0);
               iRightQuote = sLine.Find(_T("\""), iLeftQuote + 1);
               avGamePGNs[avGamePGNs.GetUpperBound()].m_sBlack = sLine.Mid(iLeftQuote + 1, iRightQuote - iLeftQuote - 1);
            }
         }
      }
      else
      {
         vConvertedPGN.Close();
         bKeepLooping = false;
      }
   }

   UpdateProgressDisplay(_T("Preparing game list..."), 0, avGamePGNs.GetSize(), 0);

   //Check if any games should be skipped
   if (!m_vEngineSettings.m_sPlayerName.IsEmpty())
   {
      for (int i = 0; i < avGamePGNs.GetSize(); i++)
      {
         if (avGamePGNs[i].m_sWhite.CompareNoCase(m_vEngineSettings.m_sPlayerName) != 0 &&
            avGamePGNs[i].m_sBlack.CompareNoCase(m_vEngineSettings.m_sPlayerName) != 0)
         {
            avGamePGNs.RemoveAt(i);
            i--;
         }
      }
   }

   if (avGamePGNs.GetSize() == 0)
   {
      CString sMessage = _T("No games to analyse.  If you have entered a player name, please ensure it is spelled correctly.");
      MessageBox(sMessage, _T("PGN Spy"), MB_ICONINFORMATION);
      return true;
   }

   m_sStatusHistory = _T("Creating temporary files...\r\n") + m_sStatusHistory;
   m_sStatus = m_sStatusHistory;
   UpdateProgressDisplay(_T("Creating temporary PGN files in %TEMP%..."), 0, avGamePGNs.GetSize(), 0);
   //We now have a complete list of games; dump them to files
   for (int i = 0; i < avGamePGNs.GetSize(); i++)
   {
      avGamePGNs[i].m_sFileName = GetTemporaryPGNFilePath(i);
      CFile vFile;
      if (!vFile.Open(avGamePGNs[i].m_sFileName, CFile::modeCreate | CFile::modeWrite))
      {
         CString sMessage = _T("Failed to create temporary output file.");
         MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
         return false;
      }
      if (!WriteCStringToFile(vFile, avGamePGNs[i].m_sPGNText))
      {
         CString sMessage = _T("Failed to write temporary output file.");
         MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
         vFile.Close();
         return false;
      }
      vFile.Close();
   }

   //We now have a complete set of files; time to process them

   CStringArray asResults;
   CArray<bool, bool> abErrors;
   asResults.SetSize(m_vEngineSettings.m_iParallelGames);
   abErrors.SetSize(m_vEngineSettings.m_iParallelGames);
   //initialize handles
   m_ahChildStdInRead.SetSize(m_vEngineSettings.m_iParallelGames);
   m_ahChildStdInWrite.SetSize(m_vEngineSettings.m_iParallelGames);
   m_ahChildStdOutRead.SetSize(m_vEngineSettings.m_iParallelGames);
   m_ahChildStdOutWrite.SetSize(m_vEngineSettings.m_iParallelGames);
   m_ahChildStdErrRead.SetSize(m_vEngineSettings.m_iParallelGames);
   m_ahChildStdErrWrite.SetSize(m_vEngineSettings.m_iParallelGames);
   m_ahProcesses.SetSize(m_vEngineSettings.m_iParallelGames);
   m_ahJobObjects.SetSize(m_vEngineSettings.m_iParallelGames);
   for (int i = 0; i < m_vEngineSettings.m_iParallelGames; i++)
   {
      m_ahChildStdInRead[i] = NULL;
      m_ahChildStdInWrite[i] = NULL;
      m_ahChildStdOutRead[i] = NULL;
      m_ahChildStdOutWrite[i] = NULL;
      m_ahChildStdErrRead[i] = NULL;
      m_ahChildStdErrWrite[i] = NULL;
      m_ahProcesses[i] = NULL;
      m_ahJobObjects[i] = NULL;
      abErrors[i] = false;
   }

   int iActiveProcesses = 0;
   int iNextGame = 0;
   int iCompletedGames = 0;
   bool bThreadsStillRunning = false;
   m_bStatusChanged = true;
   while ((iNextGame < avGamePGNs.GetSize() || bThreadsStillRunning || m_eState == STATE_PAUSED || m_eState == STATE_PAUSING) && !m_bCancelled)
   {
      if (m_eState == STATE_STOPPING)
      {
         CancelActiveProcesses();
         iActiveProcesses = 0;
         bThreadsStillRunning = false;
         break;
      }

      if (m_eState == STATE_PAUSING && iActiveProcesses == 0)
         m_eState = STATE_PAUSED;

      CString sPhase;
      if (m_eState == STATE_PAUSED)
         sPhase = _T("Analysis paused. Waiting to resume...");
      else if (m_eState == STATE_PAUSING)
         sPhase = _T("Waiting for active games to finish before pausing...");
      else if (avGamePGNs.GetSize() > 0)
         sPhase.Format(_T("Analysing game %i of %i..."), min(iNextGame + 1, avGamePGNs.GetSize()), avGamePGNs.GetSize());
      else
         sPhase = _T("Analysing games...");
      m_sStatus = m_sStatusHistory;
      UpdateProgressDisplay(sPhase, iCompletedGames, avGamePGNs.GetSize(), iActiveProcesses);

      bThreadsStillRunning = false; //we'll turn this on later if needed
                                    //check all processes to see if they're currently running
      for (int iCurThread = 0; iCurThread < m_vEngineSettings.m_iParallelGames; iCurThread++)
      {
         //if current process is free, kick off another process
         if (m_ahProcesses[iCurThread] == NULL)
         {
            if (m_eState != STATE_RUNNING)
               continue;
            if (iNextGame >= avGamePGNs.GetSize())
               continue; //don't start a new process if no games left
            abErrors[iCurThread] = false;
            //kick off process with next game
            if (!LaunchAnalyser(avGamePGNs[iNextGame], iCurThread))
            {
               //Failed to launch analyser; bail out
               return false;
            }

            iActiveProcesses++;
            bThreadsStillRunning = true; //we're launching a new thread
            iNextGame++;
            CString sStatusLine;
            sStatusLine.Format(_T("Started analysis for %s v %s"), avGamePGNs[iNextGame - 1].m_sWhite, avGamePGNs[iNextGame - 1].m_sBlack);
            m_sStatusHistory = sStatusLine + _T("\r\n") + m_sStatusHistory;
            m_sStatus = m_sStatusHistory;
            UpdateProgressDisplay(sPhase, iCompletedGames, avGamePGNs.GetSize(), iActiveProcesses, sStatusLine);
            //bail out of loop; we'll check other processes next time through
            break;
         }
         //check if current process is complete
         else if (WaitForSingleObject(m_ahProcesses[iCurThread], 0) == WAIT_OBJECT_0)
         {
            //the process has completed; read all results
            ReadFromThread(iCurThread, asResults[iCurThread], abErrors[iCurThread]);

            //close handles
            CloseThreadResources(iCurThread);
            iActiveProcesses--;
            iCompletedGames++;

            CString sStatusLine;
            //we have our results; now parse them
            if (abErrors[iCurThread])
            {
               m_iGamesWithErrors++; //skip games with errors
               sStatusLine = _T("Encountered game with error");
            }
            else
            {
               if (ProcessOutput(asResults[iCurThread]))
                  sStatusLine.Format(_T("Finished game: %s v %s"), m_avGames[m_avGames.GetUpperBound()].m_sWhite, m_avGames[m_avGames.GetUpperBound()].m_sBlack);
               else
                  sStatusLine = _T("Encountered game with error");
            }
            m_sStatusHistory = sStatusLine + _T("\r\n") + m_sStatusHistory;
            asResults[iCurThread].Empty();
            abErrors[iCurThread] = false;
            m_sStatus = m_sStatusHistory;
            UpdateProgressDisplay(_T("Collecting analysis results..."), iCompletedGames, avGamePGNs.GetSize(), iActiveProcesses, sStatusLine);

            //check if we're supposed to be decrementing threads
            if (m_iTargetParallelGames < m_vEngineSettings.m_iParallelGames)
            {
               //remove array members for current thread
               asResults.RemoveAt(iCurThread);
               abErrors.RemoveAt(iCurThread);
               m_ahChildStdInRead.RemoveAt(iCurThread);
               m_ahChildStdInWrite.RemoveAt(iCurThread);
               m_ahChildStdOutRead.RemoveAt(iCurThread);
               m_ahChildStdOutWrite.RemoveAt(iCurThread);
               m_ahChildStdErrRead.RemoveAt(iCurThread);
               m_ahChildStdErrWrite.RemoveAt(iCurThread);
               m_ahProcesses.RemoveAt(iCurThread);
               m_ahJobObjects.RemoveAt(iCurThread);

               //decrement thread count
               m_vEngineSettings.m_iParallelGames--;
            }
         }
         else
         {
            bThreadsStillRunning = true; //current thread is still running

                                         //read everything written so far to keep the pipe from filling up and locking the analyser
            ReadFromThread(iCurThread, asResults[iCurThread], abErrors[iCurThread]);
         }
      }

      if (m_eState == STATE_PAUSING && iActiveProcesses == 0)
      {
         m_eState = STATE_PAUSED;
         UpdateProgressDisplay(_T("Analysis paused. Waiting to resume..."), iCompletedGames, avGamePGNs.GetSize(), iActiveProcesses);
      }

      Sleep(50); //so we don't eat lots of CPU trying to check on every process constantly

      //check if we've incremented the thread count
      if (m_eState == STATE_RUNNING && m_iTargetParallelGames > m_vEngineSettings.m_iParallelGames)
      {
         m_vEngineSettings.m_iParallelGames++;
         //add a member to arrays and initialize values
         asResults.SetSize(m_vEngineSettings.m_iParallelGames);
         abErrors.SetSize(m_vEngineSettings.m_iParallelGames);
         //initialize handles
         m_ahChildStdInRead.SetSize(m_vEngineSettings.m_iParallelGames);
         m_ahChildStdInWrite.SetSize(m_vEngineSettings.m_iParallelGames);
         m_ahChildStdOutRead.SetSize(m_vEngineSettings.m_iParallelGames);
         m_ahChildStdOutWrite.SetSize(m_vEngineSettings.m_iParallelGames);
         m_ahChildStdErrRead.SetSize(m_vEngineSettings.m_iParallelGames);
         m_ahChildStdErrWrite.SetSize(m_vEngineSettings.m_iParallelGames);
         m_ahProcesses.SetSize(m_vEngineSettings.m_iParallelGames);
         m_ahJobObjects.SetSize(m_vEngineSettings.m_iParallelGames);
         m_ahChildStdInRead[m_vEngineSettings.m_iParallelGames - 1] = NULL;
         m_ahChildStdInWrite[m_vEngineSettings.m_iParallelGames - 1] = NULL;
         m_ahChildStdOutRead[m_vEngineSettings.m_iParallelGames - 1] = NULL;
         m_ahChildStdOutWrite[m_vEngineSettings.m_iParallelGames - 1] = NULL;
         m_ahChildStdErrRead[m_vEngineSettings.m_iParallelGames - 1] = NULL;
         m_ahChildStdErrWrite[m_vEngineSettings.m_iParallelGames - 1] = NULL;
         m_ahProcesses[m_vEngineSettings.m_iParallelGames - 1] = NULL;
         m_ahJobObjects[m_vEngineSettings.m_iParallelGames - 1] = NULL;
         abErrors[m_vEngineSettings.m_iParallelGames - 1] = false;
      }
   }

   bool bPartialResults = m_bStopped;
   m_eState = STATE_COMPLETED;
   UpdateProgressDisplay(bPartialResults ? _T("Cleaning up after stop...") : _T("Cleaning up temporary files..."), iCompletedGames, avGamePGNs.GetSize(), 0);

   //clean up temporary files
   for (int i = 0; i < avGamePGNs.GetSize(); i++)
      DeleteFile(avGamePGNs[i].m_sFileName);

   m_bShowResults = m_avGames.GetSize() > 0;
   if (m_bShowResults)
      AutoSaveOutputs(bPartialResults);

   CString sMessage;
   if (bPartialResults)
      sMessage.Format(_T("Analysis stopped after %i of %i games completed."), iCompletedGames, avGamePGNs.GetSize());
   else if (m_iGamesWithErrors > 0)
      sMessage.Format(_T("Errors were encountered on %i games out of a total %i to be analysed."), m_iGamesWithErrors, avGamePGNs.GetSize());
   else
      sMessage.Format(_T("Analysis of %i games completed with no errors."), avGamePGNs.GetSize());
   if (m_bShowResults)
   {
      if (!m_sSavedResultsPath.IsEmpty())
         sMessage += _T("\r\n\r\nAnalysis XML saved to:\r\n") + m_sSavedResultsPath;
      else if (!m_sAutoSaveError.IsEmpty())
         sMessage += _T("\r\n\r\n") + m_sAutoSaveError + _T("\r\nYou can still save the results manually from the Results window.");

      if (!m_sSavedPGNPath.IsEmpty())
         sMessage += _T("\r\n\r\nAnnotated PGN saved to:\r\n") + m_sSavedPGNPath;
      else if (!m_sAutoSavePGNError.IsEmpty())
         sMessage += _T("\r\n\r\n") + m_sAutoSavePGNError;
   }
   MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);

   //caller will display results
   return true;
}

void CAnalysisDlg::RequestStop()
{
   if (m_eState == STATE_STOPPING || m_eState == STATE_COMPLETED)
      return;

   if (IDNO == MessageBox(_T("Stop the current analysis and keep only the games that have already finished?"), _T("PGN Spy"), MB_ICONQUESTION | MB_YESNO))
      return;

   m_bStopped = true;
   m_eState = STATE_STOPPING;
   m_sStatusHistory = _T("Stop requested. Active analysis processes will be cancelled.\r\n") + m_sStatusHistory;
   m_bStatusChanged = true;
   UpdateThreadControlButtons();
   UpdateDisplay();
}

void CAnalysisDlg::CancelActiveProcesses()
{
   for (int i = 0; i < m_ahProcesses.GetSize(); i++)
   {
      KillThreadProcessTree(i);
      CloseThreadResources(i);
   }
}

void CAnalysisDlg::CloseThreadResources(int iThread)
{
   if (m_ahProcesses[iThread] != NULL)
   {
      CloseHandle(m_ahProcesses[iThread]);
      m_ahProcesses[iThread] = NULL;
   }
   if (m_ahJobObjects[iThread] != NULL)
   {
      CloseHandle(m_ahJobObjects[iThread]);
      m_ahJobObjects[iThread] = NULL;
   }
   if (m_ahChildStdInRead[iThread] != NULL)
   {
      CloseHandle(m_ahChildStdInRead[iThread]);
      m_ahChildStdInRead[iThread] = NULL;
   }
   if (m_ahChildStdInWrite[iThread] != NULL)
   {
      CloseHandle(m_ahChildStdInWrite[iThread]);
      m_ahChildStdInWrite[iThread] = NULL;
   }
   if (m_ahChildStdOutRead[iThread] != NULL)
   {
      CloseHandle(m_ahChildStdOutRead[iThread]);
      m_ahChildStdOutRead[iThread] = NULL;
   }
   if (m_ahChildStdOutWrite[iThread] != NULL)
   {
      CloseHandle(m_ahChildStdOutWrite[iThread]);
      m_ahChildStdOutWrite[iThread] = NULL;
   }
   if (m_ahChildStdErrRead[iThread] != NULL)
   {
      CloseHandle(m_ahChildStdErrRead[iThread]);
      m_ahChildStdErrRead[iThread] = NULL;
   }
   if (m_ahChildStdErrWrite[iThread] != NULL)
   {
      CloseHandle(m_ahChildStdErrWrite[iThread]);
      m_ahChildStdErrWrite[iThread] = NULL;
   }
}

void CAnalysisDlg::KillThreadProcessTree(int iThread)
{
   static const char sCancelCommand[] = "cancel\n";
   if (m_ahChildStdInWrite[iThread] != NULL)
   {
      DWORD dwWritten = 0;
      WriteFile(m_ahChildStdInWrite[iThread], sCancelCommand, (DWORD)(sizeof(sCancelCommand) - 1), &dwWritten, NULL);
   }

   if (m_ahJobObjects[iThread] != NULL)
   {
      TerminateJobObject(m_ahJobObjects[iThread], 1);
   }
   else if (m_ahProcesses[iThread] != NULL)
   {
      TerminateProcess(m_ahProcesses[iThread], 1);
   }

   if (m_ahProcesses[iThread] != NULL)
      WaitForSingleObject(m_ahProcesses[iThread], 2000);
}

bool CAnalysisDlg::AutoSaveOutputs(bool bPartialResults)
{
   m_sSavedResultsPath.Empty();
   m_sSavedPGNPath.Empty();
   m_sAutoSaveError.Empty();
   m_sAutoSavePGNError.Empty();

   CString sResultsPath = GetDefaultAnalysisResultsFilePath(m_sInputFilePath);
   if (bPartialResults)
      sResultsPath.Replace(_T(".xml"), _T(" partial.xml"));

   if (SaveGameArrayToFile(sResultsPath, m_avGames, m_vEngineSettings))
      m_sSavedResultsPath = sResultsPath;
   else
      m_sAutoSaveError = _T("Failed to auto-save analysis XML to: ") + sResultsPath;

   CString sPGNPath = sResultsPath;
   if (!sPGNPath.Replace(_T(".xml"), _T(".pgn")))
      sPGNPath += _T(".pgn");
   CString sExportError;
   if (ExportGameArrayToAnnotatedPGN(sPGNPath, m_avGames, m_vEngineSettings, sExportError))
      m_sSavedPGNPath = sPGNPath;
   else
      m_sAutoSavePGNError = _T("Failed to auto-save annotated PGN: ") + sExportError;

   return !m_sSavedResultsPath.IsEmpty() || !m_sSavedPGNPath.IsEmpty();
}

void CAnalysisDlg::ReadFromThread(int iThread, CString IN OUT &rsResults, bool IN OUT &rbError)
{
   if (m_ahChildStdOutRead[iThread] == NULL)
   {
      ASSERT(false);
      return;
   }
   CFile vOutFile(m_ahChildStdOutRead[iThread]);
   //we won't need to close these handles; they're closed elsewhere

   char sBuf[1001];
   ZeroMemory(sBuf, sizeof(sBuf));

   //check for failure
   DWORD iExitCode;
   GetExitCodeProcess(m_ahProcesses[iThread], &iExitCode);
   if (iExitCode != STILL_ACTIVE && iExitCode < 0)
   {
      //found error
      rbError = true;
   }

   //process hasn't terminated; check for error anyway
   DWORD iBytesAvailable;
   PeekNamedPipe(m_ahChildStdErrRead[iThread], NULL, NULL, NULL, &iBytesAvailable, NULL);
   if (iBytesAvailable > 0)
   {
      //Read error for debugging purposes - also to ensure pipe doesn't fill up and lock up the analyser
      CFile vErrFile(m_ahChildStdErrRead[iThread]);
      int iBytesRead = vErrFile.Read(sBuf, 1000);
      return;
   }

   //no failure, keep reading until we reach end of results or end of what's currently in the pipe
   while (rsResults.Find(_T("</gamelist>")) == -1)
   {
      //ensure there's still data to read
      DWORD iBytesAvailable;
      PeekNamedPipe(m_ahChildStdOutRead[iThread], NULL, NULL, NULL, &iBytesAvailable, NULL);
      if (iBytesAvailable <= 0)
         break; //hit end of pipe

      int iBytesRead = vOutFile.Read(sBuf, 1000);
      rsResults.Append(CA2T(sBuf, CP_ACP));
//       if (iBytesRead < 1000) //not needed if we're doing PeekNamedPipe
//          break; //hit end of pipe
      ZeroMemory(sBuf, sizeof(sBuf)); //initialise for next time round
   }
}

bool CAnalysisDlg::LaunchAnalyser(CGamePGN vGamePGN, int iCurThread)
{
   SECURITY_ATTRIBUTES vAttrib;
   vAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
   vAttrib.bInheritHandle = TRUE;
   vAttrib.lpSecurityDescriptor = NULL;

   //create std in pipe for writing to child process
   if (!CreatePipe(&m_ahChildStdInRead[iCurThread], &m_ahChildStdInWrite[iCurThread], &vAttrib, 0))
   {
      CString sMessage = _T("Failed to create pipe to communicate with analyser.");
      MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
      return false;
   }
   if (!SetHandleInformation(m_ahChildStdInWrite[iCurThread], HANDLE_FLAG_INHERIT, 0))
   {
      CString sMessage = _T("Failed set up pipe to communicate with analyser.");
      MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
      return false;
   }

   //create std out pipe for reading from child process
   if (!CreatePipe(&m_ahChildStdOutRead[iCurThread], &m_ahChildStdOutWrite[iCurThread], &vAttrib, 0))
   {
      CString sMessage = _T("Failed to create pipe to communicate with analyser.");
      MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
      return false;
   }
   if (!SetHandleInformation(m_ahChildStdOutRead[iCurThread], HANDLE_FLAG_INHERIT, 0))
   {
      CString sMessage = _T("Failed set up pipe to communicate with analyser.");
      MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
      return false;
   }

   //create std err pipe for reading from child process
   if (!CreatePipe(&m_ahChildStdErrRead[iCurThread], &m_ahChildStdErrWrite[iCurThread], &vAttrib, 0))
   {
      CString sMessage = _T("Failed to create pipe to communicate with analyser.");
      MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
      return false;
   }
   if (!SetHandleInformation(m_ahChildStdErrRead[iCurThread], HANDLE_FLAG_INHERIT, 0))
   {
      CString sMessage = _T("Failed set up pipe to communicate with analyser.");
      MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
      return false;
   }

   PROCESS_INFORMATION vProcessInfo;
   STARTUPINFO vStartupInfo = { 0 };
   vStartupInfo.cb = sizeof(STARTUPINFO);
   vStartupInfo.hStdInput = m_ahChildStdInRead[iCurThread];
   vStartupInfo.hStdOutput = m_ahChildStdOutWrite[iCurThread];
   vStartupInfo.hStdError = m_ahChildStdErrWrite[iCurThread];
   vStartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
   vStartupInfo.wShowWindow = SW_HIDE;

   CString sCommandLine;
   CString sWhiteOrBlack = _T("");
   if (!m_vEngineSettings.m_sPlayerName.IsEmpty())
   {
      if (m_vEngineSettings.m_sPlayerName.CompareNoCase(vGamePGN.m_sWhite) == 0)
         sWhiteOrBlack = _T("--whiteonly ");
      else if (m_vEngineSettings.m_sPlayerName.CompareNoCase(vGamePGN.m_sBlack) == 0)
         sWhiteOrBlack = _T("--blackonly ");
      else
         ASSERT(false); //we should have discarded this game before this point
   }
   int iBookDepthPlies = m_vEngineSettings.m_iBookDepth * 2; //double book depth, since analyser uses plies, not moves
   sCommandLine.Format(_T("--bookdepth %i --searchdepth %i --searchmaxtime %i --searchmintime %i --variations %i %s --setoption Hash %i --setoption Threads %i --engine \"%s\" \"%s\""),
      iBookDepthPlies, m_vEngineSettings.m_iSearchDepth, m_vEngineSettings.m_iMaxTime,
      m_vEngineSettings.m_iMinTime, m_vEngineSettings.m_iNumVariations + 1, sWhiteOrBlack,
      m_vEngineSettings.m_iHashSize, m_vEngineSettings.m_iEngineThreads, m_vEngineSettings.m_sEnginePath, vGamePGN.m_sFileName);
   if (!CreateProcess(GetAnalyserFilePath(), sCommandLine.GetBuffer(), NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED, NULL, NULL, &vStartupInfo, &vProcessInfo))
   {
      sCommandLine.ReleaseBuffer();
      DWORD dwError = GetLastError();
      CString sMessage = _T("Failed to launch analyser.  Please ensure it is in the same folder as PGN Spy, with the file name \"pgn-extract.exe\".");
      MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
      return false;
   }
   sCommandLine.ReleaseBuffer();
   m_ahProcesses[iCurThread] = vProcessInfo.hProcess;

   HANDLE hJob = CreateJobObject(NULL, NULL);
   if (hJob != NULL)
   {
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION vJobInfo;
      ZeroMemory(&vJobInfo, sizeof(vJobInfo));
      vJobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
      if (SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &vJobInfo, sizeof(vJobInfo)) &&
         AssignProcessToJobObject(hJob, vProcessInfo.hProcess))
      {
         m_ahJobObjects[iCurThread] = hJob;
      }
      else
      {
         CloseHandle(hJob);
         m_ahJobObjects[iCurThread] = NULL;
      }
   }

   ResumeThread(vProcessInfo.hThread);
   CloseHandle(vProcessInfo.hThread);
   return true;
}

bool CAnalysisDlg::ProcessOutput(CString sOutput)
{
   if (false)
   {
      //debugging code to dump sample xml output to a file
      CString sFileName = GetTemporaryPGNFilePath(1);
      sFileName.Replace(_T(".pgn"), _T(".xml"));
      CFile vFile;
      if (!vFile.Open(sFileName, CFile::modeCreate | CFile::modeWrite))
      {
         CString sMessage = _T("Failed to create temporary output file.");
         MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
         return false;
      }

      if (!WriteCStringToFile(vFile, sOutput))
      {
         CString sMessage = _T("Failed to write temporary output file.");
         MessageBox(sMessage, _T("PGN Spy"), MB_ICONEXCLAMATION);
         vFile.Close();
         return false;
      }
      vFile.Close();
   }

   CGame vGame;
   if (vGame.LoadGame(sOutput))
      m_avGames.Add(vGame);
   else
   {
      m_iGamesWithErrors++;
      return false;
   }
   return true;
}


void CAnalysisDlg::OnBnClickedDecreasethreads()
{
   m_iTargetParallelGames--;
   if (m_iTargetParallelGames < 1)
   {
      ASSERT(false);
      m_iTargetParallelGames = 1;
   }
   CString sStatusLine;
   sStatusLine.Format(_T("Number of parallel games will be decreased to %i as active games are completed.\r\n"), m_iTargetParallelGames);
   m_sStatusHistory = sStatusLine + m_sStatusHistory;
   m_bStatusChanged = true;
   UpdateThreadControlButtons();
}


void CAnalysisDlg::OnBnClickedIncreasethreads()
{
   m_iTargetParallelGames++;
   if (m_iTargetParallelGames > m_iMaxParallelGames)
   {
      ASSERT(false);
      m_iTargetParallelGames = m_iMaxParallelGames;
   }
   CString sStatusLine;
   sStatusLine.Format(_T("Number of parallel games increased to %i.\r\n"), m_iTargetParallelGames);
   m_sStatusHistory = sStatusLine + m_sStatusHistory;
   m_bStatusChanged = true;
   UpdateThreadControlButtons();
}

void CAnalysisDlg::OnBnClickedPauseresume()
{
   if (m_eState == STATE_STOPPING || m_eState == STATE_COMPLETED)
      return;

   CString sStatusLine;
   if (m_eState == STATE_PAUSED || m_eState == STATE_PAUSING)
   {
      m_eState = STATE_RUNNING;
      sStatusLine = _T("Analysis resumed.\r\n");
   }
   else
   {
      m_eState = STATE_PAUSING;
      sStatusLine = _T("Pause requested. No new games will be started until analysis is resumed.\r\n");
   }

   m_sStatusHistory = sStatusLine + m_sStatusHistory;
   m_bStatusChanged = true;
   UpdateThreadControlButtons();
   UpdateDisplay();
}

void CAnalysisDlg::OnBnClickedStopanalysis()
{
   RequestStop();
}



