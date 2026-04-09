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
#include "Analysis.h"
#include "PGN Spy.h"
#include "Markup.h"

static void TokenizeWhitespace(const CString &sText, CStringArray &rasTokens);
static bool IsResultToken(const CString &sToken);
static CString FormatRawScoreForPGN(const CString &sRawScore, int iNumericScore);
static CString GetPlayedMoveComment(const CPosition &vPosition);
static bool BuildAnnotatedUCIGameText(const CGame &vGame, const CEngineSettings &vEngineSettings, CString &rsOutput, CString &rsError);
static bool RunConverterToSAN(const CString &sInputPath, const CString &sOutputPath, CString &rsError);

void AddStringIfNotFound(CString sValue, CStringArray &rasArray, CArray<int, int> &raiCountArray)
{
   bool bFound = false;
   ASSERT(raiCountArray.GetSize() == rasArray.GetSize());
   for (int i = 0; i < rasArray.GetSize(); i++)
   {
      if (sValue.CompareNoCase(rasArray[i]) == 0)
      {
         raiCountArray[i]++; //increment counter
         return;
      }
   }

   //if we reach here, we didn't find it; add it
   rasArray.Add(sValue);
   raiCountArray.Add(1);
}

CAnalysisSettings::CAnalysisSettings()
{
   m_bExcludeForcedMoves = true;
   m_iForcedMoveCutoff = 50;
   m_bIncludeOnlyUnclearPositions = true;
   m_iUnclearPositionCutoff = 100;
   m_iEqualPositionThreshold = 200;
   m_iLosingThreshold = 500;
   m_bIncludeLosing = true;
   m_bIncludeWinning = false;
   m_bIncludePostLosing = false;

   //temporary filters
   m_iMoveNumMin = 11;
   m_iMoveNumMax = 10000;
   m_bWhiteOnly = false;
   m_bBlackOnly = false;
   m_bIncludeWins = true;
   m_bIncludeLosses = true;
   m_bIncludeDraws = true;
}

bool CAnalysisSettings::LoadSettingsFromRegistry()
{
   //calc number of cores and divide by two; engines aren't great with hyper-threading; checking for
   //hyper-threading seems to be a little more complex, so just assume it's present for setting the default
   //subtract one to allow for the OS
   SYSTEM_INFO vSysInfo;
   GetSystemInfo(&vSysInfo);
   int iDefaultThreads = max((int)vSysInfo.dwNumberOfProcessors / 2, 1);

   m_bExcludeForcedMoves = theApp.GetProfileInt(_T("PGNSpy"), _T("ExcludeForcedMoves"), 1) == 1;
   m_iForcedMoveCutoff = theApp.GetProfileInt(_T("PGNSpy"), _T("ForcedMoveCutoff"), 50);
   m_bIncludeOnlyUnclearPositions = theApp.GetProfileInt(_T("PGNSpy"), _T("IncludeOnlyUnclearPositions"), 1) == 1;
   m_iUnclearPositionCutoff = theApp.GetProfileInt(_T("PGNSpy"), _T("UnclearPositionCutoff"), 100);
   m_iEqualPositionThreshold = theApp.GetProfileInt(_T("PGNSpy"), _T("EqualPositionThreshold"), 200);
   m_iLosingThreshold = theApp.GetProfileInt(_T("PGNSpy"), _T("LosingThreshold"), 500);
   m_bIncludeLosing = theApp.GetProfileInt(_T("PGNSpy"), _T("IncludeLosing"), 1) == 1;
   m_bIncludeWinning = theApp.GetProfileInt(_T("PGNSpy"), _T("IncludeWinning"), 0) == 1;
   m_bIncludePostLosing = theApp.GetProfileInt(_T("PGNSpy"), _T("IncludePostLosing"), 0) == 1;
   return true;
}

bool CAnalysisSettings::SaveSettingsToRegistry()
{
   theApp.WriteProfileInt(_T("PGNSpy"), _T("ExcludeForcedMoves"), m_bExcludeForcedMoves ? 1 : 0);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("ForcedMoveCutoff"), m_iForcedMoveCutoff);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("IncludeOnlyUnclearPositions"), m_bIncludeOnlyUnclearPositions ? 1 : 0);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("UnclearPositionCutoff"), m_iUnclearPositionCutoff);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("EqualPositionThreshold"), m_iEqualPositionThreshold);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("LosingThreshold"), m_iLosingThreshold);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("IncludeLosing"), m_bIncludeLosing ? 1 : 0);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("IncludeWinning"), m_bIncludeWinning ? 1 : 0);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("IncludePostLosing"), m_bIncludePostLosing ? 1 : 0);
   return true;
}

CEngineSettings::CEngineSettings()
{
   m_iBookDepth = 10;
   m_iNumVariations = 3;
   m_iSearchDepth = 20;
   m_iMaxTime = 20000;
   m_iMinTime = 10000;
   m_iHashSize = 24;

   //calc number of cores and divide by two; engines aren't great with hyper-threading; checking for
   //hyper-threading seems to be a little more complex, so just assume it's present for setting the default
   //subtract one to allow for the OS
   SYSTEM_INFO vSysInfo;
   GetSystemInfo(&vSysInfo);
   m_iParallelGames = max((int)vSysInfo.dwNumberOfProcessors / 2, 1);
   m_iEngineThreads = 1;
}

bool CEngineSettings::LoadSettingsFromRegistry()
{
   //calc number of cores and divide by two; engines aren't great with hyper-threading; checking for
   //hyper-threading seems to be a little more complex, so just assume it's present for setting the default
   //subtract one to allow for the OS
   SYSTEM_INFO vSysInfo;
   GetSystemInfo(&vSysInfo);
   int iDefaultParallelGames = max((int)vSysInfo.dwNumberOfProcessors / 2, 1);
   int iLegacyThreads = theApp.GetProfileInt(_T("PGNSpy"), _T("NumThreads"), iDefaultParallelGames);

   m_iBookDepth = theApp.GetProfileInt(_T("PGNSpy"), _T("BookDepth"), 10);
   m_sEnginePath = theApp.GetProfileString(_T("PGNSpy"), _T("EnginePath"), _T(""));
   m_iNumVariations = theApp.GetProfileInt(_T("PGNSpy"), _T("NumVariations"), 3);
   m_iSearchDepth = theApp.GetProfileInt(_T("PGNSpy"), _T("SearchDepth"), 20);
   m_iMaxTime = theApp.GetProfileInt(_T("PGNSpy"), _T("MaxTime"), 20000);
   m_iMinTime = theApp.GetProfileInt(_T("PGNSpy"), _T("MinTime"), 10000);
   m_iParallelGames = theApp.GetProfileInt(_T("PGNSpy"), _T("ParallelGames"), iLegacyThreads);
   m_iEngineThreads = theApp.GetProfileInt(_T("PGNSpy"), _T("EngineThreads"), 1);
   m_iHashSize = theApp.GetProfileInt(_T("PGNSpy"), _T("HashSize"), 24);
   return true;
}

bool CEngineSettings::SaveSettingsToRegistry()
{
   theApp.WriteProfileInt(_T("PGNSpy"), _T("BookDepth"), m_iBookDepth);
   theApp.WriteProfileString(_T("PGNSpy"), _T("EnginePath"), m_sEnginePath);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("NumVariations"), m_iNumVariations);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("SearchDepth"), m_iSearchDepth);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("MaxTime"), m_iMaxTime);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("MinTime"), m_iMinTime);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("ParallelGames"), m_iParallelGames);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("EngineThreads"), m_iEngineThreads);
   theApp.WriteProfileInt(_T("PGNSpy"), _T("HashSize"), m_iHashSize);
   return true;
}

CEngineSettings CEngineSettings::MakeCompatible(const CEngineSettings vOtherEngineSettings, CString &rsWarning) const
{
   CEngineSettings vEngineSettings = *this;
   rsWarning = _T("");

   //A mismatch on this setting should be resolved by blanking out the setting, but doesn't require a warning
   if (m_sPlayerName.CompareNoCase(vOtherEngineSettings.m_sPlayerName) != 0)
   {
      vEngineSettings.m_sPlayerName = _T("");
   }

   //A mismatch here needs resolved; we will only have partial data for the larger number of variations, so
   //simply ignore that data
   if (m_iNumVariations != vOtherEngineSettings.m_iNumVariations)
   {
      if (!rsWarning.IsEmpty())
         rsWarning += _T("\r\n");
      rsWarning = _T("The number of variations does not match.  Only the lower number will be preserved.");
      vEngineSettings.m_iNumVariations = min(m_iNumVariations, vOtherEngineSettings.m_iNumVariations);
   }

   //the user should be warned about mismatches on the following settings, but otherwise we can ignore it
   if (m_iSearchDepth != vOtherEngineSettings.m_iSearchDepth || m_iMaxTime != vOtherEngineSettings.m_iMaxTime || m_iMinTime != vOtherEngineSettings.m_iMinTime)
   {
      if (!rsWarning.IsEmpty())
         rsWarning += _T("\r\n");
      rsWarning += _T("The search depth and/or time settings do not match.");
   }
   if (m_iHashSize != vOtherEngineSettings.m_iHashSize)
   {
      if (!rsWarning.IsEmpty())
         rsWarning += _T("\r\n");
      rsWarning += _T("The hash size setting does not match.");
   }
   if (m_iEngineThreads != vOtherEngineSettings.m_iEngineThreads)
   {
      if (!rsWarning.IsEmpty())
         rsWarning += _T("\r\n");
      rsWarning += _T("The engine thread setting does not match.");
   }
   if (m_iBookDepth != vOtherEngineSettings.m_iBookDepth)
   {
      if (!rsWarning.IsEmpty())
         rsWarning += _T("\r\n");
      rsWarning += _T("The book depth setting does not match.");
   }
   if (m_sEnginePath.CompareNoCase(vOtherEngineSettings.m_sEnginePath) != 0)
   {
      CString sEngineName, sEngineNameOther;
      int iLastSlash = m_sEnginePath.ReverseFind('\\');
      if (iLastSlash != -1)
         sEngineName = m_sEnginePath.Right(m_sEnginePath.GetLength() - iLastSlash - 1);
      iLastSlash = vOtherEngineSettings.m_sEnginePath.ReverseFind('\\');
      if (iLastSlash != -1)
         sEngineNameOther = vOtherEngineSettings.m_sEnginePath.Right(vOtherEngineSettings.m_sEnginePath.GetLength() - iLastSlash - 1);

      if (sEngineName.CompareNoCase(sEngineNameOther) != 0)
      {
         if (!rsWarning.IsEmpty())
            rsWarning += _T("\r\n");
         rsWarning += _T("The engine does not match.");
      }
   }
   return vEngineSettings;
}


CMove::CMove()
{
   m_iDepth = 0;
   m_iTime = 0;
   m_iScore = 0;
}

CPosition::CPosition()
{
   m_iMovePlayed = 0;
   m_bWhite = false;
}

CPosition::CPosition(const CPosition &rSrc)
{
   *this = rSrc;
}

CPosition &CPosition::operator=(const CPosition &rSrc)
{
   m_iMovePlayed = rSrc.m_iMovePlayed;
   m_bWhite = rSrc.m_bWhite;
   m_avTopMoves.Copy(rSrc.m_avTopMoves);
   return *this;
}

bool CPosition::IsForcedMove(int iVariation, int iForcedMoveThreshold)
{
   //ensure there's at least one move not significantly worse than the current move
   if (m_avTopMoves.GetSize() <= iVariation + 1)
      return true; //not enough legal moves
   if (m_avTopMoves[iVariation].m_iScore > m_avTopMoves[iVariation + 1].m_iScore + iForcedMoveThreshold)
      return true;
   return false;
}

bool CPosition::IsUnclearPosition(int iVariation, int iUnclearPositionThreshold)
{
   //ensure n+1 move is not significantly worse than the first-choice move
   if (m_avTopMoves.GetSize() <= iVariation + 1)
      return false; //not enough legal moves
   if (m_avTopMoves[0].m_iScore > m_avTopMoves[iVariation + 1].m_iScore + iUnclearPositionThreshold)
      return false;
   return true;
}

bool CPosition::IsEqualPosition(int iEqualPositionThreshold)
{
   return abs(m_avTopMoves[0].m_iScore) <= iEqualPositionThreshold;
}

bool CPosition::IsLosingPosition(int iEqualPositionThreshold, int iLosingPositionThreshold)
{
   if (IsEqualPosition(iEqualPositionThreshold))
      return false;
   return m_avTopMoves[0].m_iScore < 0 && abs(m_avTopMoves[0].m_iScore) <= iLosingPositionThreshold;
}

bool CPosition::IsWinningPosition(int iEqualPositionThreshold, int iLosingPositionThreshold)
{
   if (IsEqualPosition(iEqualPositionThreshold))
      return false;
   return m_avTopMoves[0].m_iScore > 0 && abs(m_avTopMoves[0].m_iScore) <= iLosingPositionThreshold;
}

bool CPosition::IsExcludedPosition(int iLosingPositionThreshold)
{
   return abs(m_avTopMoves[0].m_iScore) > iLosingPositionThreshold;
}

int CPosition::GetCentipawnLoss()
{
   //[0] will have the highest value; positive is winning, negative losing
   return m_avTopMoves[0].m_iScore - m_avTopMoves[m_iMovePlayed].m_iScore;
}

CGame::CGame(const CGame &rSrc)
{
   *this = rSrc;
}

CGame &CGame::operator=(const CGame &rSrc)
{
   m_sEvent = rSrc.m_sEvent;
   m_sDate = rSrc.m_sDate;
   m_sWhite = rSrc.m_sWhite;
   m_sBlack = rSrc.m_sBlack;
   m_sResult = rSrc.m_sResult;
   m_sTimeControl = rSrc.m_sTimeControl;
    m_sUCIGameText = rSrc.m_sUCIGameText;
   m_avPositions.Copy(rSrc.m_avPositions);

   return *this;
}

bool CGame::LoadGame(CString sGameText)
{
   CMarkup vGame;
   if (!vGame.SetDoc(sGameText))
      return false;
   vGame.SetDocFlags(vGame.GetDocFlags() | CMarkup::MDF_COLLAPSEWHITESPACE);

   if (!vGame.FindElem()) //find root elem <gamelist>
      return false;
   if (!vGame.IntoElem())
      return false;
   if (!vGame.FindElem(_T("game")))
      return false;
   if (!vGame.IntoElem())
      return false;
   if (!vGame.FindElem(_T("tags")))
      return false;
   if (!vGame.IntoElem())
      return false;
   //load tags
   m_sUCIGameText.Empty();
   while (vGame.FindElem(_T("tag")))
   {
      CString sName = vGame.GetAttrib(_T("name"));
      CString sValue = vGame.GetAttrib(_T("value"));
      CString sTagLine;
      sTagLine.Format(_T("[%s \"%s\"]"), sName, sValue);
      m_sUCIGameText += sTagLine + _T("\r\n");
      if (sName.CompareNoCase(_T("Event")) == 0)
         m_sEvent = sValue;
      else if (sName.CompareNoCase(_T("Date")) == 0)
         m_sDate = sValue;
      else if (sName.CompareNoCase(_T("White")) == 0)
         m_sWhite = sValue;
      else if (sName.CompareNoCase(_T("Black")) == 0)
         m_sBlack = sValue;
      else if (sName.CompareNoCase(_T("Result")) == 0)
         m_sResult = sValue;
      else if (sName.CompareNoCase(_T("TimeControl")) == 0)
         m_sTimeControl = sValue;
      else
      {
         //ignore tag
      }
   }
   //done loading tags, step out
   if (!vGame.OutOfElem())
      return false;
   if (!vGame.FindElem(_T("moves")))
      return false;
   CString sMovesText = vGame.GetData();
   if (!m_sUCIGameText.IsEmpty())
      m_sUCIGameText += _T("\r\n");
   m_sUCIGameText += sMovesText;
   if (!vGame.FindElem(_T("analysis")))
      return false;
   if (!vGame.IntoElem())
      return false;
   //load moves
   while (vGame.FindElem(_T("move")))
   {
      CPosition vPosition;
      CString sMovePlayed;
      CString sText;
      if (!vGame.IntoElem())
         return false;
      if (!vGame.FindElem(_T("played")))
         return false;
      sMovePlayed = vGame.GetData();
      if (sMovePlayed.IsEmpty())
         return false;
      if (!vGame.FindElem(_T("white")))
         return false;
      sText = vGame.GetData();
      vPosition.m_bWhite = _ttoi(sText) == 1;
      while (vGame.FindElem(_T("evaluation")))
      {
         //find all moves and their evaluation
         CMove vMove;
         vMove.m_sMove = vGame.GetAttrib(_T("move"));
         if (vMove.m_sMove.CompareNoCase(sMovePlayed) == 0)
            vPosition.m_iMovePlayed = vPosition.m_avTopMoves.GetSize();//new move will be inserted at this point, so using GetSize will be valid
         sText = vGame.GetAttrib(_T("depth"));
         vMove.m_iDepth = _ttoi(sText);
         sText = vGame.GetAttrib(_T("time"));
         vMove.m_iTime = _ttoi(sText);
         sText = vGame.GetAttrib(_T("value"));
         vMove.m_sRawScore = sText;
         if (sText.Find(_T("mate")) == 0)
         {
            //sText will take the format _T("mate n"), with n positive if we have a mate, and negative if we're being mated
            CString sMateMoves = sText.Right(sText.GetLength() - 5);
            int iMateMoves = _ttoi(sMateMoves);
            if (iMateMoves > 0)
               vMove.m_iScore = 1000;
            else
               vMove.m_iScore = -1000;
         }
         else
         {
            vMove.m_iScore = _ttoi(sText);
            if (vMove.m_iScore > 1000)
               vMove.m_iScore = 1000;
            else if (vMove.m_iScore < -1000)
               vMove.m_iScore = -1000;
         }
         vPosition.m_avTopMoves.Add(vMove);
      }
      if (!vGame.OutOfElem())
         return false;
      ASSERT(vPosition.m_avTopMoves.GetSize()>0);
      m_avPositions.Add(vPosition);
   }
   return true;
}

CStats::CStats()
{
   ZeroAll();
}

void CStats::ZeroAll()
{
   m_iNumVariations = 0;
   m_iNumPositions = 0;
   m_iTotalCentipawnLoss = 0;
   m_aiCentipawnLosses.RemoveAll();
   m_dAvgCentipawnLoss = 0;
   m_dCentipawnLossStdDeviation = 0;
   m_i0CPLoss = 0;
   m_i10CPLoss = 0;
   m_i25CPLoss = 0;
   m_i50CPLoss = 0;
   m_i100CPLoss = 0;
   m_i200CPLoss = 0;
   m_i500CPLoss = 0;
}

void CStats::Initialize(const CEngineSettings &vSettings)
{
   ZeroAll();
   m_iNumVariations = vSettings.m_iNumVariations;
   m_aiTValues.SetSize(vSettings.m_iNumVariations + 1);
   m_aiTMoves.SetSize(vSettings.m_iNumVariations + 1);
   for (int i = 0; i < vSettings.m_iNumVariations + 1; i++)
   {
      m_aiTValues[i] = 0;
      m_aiTMoves[i] = 0;
   }
}

void CStats::AddPosition(CPosition &vPosition, const CAnalysisSettings &vSettings)
{
   m_iNumPositions++;
   m_iTotalCentipawnLoss += vPosition.GetCentipawnLoss();

   int iCPLossIndex = 0;
   int iCPLoss = vPosition.GetCentipawnLoss();
   while (iCPLossIndex < m_aiCentipawnLosses.GetSize() && m_aiCentipawnLosses[iCPLossIndex] > iCPLoss)
      iCPLossIndex++; //sort cp loss array highest to lowest
   m_aiCentipawnLosses.InsertAt(iCPLossIndex, iCPLoss);

   if (iCPLoss > 0)
      m_i0CPLoss++;
   if (iCPLoss > 10)
      m_i10CPLoss++;
   if (iCPLoss > 25)
      m_i25CPLoss++;
   if (iCPLoss > 50)
      m_i50CPLoss++;
   if (iCPLoss > 100)
      m_i100CPLoss++;
   if (iCPLoss > 200)
      m_i200CPLoss++;
   if (iCPLoss > 500)
      m_i500CPLoss++;

   for (int i = 0; i < m_iNumVariations; i++)
   {
      if (vSettings.m_bExcludeForcedMoves && vPosition.IsForcedMove(i, vSettings.m_iForcedMoveCutoff))
         break; //exclude forced moves
      if (vSettings.m_bIncludeOnlyUnclearPositions && !vPosition.IsUnclearPosition(i, vSettings.m_iUnclearPositionCutoff))
         break; //include only unclear positions

      //increment T1/T2/T3/etc. values as appropriate
      m_aiTMoves[i]++;
      if (vPosition.m_iMovePlayed <= i)
         m_aiTValues[i]++;
   }
}

void CStats::FinaliseStats()
{
   if (m_iNumPositions > 0)
      m_dAvgCentipawnLoss = (double)m_iTotalCentipawnLoss / (double)m_iNumPositions;
   else
      m_dAvgCentipawnLoss = 0;

   double dTotalVariance = 0;
   for (int i = 0; i < m_iNumPositions; i++)
   {
      double dDiffFromMean = (double)m_aiCentipawnLosses[i] - m_dAvgCentipawnLoss;
      dTotalVariance += dDiffFromMean * dDiffFromMean;
   }

   double dVariance;
   if (m_iNumPositions > 0)
      dVariance = dTotalVariance / (double)m_iNumPositions;
   else
      dVariance = 0;
   m_dCentipawnLossStdDeviation = sqrt(dVariance);
}

CString CStats::GetResultsText()
{
   CString sLine;
   CString sResults;
   sLine.Format(Loc(_T("Positions: %i"), _T("Позиции: %i")), m_iNumPositions);
   sResults = sLine + _T("\r\n");
   if (m_iNumPositions > 0)
   {
      //T-values
      for (int i = 0; i < m_iNumVariations; i++)
      {
         if (m_aiTMoves[i] == 0)
            sLine.Format(_T("T%i: 0/0"), i+1);
         else
         {
            double dFrac = ((double)m_aiTValues[i] / (double)m_aiTMoves[i]);
            double dStdError = sqrt(dFrac * (1 - dFrac) / m_aiTMoves[i]) * 100;
            sLine.Format(Loc(_T("T%i: %i/%i; %.2f%% (std error %.2f)"), _T("T%i: %i/%i; %.2f%% (стд. ошибка %.2f)")), i + 1, m_aiTValues[i], m_aiTMoves[i], dFrac*100.0, dStdError);
         }
         sResults += sLine + _T("\r\n");
      }
      //=0 CP loss
      {
         double dFrac = ((double)(m_iNumPositions - m_i0CPLoss) / (double)m_iNumPositions);
         double dStdError = sqrt(dFrac * (1 - dFrac) / m_iNumPositions) * 100;
         sLine.Format(Loc(_T("=0 CP loss: %i/%i; %.2f%% (std error %.2f)"), _T("=0 потерь в сотых пешки: %i/%i; %.2f%% (стд. ошибка %.2f)")), m_iNumPositions - m_i0CPLoss, m_iNumPositions, dFrac*100.0, dStdError);
         sResults += sLine + _T("\r\n");
      }
      //>0 CP loss
      {
         double dFrac = ((double)m_i0CPLoss / (double)m_iNumPositions);
         double dStdError = sqrt(dFrac * (1 - dFrac) / m_iNumPositions) * 100;
         sLine.Format(Loc(_T(">0 CP loss: %i/%i; %.2f%% (std error %.2f)"), _T(">0 потерь в сотых пешки: %i/%i; %.2f%% (стд. ошибка %.2f)")), m_i0CPLoss, m_iNumPositions, dFrac*100.0, dStdError);
         sResults += sLine + _T("\r\n");
      }
      //>10 CP loss
      {
         double dFrac = ((double)m_i10CPLoss / (double)m_iNumPositions);
         double dStdError = sqrt(dFrac * (1 - dFrac) / m_iNumPositions) * 100;
         sLine.Format(Loc(_T(">10 CP loss: %i/%i; %.2f%% (std error %.2f)"), _T(">10 потерь в сотых пешки: %i/%i; %.2f%% (стд. ошибка %.2f)")), m_i10CPLoss, m_iNumPositions, dFrac*100.0, dStdError);
         sResults += sLine + _T("\r\n");
      }
      //>25 CP loss
      {
         double dFrac = ((double)m_i25CPLoss / (double)m_iNumPositions);
         double dStdError = sqrt(dFrac * (1 - dFrac) / m_iNumPositions) * 100;
         sLine.Format(Loc(_T(">25 CP loss: %i/%i; %.2f%% (std error %.2f)"), _T(">25 потерь в сотых пешки: %i/%i; %.2f%% (стд. ошибка %.2f)")), m_i25CPLoss, m_iNumPositions, dFrac*100.0, dStdError);
         sResults += sLine + _T("\r\n");
      }
      //>50 CP loss
      {
         double dFrac = ((double)m_i50CPLoss / (double)m_iNumPositions);
         double dStdError = sqrt(dFrac * (1 - dFrac) / m_iNumPositions) * 100;
         sLine.Format(Loc(_T(">50 CP loss: %i/%i; %.2f%% (std error %.2f)"), _T(">50 потерь в сотых пешки: %i/%i; %.2f%% (стд. ошибка %.2f)")), m_i50CPLoss, m_iNumPositions, dFrac*100.0, dStdError);
         sResults += sLine + _T("\r\n");
      }
      //>100 CP loss
      {
         double dFrac = ((double)m_i100CPLoss / (double)m_iNumPositions);
         double dStdError = sqrt(dFrac * (1 - dFrac) / m_iNumPositions) * 100;
         sLine.Format(Loc(_T(">100 CP loss: %i/%i; %.2f%% (std error %.2f)"), _T(">100 потерь в сотых пешки: %i/%i; %.2f%% (стд. ошибка %.2f)")), m_i100CPLoss, m_iNumPositions, dFrac*100.0, dStdError);
         sResults += sLine + _T("\r\n");
      }
      //>200 CP loss
      {
         double dFrac = ((double)m_i200CPLoss / (double)m_iNumPositions);
         double dStdError = sqrt(dFrac * (1 - dFrac) / m_iNumPositions) * 100;
         sLine.Format(Loc(_T(">200 CP loss: %i/%i; %.2f%% (std error %.2f)"), _T(">200 потерь в сотых пешки: %i/%i; %.2f%% (стд. ошибка %.2f)")), m_i200CPLoss, m_iNumPositions, dFrac*100.0, dStdError);
         sResults += sLine + _T("\r\n");
      }
      //>500 CP loss
      {
         double dFrac = ((double)m_i500CPLoss / (double)m_iNumPositions);
         double dStdError = sqrt(dFrac * (1 - dFrac) / m_iNumPositions) * 100;
         sLine.Format(Loc(_T(">500 CP loss: %i/%i; %.2f%% (std error %.2f)"), _T(">500 потерь в сотых пешки: %i/%i; %.2f%% (стд. ошибка %.2f)")), m_i500CPLoss, m_iNumPositions, dFrac*100.0, dStdError);
         sResults += sLine + _T("\r\n");
      }

      sLine.Format(Loc(_T("CP loss mean %.2f, std deviation %.2f"), _T("Средняя потеря %.2f, стандартное отклонение %.2f")), m_dAvgCentipawnLoss,m_dCentipawnLossStdDeviation);
      sResults += sLine + _T("\r\n");
   }
   return sResults;
}

bool LoadGameArrayFromFile(CString sFileName, CArray<CGame, CGame> &raGames, CEngineSettings &rvEngineSettings)
{
   raGames.RemoveAll();
   CMarkup vFile;
   if (!vFile.Load(sFileName))
      return false;
   if (!vFile.FindElem(_T("Games")))
      return false;
   vFile.IntoElem();

   //engine settings
   if (!vFile.FindElem(_T("EngineSettings")))
      return false;
   vFile.IntoElem();
   vFile.FindElem(_T("EnginePath"));
   rvEngineSettings.m_sEnginePath = vFile.GetData();
   vFile.FindElem(_T("NumVariations"));
   rvEngineSettings.m_iNumVariations = _ttoi(vFile.GetData());
   vFile.FindElem(_T("SearchDepth"));
   rvEngineSettings.m_iSearchDepth = _ttoi(vFile.GetData());
   vFile.FindElem(_T("MinTime"));
   rvEngineSettings.m_iMinTime = _ttoi(vFile.GetData());
   vFile.FindElem(_T("MaxTime"));
   rvEngineSettings.m_iMaxTime = _ttoi(vFile.GetData());
   vFile.FindElem(_T("HashSize"));
   rvEngineSettings.m_iHashSize = _ttoi(vFile.GetData());
   if (vFile.FindElem(_T("EngineThreads")))
      rvEngineSettings.m_iEngineThreads = _ttoi(vFile.GetData());
   else
      rvEngineSettings.m_iEngineThreads = 1;
   if (vFile.FindElem(_T("ParallelGames")))
      rvEngineSettings.m_iParallelGames = _ttoi(vFile.GetData());
   else
      rvEngineSettings.m_iParallelGames = 1;
   vFile.FindElem(_T("BookDepth"));
   rvEngineSettings.m_iBookDepth = _ttoi(vFile.GetData());
   vFile.FindElem(_T("PlayerName"));
   rvEngineSettings.m_sPlayerName = vFile.GetData();
   vFile.OutOfElem();

   //loop through all games
   while (vFile.FindElem(_T("Game")))
   {
      CGame vGame;
      vFile.IntoElem();

      vFile.FindElem(_T("Event"));
      vGame.m_sEvent = vFile.GetData();
      vFile.FindElem(_T("Date"));
      vGame.m_sDate = vFile.GetData();
      vFile.FindElem(_T("White"));
      vGame.m_sWhite = vFile.GetData();
      vFile.FindElem(_T("Black"));
      vGame.m_sBlack = vFile.GetData();
      vFile.FindElem(_T("Result"));
      vGame.m_sResult = vFile.GetData();
      vFile.FindElem(_T("TimeControl"));
      vGame.m_sTimeControl = vFile.GetData();
      if (vFile.FindElem(_T("UCIGameText")))
         vGame.m_sUCIGameText = vFile.GetData();

      //loop through all positions
      vFile.FindElem(_T("Positions"));
      vFile.IntoElem();
      while (vFile.FindElem(_T("Position")))
      {
         CPosition vPosition;
         vFile.IntoElem();

         vFile.FindElem(_T("White"));
         vPosition.m_bWhite = _ttoi(vFile.GetData()) == 1;
         vFile.FindElem(_T("MovePlayed"));
         vPosition.m_iMovePlayed = _ttoi(vFile.GetData());

         //loop through all moves
         vFile.FindElem(_T("Moves"));
         vFile.IntoElem();
         while (vFile.FindElem(_T("Move")))
         {
            CMove vMove;
            vFile.IntoElem();
            vFile.FindElem(_T("Move"));
            vMove.m_sMove = vFile.GetData();
            vFile.FindElem(_T("Depth"));
            vMove.m_iDepth = _ttoi(vFile.GetData());
            vFile.FindElem(_T("Time"));
            vMove.m_iTime = _ttoi(vFile.GetData());
            vFile.FindElem(_T("Score"));
            vMove.m_iScore = _ttoi(vFile.GetData());
            if (vFile.FindElem(_T("RawScore")))
               vMove.m_sRawScore = vFile.GetData();

            vPosition.m_avTopMoves.Add(vMove);
            vFile.OutOfElem(); //</Move>
         }

         vGame.m_avPositions.Add(vPosition);
         vFile.OutOfElem(); //</Moves>
         vFile.OutOfElem(); //</Position>
      }

      vFile.OutOfElem(); //</Positions>
      raGames.Add(vGame);
      vFile.OutOfElem(); //</Game>
   }

   return true;
}

bool SaveGameArrayToFile(CString sFileName, const CArray<CGame, CGame> &raGames, CEngineSettings vEngineSettings)
{
   if (raGames.GetSize() == 0)
      return false;

   CMarkup vFile;
   vFile.AddElem(_T("Games"));
   vFile.IntoElem();

   //engine settings
   vFile.AddElem(_T("EngineSettings"));
   vFile.IntoElem();
   vFile.AddElem(_T("EnginePath"), vEngineSettings.m_sEnginePath);
   vFile.AddElem(_T("NumVariations"), vEngineSettings.m_iNumVariations);
   vFile.AddElem(_T("SearchDepth"), vEngineSettings.m_iSearchDepth);
   vFile.AddElem(_T("MinTime"), vEngineSettings.m_iMinTime);
   vFile.AddElem(_T("MaxTime"), vEngineSettings.m_iMaxTime);
   vFile.AddElem(_T("HashSize"), vEngineSettings.m_iHashSize);
    vFile.AddElem(_T("EngineThreads"), vEngineSettings.m_iEngineThreads);
   vFile.AddElem(_T("ParallelGames"), vEngineSettings.m_iParallelGames);
   vFile.AddElem(_T("BookDepth"), vEngineSettings.m_iBookDepth);
   vFile.AddElem(_T("PlayerName"), vEngineSettings.m_sPlayerName);
   vFile.OutOfElem();

   //loop through all games
   for (int iGame = 0; iGame < raGames.GetSize(); iGame++)
   {
      vFile.AddElem(_T("Game"));
      vFile.IntoElem();
      vFile.AddElem(_T("Event"), raGames[iGame].m_sEvent);
      vFile.AddElem(_T("Date"), raGames[iGame].m_sDate);
      vFile.AddElem(_T("White"), raGames[iGame].m_sWhite);
      vFile.AddElem(_T("Black"), raGames[iGame].m_sBlack);
      vFile.AddElem(_T("Result"), raGames[iGame].m_sResult);
      vFile.AddElem(_T("TimeControl"), raGames[iGame].m_sTimeControl);
      vFile.AddElem(_T("UCIGameText"), raGames[iGame].m_sUCIGameText);

      //loop through all positions
      vFile.AddElem(_T("Positions"));
      vFile.IntoElem();
      for (int iPosition = 0; iPosition < raGames[iGame].m_avPositions.GetSize(); iPosition++)
      {
         vFile.AddElem(_T("Position"));
         vFile.IntoElem();
         vFile.AddElem(_T("White"), raGames[iGame].m_avPositions[iPosition].m_bWhite ? 1 : 0);
         vFile.AddElem(_T("MovePlayed"), raGames[iGame].m_avPositions[iPosition].m_iMovePlayed);

         //loop through all moves
         vFile.AddElem(_T("Moves"));
         vFile.IntoElem();
         for (int iMove = 0; iMove < raGames[iGame].m_avPositions[iPosition].m_avTopMoves.GetSize(); iMove++)
         {
            vFile.AddElem(_T("Move"));
            vFile.IntoElem();
            vFile.AddElem(_T("Move"), raGames[iGame].m_avPositions[iPosition].m_avTopMoves[iMove].m_sMove);
            vFile.AddElem(_T("Depth"), raGames[iGame].m_avPositions[iPosition].m_avTopMoves[iMove].m_iDepth);
            vFile.AddElem(_T("Time"), raGames[iGame].m_avPositions[iPosition].m_avTopMoves[iMove].m_iTime);
            vFile.AddElem(_T("Score"), raGames[iGame].m_avPositions[iPosition].m_avTopMoves[iMove].m_iScore);
            vFile.AddElem(_T("RawScore"), raGames[iGame].m_avPositions[iPosition].m_avTopMoves[iMove].m_sRawScore);

            vFile.OutOfElem(); //</Move>
         }
         vFile.OutOfElem(); //</Moves>
         vFile.OutOfElem(); //</Position>
      }
      vFile.OutOfElem(); //</Positions>
      vFile.OutOfElem(); //</Game>
   }

   if (!vFile.Save(sFileName))
      return false;
   return true;
}

bool CanExportAnnotatedPGN(const CArray<CGame, CGame> &raGames, CString &rsReason)
{
   rsReason.Empty();
   if (raGames.GetSize() == 0)
   {
      rsReason = Loc(_T("There are no analysis results to export."), _T("Нет результатов анализа для экспорта."));
      return false;
   }

   for (int iGame = 0; iGame < raGames.GetSize(); iGame++)
   {
      if (raGames[iGame].m_sUCIGameText.IsEmpty())
      {
         rsReason = Loc(_T("Annotated PGN export requires results saved by the new format. Legacy XML files do not contain the original move list."), _T("Экспорт аннотированного PGN требует результаты, сохранённые в новом формате. Старые XML-файлы не содержат исходного списка ходов."));
         return false;
      }
   }

   return true;
}

bool ExportGameArrayToAnnotatedPGN(CString sFileName, const CArray<CGame, CGame> &raGames, const CEngineSettings &vEngineSettings, CString &rsError)
{
   rsError.Empty();
   if (!CanExportAnnotatedPGN(raGames, rsError))
      return false;

   CString sCombinedText;
   for (int iGame = 0; iGame < raGames.GetSize(); iGame++)
   {
      CString sGameText;
      if (!BuildAnnotatedUCIGameText(raGames[iGame], vEngineSettings, sGameText, rsError))
         return false;

      if (!sCombinedText.IsEmpty())
         sCombinedText += _T("\r\n\r\n");
      sCombinedText += sGameText;
   }

   CString sTempInput = GetTemporaryPGNFilePath(0);
   CFile vFile;
   if (!vFile.Open(sTempInput, CFile::modeCreate | CFile::modeWrite))
   {
      rsError = Loc(_T("Failed to create a temporary PGN file for export."), _T("Не удалось создать временный PGN-файл для экспорта."));
      return false;
   }
   if (!WriteCStringToFile(vFile, sCombinedText))
   {
      vFile.Close();
      rsError = Loc(_T("Failed to write the temporary PGN file for export."), _T("Не удалось записать временный PGN-файл для экспорта."));
      DeleteFile(sTempInput);
      return false;
   }
   vFile.Close();

   bool bSuccess = RunConverterToSAN(sTempInput, sFileName, rsError);
   DeleteFile(sTempInput);
   return bSuccess;
}

static void TokenizeWhitespace(const CString &sText, CStringArray &rasTokens)
{
   rasTokens.RemoveAll();
   CString sCurrentToken;
   for (int i = 0; i < sText.GetLength(); i++)
   {
      TCHAR c = sText[i];
      if (_istspace(c))
      {
         if (!sCurrentToken.IsEmpty())
         {
            rasTokens.Add(sCurrentToken);
            sCurrentToken.Empty();
         }
      }
      else
      {
         sCurrentToken += c;
      }
   }
   if (!sCurrentToken.IsEmpty())
      rasTokens.Add(sCurrentToken);
}

static bool IsResultToken(const CString &sToken)
{
   return sToken == _T("1-0") ||
      sToken == _T("0-1") ||
      sToken == _T("1/2-1/2") ||
      sToken == _T("*");
}

static CString FormatRawScoreForPGN(const CString &sRawScore, int iNumericScore)
{
   CString sTrimmed = sRawScore;
   sTrimmed.Trim();

   if (sTrimmed.Left(5).CompareNoCase(_T("mate ")) == 0)
   {
      int iMate = _ttoi(sTrimmed.Mid(5));
      CString sMate;
      if (iMate < 0)
         sMate.Format(_T("#-%i"), abs(iMate));
      else
         sMate.Format(_T("#%i"), iMate);
      return sMate;
   }

   int iScore = iNumericScore;
   if (!sTrimmed.IsEmpty())
   {
      int iSpace = sTrimmed.Find(' ');
      CString sNumericToken = (iSpace == -1) ? sTrimmed : sTrimmed.Left(iSpace);
      iScore = _ttoi(sNumericToken);
   }

   CString sFormatted;
   sFormatted.Format(_T("%s%i.%02i"), iScore < 0 ? _T("-") : _T(""), abs(iScore) / 100, abs(iScore) % 100);
   return sFormatted;
}

static CString GetPlayedMoveComment(const CPosition &vPosition)
{
   if (vPosition.m_iMovePlayed < 0 || vPosition.m_iMovePlayed >= vPosition.m_avTopMoves.GetSize())
      return _T("");

   const CMove &vMove = vPosition.m_avTopMoves[vPosition.m_iMovePlayed];
   return FormatRawScoreForPGN(vMove.m_sRawScore, vMove.m_iScore);
}

static bool BuildAnnotatedUCIGameText(const CGame &vGame, const CEngineSettings &vEngineSettings, CString &rsOutput, CString &rsError)
{
   rsOutput.Empty();
   rsError.Empty();

   CString sSource = vGame.m_sUCIGameText;
   sSource.Replace(_T("\r\n"), _T("\n"));
   sSource.Replace('\r', '\n');

   int iSeparator = sSource.Find(_T("\n\n"));
   if (iSeparator == -1)
   {
      rsError.Format(_T("Failed to export annotated PGN for %s v %s because the stored game text is incomplete."), vGame.m_sWhite, vGame.m_sBlack);
      return false;
   }

   CString sTags = sSource.Left(iSeparator);
   CString sMovesText = sSource.Mid(iSeparator + 2);
   sTags.Trim();
   sMovesText.Trim();
   if (sMovesText.IsEmpty())
   {
      rsError.Format(_T("Failed to export annotated PGN for %s v %s because no moves were stored."), vGame.m_sWhite, vGame.m_sBlack);
      return false;
   }

   bool bWhiteToMove = true;
   int iFenTag = sTags.Find(_T("[FEN \""));
   if (iFenTag != -1)
   {
      int iFenStart = iFenTag + 6;
      int iFenEnd = sTags.Find(_T("\"]"), iFenStart);
      if (iFenEnd == -1)
         iFenEnd = sTags.Find('\"', iFenStart);
      CString sFen = (iFenEnd == -1) ? _T("") : sTags.Mid(iFenStart, iFenEnd - iFenStart);
      int iSideField = sFen.Find(' ');
      if (iSideField != -1 && iSideField + 1 < sFen.GetLength())
         bWhiteToMove = (sFen[iSideField + 1] != 'b' && sFen[iSideField + 1] != 'B');
   }

   CStringArray asMoves;
   TokenizeWhitespace(sMovesText, asMoves);
   if (asMoves.GetSize() == 0)
   {
      rsError.Format(_T("Failed to export annotated PGN for %s v %s because the move list is empty."), vGame.m_sWhite, vGame.m_sBlack);
      return false;
   }

   bool bAnalyseWhite = true;
   bool bAnalyseBlack = true;
   if (!vEngineSettings.m_sPlayerName.IsEmpty())
   {
      bAnalyseWhite = vEngineSettings.m_sPlayerName.CompareNoCase(vGame.m_sWhite) == 0;
      bAnalyseBlack = vEngineSettings.m_sPlayerName.CompareNoCase(vGame.m_sBlack) == 0;
   }

   int iBookDepthPlies = max(vEngineSettings.m_iBookDepth * 2, 0);
   int iNonResultMovesSeen = 0;
   CString sAnnotatedMoves;
   int iPosition = 0;
   for (int iMove = 0; iMove < asMoves.GetSize(); iMove++)
   {
      CString sMove = asMoves[iMove];
      if (!sAnnotatedMoves.IsEmpty())
         sAnnotatedMoves += _T(" ");
      sAnnotatedMoves += sMove;

      if (IsResultToken(sMove))
         continue;

      bool bPastBook = iNonResultMovesSeen >= iBookDepthPlies;
      bool bSideSelected = (bWhiteToMove && bAnalyseWhite) || (!bWhiteToMove && bAnalyseBlack);
      if (bPastBook && bSideSelected && iPosition < vGame.m_avPositions.GetSize() && vGame.m_avPositions[iPosition].m_bWhite == bWhiteToMove)
      {
         CString sEval = GetPlayedMoveComment(vGame.m_avPositions[iPosition]);
         if (!sEval.IsEmpty())
            sAnnotatedMoves += _T(" { [%eval ") + sEval + _T("] }");
         iPosition++;
      }

      iNonResultMovesSeen++;
      bWhiteToMove = !bWhiteToMove;
   }

   if (iPosition != vGame.m_avPositions.GetSize())
   {
      rsError.Format(_T("Failed to export annotated PGN for %s v %s because the stored move list does not match the analysed positions."), vGame.m_sWhite, vGame.m_sBlack);
      return false;
   }

   rsOutput = sTags + _T("\r\n\r\n") + sAnnotatedMoves + _T("\r\n");
   return true;
}

static bool RunConverterToSAN(const CString &sInputPath, const CString &sOutputPath, CString &rsError)
{
   STARTUPINFO vStartupInfo = { 0 };
   vStartupInfo.cb = sizeof(STARTUPINFO);
   vStartupInfo.dwFlags = STARTF_USESHOWWINDOW;
   vStartupInfo.wShowWindow = SW_HIDE;

   PROCESS_INFORMATION vProcessInfo = { 0 };
   CString sCommandLine;
   sCommandLine.Format(_T("-s -Wsan -o\"%s\" \"%s\""), sOutputPath, sInputPath);
   if (!CreateProcess(GetConverterFilePath(), sCommandLine.GetBuffer(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &vStartupInfo, &vProcessInfo))
   {
      sCommandLine.ReleaseBuffer();
      rsError = Loc(_T("Failed to launch pgn-extract.exe to generate the annotated PGN."), _T("Не удалось запустить pgn-extract.exe для создания аннотированного PGN."));
      return false;
   }
   sCommandLine.ReleaseBuffer();

   WaitForSingleObject(vProcessInfo.hProcess, INFINITE);
   DWORD dwExitCode = 1;
   GetExitCodeProcess(vProcessInfo.hProcess, &dwExitCode);
   CloseHandle(vProcessInfo.hThread);
   CloseHandle(vProcessInfo.hProcess);

   if (dwExitCode != 0 || !PathFileExists(sOutputPath))
   {
      rsError = Loc(_T("pgn-extract.exe failed to generate the annotated PGN."), _T("pgn-extract.exe не смог создать аннотированный PGN."));
      return false;
   }

   return true;
}

/*
<games>
   <enginesettings>
      <enginepath>path</enginepath>
      <numvariations>3</numvariations>
      <searchdepth>20</searchdepth>
      <maxtime>20000</maxtime>
      <mintime>10000</mintime>
      <hashsize>512</hashsize>
      <bookdepth>10</bookdepth>
      <playername></playername>
   </enginesettings>
   <game>
      <event>event</event>
      <date>date</date>
      <white>white</white>
      <black>black</black>
      <result>result</result>
      <timecontrol>timecontrol</timecontrol>
      <positions>
         <position>
            <white>1</white>
            <moveplayed>3<moveplayed>
            <moves>
               <move>
                  <move>e2e4</move>
                  <depth>19</depth>
                  <time>20000</time>
                  <score>35</score>
               </move>
            </moves>
         </position>
      </positions>
   </game>
</games>
*/





