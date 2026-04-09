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
#include "Analysis.h"
#include "AnalysisDlg.h"
#include "ResultsDlg.h"
#include "Localization.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
   CAboutDlg();

// Dialog Data
   enum { IDD = IDD_ABOUTBOX };

   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog();

// Implementation
protected:
   DECLARE_MESSAGE_MAP()
public:
   CString m_sCredits;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
, m_sCredits(_T(""))
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_CREDITS, m_sCredits);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
   CDialog::OnInitDialog();
   ApplyDialogTranslations(this, IDD_ABOUTBOX);

   m_sCredits = Loc(
      _T("This project would have been much more difficult without the contributions of several different people.\r\n")
      _T("I would especially like to thank David Barnes for the use of uci-analyser and pgn-extract,\r\n")
      _T("Ben Bryant of firstobject.com for the use of CMarkup,\r\n")
      _T("and LegoPirateSenior and others who gave advice and/or contributed to testing.\r\n"),
      _T("Этот проект было бы значительно сложнее реализовать без помощи многих людей.\r\n")
      _T("Особая благодарность David Barnes за uci-analyser и pgn-extract,\r\n")
      _T("Ben Bryant с firstobject.com за библиотеку CMarkup,\r\n")
      _T("а также LegoPirateSenior и другим участникам, помогавшим советами и тестированием.\r\n"));
   UpdateData(FALSE);

   return TRUE;  // return TRUE  unless you set the focus to a control
}



// CPGNSpyDlg dialog




CPGNSpyDlg::CPGNSpyDlg(CWnd* pParent /*=NULL*/)
   : CDialog(CPGNSpyDlg::IDD, pParent)
   , m_sInputFile(_T(""))
{
   m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPGNSpyDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_INPUTANALYSE, m_sInputFile);
   DDX_Text(pDX, IDC_PLAYER, m_vEngineSettings.m_sPlayerName);
   DDX_Text(pDX, IDC_ENGINE, m_vEngineSettings.m_sEnginePath);
   DDX_Text(pDX, IDC_SEARCHDEPTH, m_vEngineSettings.m_iSearchDepth);
   DDV_MinMaxInt(pDX, m_vEngineSettings.m_iSearchDepth, 1, 50);
   DDX_Text(pDX, IDC_BOOKDEPTH, m_vEngineSettings.m_iBookDepth);
   DDV_MinMaxInt(pDX, m_vEngineSettings.m_iBookDepth, 0, 30);
   DDX_Text(pDX, IDC_NUMTHREADS, m_vEngineSettings.m_iEngineThreads);
   DDV_MinMaxInt(pDX, m_vEngineSettings.m_iEngineThreads, 1, 128);
   DDX_Text(pDX, IDC_PARALLELGAMES, m_vEngineSettings.m_iParallelGames);
   DDV_MinMaxInt(pDX, m_vEngineSettings.m_iParallelGames, 1, 128);
   DDX_Text(pDX, IDC_MINTIME, m_vEngineSettings.m_iMinTime);
   DDV_MinMaxInt(pDX, m_vEngineSettings.m_iMinTime, 1, 60000);
   DDX_Text(pDX, IDC_MAXTIME, m_vEngineSettings.m_iMaxTime);
   DDV_MinMaxInt(pDX, m_vEngineSettings.m_iMaxTime, 1, 60000);
   DDX_Text(pDX, IDC_HASHSIZE, m_vEngineSettings.m_iHashSize);
   DDV_MinMaxInt(pDX, m_vEngineSettings.m_iHashSize, 1, 8192);
   DDX_Text(pDX, IDC_VARIATIONS, m_vEngineSettings.m_iNumVariations);
   DDV_MinMaxInt(pDX, m_vEngineSettings.m_iNumVariations, 1, 10);
   DDX_Control(pDX, IDC_LANGUAGE, m_vLanguage);
}

BEGIN_MESSAGE_MAP(CPGNSpyDlg, CDialog)
   ON_WM_SYSCOMMAND()
   ON_WM_PAINT()
   ON_WM_QUERYDRAGICON()
   //}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_BROWSEANALYSE, &CPGNSpyDlg::OnBnClickedBrowseanalyse)
   ON_BN_CLICKED(IDC_BROWSEENGINE, &CPGNSpyDlg::OnBnClickedBrowseengine)
   ON_BN_CLICKED(IDC_RUNANALYSIS, &CPGNSpyDlg::OnBnClickedRunanalysis)
   ON_BN_CLICKED(IDC_HELPPLAYER, &CPGNSpyDlg::OnBnClickedHelpplayer)
   ON_BN_CLICKED(IDC_HELPDEPTH, &CPGNSpyDlg::OnBnClickedHelpdepth)
   ON_BN_CLICKED(IDC_HELPBOOKDEPTH, &CPGNSpyDlg::OnBnClickedHelpbookdepth)
   ON_BN_CLICKED(IDC_HELPTHREADS, &CPGNSpyDlg::OnBnClickedHelpthreads)
   ON_BN_CLICKED(IDC_HELPPARALLELGAMES, &CPGNSpyDlg::OnBnClickedHelpparallelgames)
   ON_BN_CLICKED(IDC_HELPMINTIME, &CPGNSpyDlg::OnBnClickedHelpmintime)
   ON_BN_CLICKED(IDC_HELPMAXTIME, &CPGNSpyDlg::OnBnClickedHelpmaxtime)
   ON_BN_CLICKED(IDC_HELPPHASHSIZE, &CPGNSpyDlg::OnBnClickedHelpphashsize)
   ON_BN_CLICKED(IDC_SAVESETTINGS, &CPGNSpyDlg::OnBnClickedSavesettings)
   ON_BN_CLICKED(IDC_FORCEDMOVEHELP, &CPGNSpyDlg::OnBnClickedForcedmovehelp)
   ON_BN_CLICKED(IDC_UNCLEARPOSITIONHELP, &CPGNSpyDlg::OnBnClickedUnclearpositionhelp)
   ON_BN_CLICKED(IDC_EQUALPOSITIONHELP, &CPGNSpyDlg::OnBnClickedEqualpositionhelp)
   ON_BN_CLICKED(IDC_LOSINGTHRESHOLDHELP, &CPGNSpyDlg::OnBnClickedLosingthresholdhelp)
   ON_BN_CLICKED(IDC_NUMVARIATIONSHELP, &CPGNSpyDlg::OnBnClickedNumvariationshelp)
   ON_BN_CLICKED(IDC_LOADRESULTS, &CPGNSpyDlg::OnBnClickedLoadresults)
   ON_CBN_SELCHANGE(IDC_LANGUAGE, &CPGNSpyDlg::OnCbnSelchangeLanguage)
END_MESSAGE_MAP()


// CPGNSpyDlg message handlers

BOOL CPGNSpyDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   // Add _T("About...") menu item to system menu.

   // IDM_ABOUTBOX must be in the system command range.
   ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
   ASSERT(IDM_ABOUTBOX < 0xF000);

   CMenu* pSysMenu = GetSystemMenu(FALSE);
   if (pSysMenu != NULL)
   {
      CString strAboutMenu = GetAboutMenuText();
      if (!strAboutMenu.IsEmpty())
      {
         pSysMenu->AppendMenu(MF_SEPARATOR);
         pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
      }
   }

   // Set the icon for this dialog.  The framework does this automatically
   //  when the application's main window is not a dialog
   SetIcon(m_hIcon, TRUE);			// Set big icon
   SetIcon(m_hIcon, FALSE);		// Set small icon

   if (!m_vEngineSettings.LoadSettingsFromRegistry())
      m_vEngineSettings = CEngineSettings(); //failed to load, so restore defaults
   UpdateData(FALSE);
   PopulateLanguageCombo(m_vLanguage, GetCurrentAppLanguage());
   ApplyDialogTranslations(this, IDD_PGNSPY_DIALOG);

   return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPGNSpyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
   if ((nID & 0xFFF0) == IDM_ABOUTBOX)
   {
      CAboutDlg dlgAbout;
      dlgAbout.DoModal();
   }
   else
   {
      CDialog::OnSysCommand(nID, lParam);
   }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPGNSpyDlg::OnPaint()
{
   if (IsIconic())
   {
      CPaintDC dc(this); // device context for painting

      SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

      // Center icon in client rectangle
      int cxIcon = GetSystemMetrics(SM_CXICON);
      int cyIcon = GetSystemMetrics(SM_CYICON);
      CRect rect;
      GetClientRect(&rect);
      int x = (rect.Width() - cxIcon + 1) / 2;
      int y = (rect.Height() - cyIcon + 1) / 2;

      // Draw the icon
      dc.DrawIcon(x, y, m_hIcon);
   }
   else
   {
      CDialog::OnPaint();
   }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPGNSpyDlg::OnQueryDragIcon()
{
   return static_cast<HCURSOR>(m_hIcon);
}

void CPGNSpyDlg::OnBnClickedBrowseanalyse()
{
   if (!UpdateData())
      return;
   CFileDialog vFileDialog(TRUE, _T("pgn"), _T("*.pgn"), OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT,
      Loc(_T("Portable Game Notation file (*.pgn)|*.pgn|All files (*.*)|*.*||"),
         _T("Файлы Portable Game Notation (*.pgn)|*.pgn|Все файлы (*.*)|*.*||")),
      this);
   if (vFileDialog.DoModal() != IDOK)
      return;
   m_sInputFile = vFileDialog.GetPathName();
   UpdateData(FALSE);
}

void CPGNSpyDlg::OnBnClickedBrowseengine()
{
   if (!UpdateData())
      return;
   CFileDialog vFileDialog(TRUE, _T("exe"), _T("*.exe"), OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT,
      Loc(_T("Chess Engines (*.exe)|*.exe|All files (*.*)|*.*||"),
         _T("Шахматные движки (*.exe)|*.exe|Все файлы (*.*)|*.*||")),
      this);
   if (vFileDialog.DoModal() != IDOK)
      return;
   m_vEngineSettings.m_sEnginePath = vFileDialog.GetPathName();
   UpdateData(FALSE);
}

bool CPGNSpyDlg::ConvertFileForAnalysis(CString OUT &sConvertedFile)
{
   sConvertedFile = GetConvertedPGNFilePath();
   CFile vFile;
   if (!vFile.Open(sConvertedFile, CFile::modeCreate | CFile::modeWrite))
   {
      MessageBox(Loc(_T("Failed to create temporary output file."), _T("Не удалось создать временный выходной файл.")), GetAppTitle(), MB_ICONEXCLAMATION);
      return false;
   }
   vFile.Close();

   CString sCommandLine = _T(" -Wuci \"-o") + sConvertedFile + _T("\" \"") + m_sInputFile + _T("\"");

   PROCESS_INFORMATION vProcessInfo;
   STARTUPINFO vStartupInfo = {0};
   vStartupInfo.cb = sizeof(vStartupInfo);
   vStartupInfo.dwFlags = STARTF_USESHOWWINDOW;
   vStartupInfo.wShowWindow = SW_HIDE;
   if (!CreateProcess(GetConverterFilePath(), sCommandLine.GetBuffer(), NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &vStartupInfo, &vProcessInfo))
   {
      sCommandLine.ReleaseBuffer();
      MessageBox(
         Loc(
            _T("Failed to launch converter. Please ensure it is in the same folder as PGN Spy, with the file name \"pgn-extract.exe\"."),
            _T("Не удалось запустить конвертер. Убедитесь, что он находится в той же папке, что и PGN Spy, и называется \"pgn-extract.exe\".")),
         GetAppTitle(),
         MB_ICONEXCLAMATION);
      return false;
   }
   sCommandLine.ReleaseBuffer();
   WaitForSingleObject(vProcessInfo.hProcess, INFINITE);
   CloseHandle(vProcessInfo.hProcess);
   CloseHandle(vProcessInfo.hThread);

   return true;
}

void CPGNSpyDlg::OnBnClickedRunanalysis()
{
   if (!ValidateSettings())
      return;

   //validate file paths
   if (!PathFileExists(m_sInputFile))
   {
      MessageBox(Loc(_T("The specified input file does not exist."), _T("Указанный входной файл не существует.")), GetAppTitle(), MB_ICONEXCLAMATION);
      return;
   }

   CString sTemporaryFile;
   if (!ConvertFileForAnalysis(sTemporaryFile))
   {
      MessageBox(Loc(_T("Failed to convert the PGN file into the appropriate format for analysis."), _T("Не удалось преобразовать PGN-файл в формат, подходящий для анализа.")), GetAppTitle(), MB_ICONEXCLAMATION);
      return;
   }

   //file is converted; now process it
   CAnalysisDlg vAnalyserDlg;
   vAnalyserDlg.m_sConvertedPGN = sTemporaryFile;
   vAnalyserDlg.m_sInputFilePath = m_sInputFile;
   vAnalyserDlg.m_vEngineSettings = m_vEngineSettings;
   vAnalyserDlg.DoModal();

   //delete temporary file
   DeleteFile(sTemporaryFile);

   if (!vAnalyserDlg.m_bShowResults)
      return;
   
   //now launch the window to process and display the results
   CResultsDlg vResultsDlg;
   vResultsDlg.m_avGames.Copy(vAnalyserDlg.m_avGames);
   vResultsDlg.m_vEngineSettings = m_vEngineSettings;
   if (!vAnalyserDlg.m_sSavedResultsPath.IsEmpty())
      vResultsDlg.m_sSavedResultsPath = Loc(_T("Saved analysis XML: "), _T("Сохранённый XML анализа: ")) + vAnalyserDlg.m_sSavedResultsPath;
   else if (!vAnalyserDlg.m_sAutoSaveError.IsEmpty())
      vResultsDlg.m_sSavedResultsPath = vAnalyserDlg.m_sAutoSaveError;
   if (!vAnalyserDlg.m_sSavedPGNPath.IsEmpty())
      vResultsDlg.m_sSavedPGNPath = Loc(_T("Saved annotated PGN: "), _T("Сохранённый аннотированный PGN: ")) + vAnalyserDlg.m_sSavedPGNPath;
   else if (!vAnalyserDlg.m_sAutoSavePGNError.IsEmpty())
      vResultsDlg.m_sSavedPGNPath = vAnalyserDlg.m_sAutoSavePGNError;
   vResultsDlg.DoModal();
}

void CPGNSpyDlg::OnBnClickedSavesettings()
{
   if (!ValidateSettings())
      return;

   if (!m_vEngineSettings.SaveSettingsToRegistry())
      MessageBox(Loc(_T("Failed to save settings."), _T("Не удалось сохранить настройки.")), GetAppTitle(), MB_ICONEXCLAMATION);
   else
      MessageBox(Loc(_T("Settings saved."), _T("Настройки сохранены.")), GetAppTitle(), MB_ICONINFORMATION);
}

bool CPGNSpyDlg::ValidateSettings()
{
   if (!UpdateData())
      return false;

   if (m_vEngineSettings.m_iMinTime > m_vEngineSettings.m_iMaxTime)
   {
      MessageBox(Loc(_T("The minimum time for analysis must not exceed the maximum time."), _T("Минимальное время анализа не должно превышать максимальное.")), GetAppTitle(), MB_ICONEXCLAMATION);
      return false;
   }

   if (!PathFileExists(m_vEngineSettings.m_sEnginePath))
   {
      MessageBox(Loc(_T("The specified engine does not exist."), _T("Указанный движок не существует.")), GetAppTitle(), MB_ICONEXCLAMATION);
      return false;
   }

   SYSTEM_INFO vSysInfo;
   GetSystemInfo(&vSysInfo);
   if (m_vEngineSettings.m_iEngineThreads > (int)vSysInfo.dwNumberOfProcessors)
   {
      MessageBox(Loc(_T("You have entered more engine threads than the number of processors present in your system."), _T("Указано больше потоков движка, чем доступно процессоров в системе.")), GetAppTitle(), MB_ICONEXCLAMATION);
      return false;
   }

   if (m_vEngineSettings.m_iParallelGames > (int)vSysInfo.dwNumberOfProcessors)
   {
      MessageBox(Loc(_T("You have entered more parallel games than the number of processors present in your system."), _T("Указано больше параллельных партий, чем доступно процессоров в системе.")), GetAppTitle(), MB_ICONEXCLAMATION);
      return false;
   }

   return true;
}

void CPGNSpyDlg::OnBnClickedHelpplayer()
{
   CString sMessage = Loc(
      _T("If a player name is entered, statistics for the specified player will be reported. Games excluding this player will be ignored.\n\nIf no player name is entered, aggregate statistics for all players will be reported. This is useful for establishing baselines."),
      _T("Если указано имя игрока, статистика будет рассчитана только для него. Партии без этого игрока будут исключены.\n\nЕсли имя не указано, будет показана суммарная статистика по всем игрокам. Это удобно для построения базового уровня."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedHelpdepth()
{
   CString sMessage = Loc(
      _T("Specify the minimum number of plies to search for each position. A ply is half a move, that is one white move or one black move.\n\nIf this depth is not reached within the minimum time, the search will continue until either this depth is reached or the maximum time has been reached."),
      _T("Укажите минимальную глубину поиска в полуходах для каждой позиции. Полуход - это ход одной стороны.\n\nЕсли эта глубина не достигнута за минимальное время, поиск продолжится, пока глубина не будет достигнута либо не истечёт максимальное время."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedHelpbookdepth()
{
   CString sMessage = Loc(
      _T("Specify the number of opening moves to exclude. A move consists of one white move and one black move, so entering five means ignoring five moves for each side.\n\nOpening moves are better excluded from calculations, as a player who has studied an opening may reproduce main-line moves from memory without using an engine. Static resources such as books and databases are also usually allowed in correspondence chess, so even a weaker player may legitimately play main-line moves early in the game without engine assistance."),
      _T("Укажите число дебютных ходов, которые нужно исключить. Один ход включает ход белых и ход чёрных, поэтому значение 5 означает исключение пяти ходов каждой стороны.\n\nДебют обычно лучше исключать из расчётов: подготовленный игрок может воспроизводить основные варианты по памяти без помощи движка. Кроме того, в заочных шахматах книги и базы обычно разрешены, поэтому даже более слабый игрок может корректно играть теоретические ходы в начале партии без подсказок движка."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedHelpthreads()
{
   CString sMessage = Loc(
      _T("Specify the number of CPU threads to give the engine.\n\nPGN Spy passes this value to the UCI engine as its Threads setting. A higher value allows the engine to use more cores while analysing.\n\nIf you change this setting while analysis is running, the new value is applied to analyser processes started after the change. Positions already being analysed keep their current engine thread count.\n\nDo not set this above the number of physical cores unless you have a specific reason to do so."),
      _T("Укажите число потоков CPU, которое можно использовать движку.\n\nPGN Spy передаёт это значение в UCI-движок как параметр Threads. Чем выше значение, тем больше ядер движок сможет задействовать при анализе.\n\nЕсли изменить это значение во время анализа, новый параметр будет применён к процессам анализатора, которые запускаются после изменения. Уже запущенные позиции продолжают анализироваться с прежним числом потоков.\n\nБез особой причины не рекомендуется задавать значение выше числа физических ядер."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedHelpparallelgames()
{
   CString sMessage = Loc(
      _T("Specify how many games may be analysed at the same time.\n\nEach parallel game starts a separate analyser process. Increasing this value can improve throughput on multi-core systems, but it also increases CPU and memory usage.\n\nThe Increase/Decrease controls in the analysis window change this value for new analyser processes started after the change."),
      _T("Укажите, сколько партий можно анализировать одновременно.\n\nДля каждой параллельной партии запускается отдельный процесс анализатора. Чем выше это значение, тем быстрее может идти общий анализ на многоядерной системе, но тем выше нагрузка на CPU и память.\n\nКнопки увеличения и уменьшения в окне анализа меняют именно это значение для новых процессов анализатора, которые запускаются после изменения."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedHelpmintime()
{
   CString sMessage = Loc(_T("Specify the minimum time in milliseconds to spend analysing each position."), _T("Укажите минимальное время в миллисекундах, отводимое на анализ каждой позиции."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedHelpmaxtime()
{
   CString sMessage = Loc(
      _T("Specify the maximum time in milliseconds to spend analysing each position. Analysis will stop when this limit is reached even if the minimum search depth has not yet been achieved."),
      _T("Укажите максимальное время в миллисекундах, отводимое на анализ каждой позиции. По достижении этого лимита анализ будет остановлен, даже если минимальная глубина поиска ещё не достигнута."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedHelpphashsize()
{
   CString sMessage = Loc(
      _T("Specify the size of the engine hash in megabytes.\n\nSee the engine documentation for suitable values.\n\nIf several threads are used, remember that each thread has its own hash. The total hash allocation should not exceed the physical RAM available in the machine, and enough memory should remain available for Windows and other running applications."),
      _T("Укажите размер hash-памяти движка в мегабайтах.\n\nРекомендуемые значения смотрите в документации к движку.\n\nПри использовании нескольких потоков у каждого потока будет собственный hash. Суммарный объём не должен превышать доступную физическую память, и часть памяти нужно оставить системе Windows и другим запущенным приложениям."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedForcedmovehelp()
{
   CString sMessage = Loc(
      _T("For T1/T2/T3 and similar statistics, moves where the next-best move is evaluated as worse than the played move by more than the specified threshold are excluded. This avoids flagging obvious recaptures and other moves that a strong player would normally find. Values are given in centipawns."),
      _T("Для показателей T1/T2/T3 и т. п. из анализа исключаются ходы, у которых следующий по силе ход хуже сыгранного более чем на указанный порог. Это позволяет не считать очевидные взятия и другие естественные ходы, которые сильный игрок обычно находит без труда. Значения задаются в сотых долях пешки."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedUnclearpositionhelp()
{
   CString sMessage = Loc(
      _T("For T1/T2/T3 and similar statistics, positions where the next-best move is worse than the engine's first choice by more than the specified threshold are excluded. Values are given in centipawns."),
      _T("Для показателей T1/T2/T3 и т. п. из анализа исключаются позиции, в которых следующий по силе ход хуже первого выбора движка более чем на указанный порог. Значения задаются в сотых долях пешки."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedEqualpositionhelp()
{
   CString sMessage = Loc(
      _T("Positions where neither side is better by more than the specified threshold are analysed. This helps detect players who stop using assistance once they obtain an advantage.\n\nThese results are reported separately from losing positions."),
      _T("Анализируются позиции, в которых перевес ни одной из сторон не превышает указанный порог. Это помогает выявлять игроков, которые перестают пользоваться подсказками, получив преимущество.\n\nЭти результаты выводятся отдельно от проигранных позиций."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedLosingthresholdhelp()
{
   CString sMessage = Loc(
      _T("Positions where the side to move is worse than the equal-position threshold but not yet beyond the losing-position threshold are analysed. This helps detect players who start using assistance only after the game turns against them.\n\nThese results are reported separately from equal positions."),
      _T("Анализируются позиции, в которых сторона хуже порога равной позиции, но ещё не вышла за порог проигранной позиции. Это помогает выявлять игроков, которые начинают пользоваться подсказками только после ухудшения позиции.\n\nЭти результаты выводятся отдельно от равных позиций."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedNumvariationshelp()
{
   CString sMessage = Loc(
      _T("Specify how many top engine moves should be compared with the move actually played in each position. This helps detect players who regularly choose the engine's second line, or who may use a different engine or settings."),
      _T("Укажите, сколько лучших ходов движка нужно сравнивать с реально сыгранным ходом в каждой позиции. Это помогает выявлять игроков, которые регулярно выбирают второй или третий выбор движка, либо используют другой движок или иные настройки."));
   MessageBox(sMessage, GetAppTitle(), MB_ICONINFORMATION);
}

void CPGNSpyDlg::OnBnClickedLoadresults()
{
   CFileDialog vFileDialog(TRUE, _T("xml"), _T("*.xml"), OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT,
      Loc(_T("PGN Spy files (*.xml)|*.xml|All files (*.*)|*.*||"),
         _T("Файлы PGN Spy (*.xml)|*.xml|Все файлы (*.*)|*.*||")),
      this);
   if (vFileDialog.DoModal() != IDOK)
      return;
   CArray <CGame, CGame> avGames;
   CEngineSettings vEngineSettings;
   if (!LoadGameArrayFromFile(vFileDialog.GetPathName(), avGames, vEngineSettings))
   {
      MessageBox(Loc(_T("Failed to load game file."), _T("Не удалось загрузить файл результатов.")), GetAppTitle(), MB_ICONEXCLAMATION);
      return;
   }

   CResultsDlg vResultsDlg;
   vResultsDlg.m_avGames.Copy(avGames);
   vResultsDlg.m_vEngineSettings = vEngineSettings;
   vResultsDlg.m_sSavedResultsPath = Loc(_T("Loaded analysis XML: "), _T("Загруженный XML анализа: ")) + vFileDialog.GetPathName();
   vResultsDlg.DoModal();
}

void CPGNSpyDlg::OnCbnSelchangeLanguage()
{
   EAppLanguage eSelectedLanguage = GetSelectedLanguage(m_vLanguage);
   if (eSelectedLanguage == GetCurrentAppLanguage())
      return;

   SaveAppLanguageToRegistry(eSelectedLanguage);
   MessageBox(GetLanguageChangedMessage(), GetAppTitle(), MB_ICONINFORMATION);
}



