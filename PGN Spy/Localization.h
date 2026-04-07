#pragma once

enum EAppLanguage
{
   APPLANG_ENGLISH = 0,
   APPLANG_RUSSIAN = 1
};

void LoadAppLanguageFromRegistry();
bool SaveAppLanguageToRegistry(EAppLanguage eLanguage);
EAppLanguage GetCurrentAppLanguage();
bool IsRussianLanguage();

CString Loc(LPCTSTR sEnglish, LPCTSTR sRussian);
CString GetLanguageRegistryValue(EAppLanguage eLanguage);
CString GetLanguageDisplayName(EAppLanguage eLanguage);
CString GetAppTitle();
CString GetAboutMenuText();
CString GetLanguageChangedMessage();

void ApplyDialogTranslations(CWnd* pDialog, UINT nDialogId);
void PopulateLanguageCombo(CComboBox& vCombo, EAppLanguage eSelectedLanguage);
EAppLanguage GetSelectedLanguage(const CComboBox& vCombo);

bool WriteCStringToFile(CFile& vFile, const CString& sText);
