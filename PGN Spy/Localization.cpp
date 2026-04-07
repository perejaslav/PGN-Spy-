#include "stdafx.h"
#include "PGN Spy.h"
#include "Localization.h"

namespace
{
   EAppLanguage g_eCurrentLanguage = APPLANG_ENGLISH;

   struct CDialogTextTranslation
   {
      UINT m_nDialogId;
      LPCTSTR m_pszEnglish;
      LPCTSTR m_pszRussian;
   };

   const CDialogTextTranslation g_vDialogTranslations[] =
   {
      { IDD_ABOUTBOX, _T("About PGN Spy"), _T("О программе PGN Spy") },
      { IDD_ABOUTBOX, _T("Version 1.1"), _T("Версия 1.1") },
      { IDD_ABOUTBOX, _T("Copyright (C) 2017 Michael J. Gleason"), _T("Copyright (C) 2017 Michael J. Gleason") },
      { IDD_ABOUTBOX, _T("OK"), _T("ОК") },

      { IDD_PGNSPY_DIALOG, _T("PGN Spy"), _T("PGN Spy") },
      { IDD_PGNSPY_DIALOG, _T("Input PGN"), _T("Входной PGN") },
      { IDD_PGNSPY_DIALOG, _T("Browse"), _T("Обзор") },
      { IDD_PGNSPY_DIALOG, _T("Player to investigate (blank for all)"), _T("Игрок для проверки (пусто - все)") },
      { IDD_PGNSPY_DIALOG, _T("Run Analysis"), _T("Запустить анализ") },
      { IDD_PGNSPY_DIALOG, _T("Book depth"), _T("Глубина книги") },
      { IDD_PGNSPY_DIALOG, _T("Variations"), _T("Варианты") },
      { IDD_PGNSPY_DIALOG, _T("Threads"), _T("Потоки") },
      { IDD_PGNSPY_DIALOG, _T("Hash size (MB)"), _T("Размер hash (МБ)") },
      { IDD_PGNSPY_DIALOG, _T("Min time per move (ms)"), _T("Мин. время на ход (мс)") },
      { IDD_PGNSPY_DIALOG, _T("Max time per move (ms)"), _T("Макс. время на ход (мс)") },
      { IDD_PGNSPY_DIALOG, _T("Min search depth"), _T("Мин. глубина поиска") },
      { IDD_PGNSPY_DIALOG, _T("Engine"), _T("Движок") },
      { IDD_PGNSPY_DIALOG, _T("Save Settings"), _T("Сохранить настройки") },
      { IDD_PGNSPY_DIALOG, _T("Load Results"), _T("Загрузить результаты") },
      { IDD_PGNSPY_DIALOG, _T("Language"), _T("Язык") },

      { IDD_ANALYSIS, _T("PGN Spy"), _T("PGN Spy") },
      { IDD_ANALYSIS, _T("Preparing analysis..."), _T("Подготовка анализа...") },
      { IDD_ANALYSIS, _T("0 of 0 games completed. 0 active threads."), _T("Завершено 0 из 0 партий. Активных потоков: 0.") },
      { IDD_ANALYSIS, _T("Decrease Threads"), _T("Меньше потоков") },
      { IDD_ANALYSIS, _T("Pause"), _T("Пауза") },
      { IDD_ANALYSIS, _T("Stop"), _T("Стоп") },
      { IDD_ANALYSIS, _T("Increase Threads"), _T("Больше потоков") },

      { IDD_RESULTS, _T("Results"), _T("Результаты") },
      { IDD_RESULTS, _T("Analysis Settings"), _T("Настройки анализа") },
      { IDD_RESULTS, _T("Exclude forced moves"), _T("Исключать вынужденные ходы") },
      { IDD_RESULTS, _T("Forced move threshold"), _T("Порог вынужденного хода") },
      { IDD_RESULTS, _T("Include only unclear positions"), _T("Только неясные позиции") },
      { IDD_RESULTS, _T("Unclear position threshold"), _T("Порог неясной позиции") },
      { IDD_RESULTS, _T("Losing position threshold"), _T("Порог проигранной позиции") },
      { IDD_RESULTS, _T("Undecided position threshold"), _T("Порог равной позиции") },
      { IDD_RESULTS, _T("Include Losing Positions"), _T("Включать проигранные позиции") },
      { IDD_RESULTS, _T("Include Winning Positions"), _T("Включать выигранные позиции") },
      { IDD_RESULTS, _T("Include Post-losing Positions"), _T("Включать позиции после ухудшения") },
      { IDD_RESULTS, _T("Save Settings"), _T("Сохранить настройки") },
      { IDD_RESULTS, _T("Temporary Filters"), _T("Временные фильтры") },
      { IDD_RESULTS, _T("Player to investigate (blank for all)"), _T("Игрок для проверки (пусто - все)") },
      { IDD_RESULTS, _T("Opponent (blank for all)"), _T("Соперник (пусто - все)") },
      { IDD_RESULTS, _T("Event (blank for all)"), _T("Турнир (пусто - все)") },
      { IDD_RESULTS, _T("White Moves"), _T("Ходы белых") },
      { IDD_RESULTS, _T("All Moves"), _T("Все ходы") },
      { IDD_RESULTS, _T("Black Moves"), _T("Ходы чёрных") },
      { IDD_RESULTS, _T("Include Wins"), _T("Включать победы") },
      { IDD_RESULTS, _T("Include Losses"), _T("Включать поражения") },
      { IDD_RESULTS, _T("Include Draws"), _T("Включать ничьи") },
      { IDD_RESULTS, _T("Move range min"), _T("Мин. номер хода") },
      { IDD_RESULTS, _T("Move range max"), _T("Макс. номер хода") },
      { IDD_RESULTS, _T("About These Results"), _T("О показателях") },
      { IDD_RESULTS, _T("Recalculate"), _T("Пересчитать") },
      { IDD_RESULTS, _T("Export per-game statistics"), _T("Экспорт статистики по партиям") },
      { IDD_RESULTS, _T("Load and Merge Results"), _T("Загрузить и объединить") },
      { IDD_RESULTS, _T("Export per-move statistics"), _T("Экспорт статистики по ходам") },
      { IDD_RESULTS, _T("Save analysis results"), _T("Сохранить результаты анализа") },
      { IDD_RESULTS, _T("Export annotated PGN"), _T("Экспорт аннотированного PGN") },
      { IDD_RESULTS, _T("Saved XML / load source"), _T("XML / источник загрузки") },
      { IDD_RESULTS, _T("Annotated PGN"), _T("Аннотированный PGN") },
   };

   CString GetRussianOrEnglish(LPCTSTR sEnglish, LPCTSTR sRussian)
   {
      return (g_eCurrentLanguage == APPLANG_RUSSIAN) ? CString(sRussian) : CString(sEnglish);
   }

   BOOL CALLBACK ApplyTranslationToChild(HWND hWnd, LPARAM lParam)
   {
      UINT nDialogId = (UINT)lParam;
      CString sText;
      ::GetWindowText(hWnd, sText.GetBufferSetLength(512), 512);
      sText.ReleaseBuffer();
      sText.TrimRight();
      if (sText.IsEmpty())
         return TRUE;

      for (int i = 0; i < _countof(g_vDialogTranslations); i++)
      {
         if (g_vDialogTranslations[i].m_nDialogId == nDialogId &&
            sText == g_vDialogTranslations[i].m_pszEnglish)
         {
            ::SetWindowText(hWnd, GetRussianOrEnglish(g_vDialogTranslations[i].m_pszEnglish, g_vDialogTranslations[i].m_pszRussian));
            break;
         }
      }
      return TRUE;
   }
}

void LoadAppLanguageFromRegistry()
{
   CString sLanguage = theApp.GetProfileString(_T("PGNSpy"), _T("Language"), _T("en"));
   if (sLanguage.CompareNoCase(_T("ru")) == 0)
      g_eCurrentLanguage = APPLANG_RUSSIAN;
   else
      g_eCurrentLanguage = APPLANG_ENGLISH;
}

bool SaveAppLanguageToRegistry(EAppLanguage eLanguage)
{
   g_eCurrentLanguage = eLanguage;
   return theApp.WriteProfileString(_T("PGNSpy"), _T("Language"), GetLanguageRegistryValue(eLanguage)) != FALSE;
}

EAppLanguage GetCurrentAppLanguage()
{
   return g_eCurrentLanguage;
}

bool IsRussianLanguage()
{
   return g_eCurrentLanguage == APPLANG_RUSSIAN;
}

CString Loc(LPCTSTR sEnglish, LPCTSTR sRussian)
{
   return GetRussianOrEnglish(sEnglish, sRussian);
}

CString GetLanguageRegistryValue(EAppLanguage eLanguage)
{
   return (eLanguage == APPLANG_RUSSIAN) ? _T("ru") : _T("en");
}

CString GetLanguageDisplayName(EAppLanguage eLanguage)
{
   return (eLanguage == APPLANG_RUSSIAN) ? _T("Русский") : _T("English");
}

CString GetAppTitle()
{
   return _T("PGN Spy");
}

CString GetAboutMenuText()
{
   return Loc(_T("&About PGN Spy..."), _T("&О программе PGN Spy..."));
}

CString GetLanguageChangedMessage()
{
   return Loc(
      _T("Language selection has been saved. Restart PGN Spy to apply the new language."),
      _T("Выбор языка сохранён. Перезапустите PGN Spy, чтобы применить новый язык."));
}

void ApplyDialogTranslations(CWnd* pDialog, UINT nDialogId)
{
   if (pDialog == NULL || !::IsWindow(pDialog->GetSafeHwnd()))
      return;

   if (nDialogId == IDD_ABOUTBOX)
      pDialog->SetWindowText(Loc(_T("About PGN Spy"), _T("О программе PGN Spy")));
   else if (nDialogId == IDD_RESULTS)
      pDialog->SetWindowText(Loc(_T("Results"), _T("Результаты")));
   else
      pDialog->SetWindowText(GetAppTitle());

   EnumChildWindows(pDialog->GetSafeHwnd(), ApplyTranslationToChild, (LPARAM)nDialogId);
}

void PopulateLanguageCombo(CComboBox& vCombo, EAppLanguage eSelectedLanguage)
{
   vCombo.ResetContent();
   int iEnglish = vCombo.AddString(GetLanguageDisplayName(APPLANG_ENGLISH));
   int iRussian = vCombo.AddString(GetLanguageDisplayName(APPLANG_RUSSIAN));
   vCombo.SetItemData(iEnglish, APPLANG_ENGLISH);
   vCombo.SetItemData(iRussian, APPLANG_RUSSIAN);
   vCombo.SetCurSel(eSelectedLanguage == APPLANG_RUSSIAN ? iRussian : iEnglish);
}

EAppLanguage GetSelectedLanguage(const CComboBox& vCombo)
{
   int iSelection = vCombo.GetCurSel();
   if (iSelection == CB_ERR)
      return APPLANG_ENGLISH;
   DWORD_PTR nLanguage = vCombo.GetItemData(iSelection);
   return (nLanguage == APPLANG_RUSSIAN) ? APPLANG_RUSSIAN : APPLANG_ENGLISH;
}

bool WriteCStringToFile(CFile& vFile, const CString& sText)
{
   CT2A sAnsiText(sText, CP_ACP);
   LPCSTR pszText = sAnsiText;
   if (pszText == NULL)
      return false;
   vFile.Write(pszText, (UINT)strlen(pszText));
   return true;
}
