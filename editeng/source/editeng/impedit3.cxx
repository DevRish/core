/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */


#include <vcl/svapp.hxx>
#include <vcl/metaact.hxx>
#include <vcl/gdimtf.hxx>
#include <vcl/settings.hxx>
#include <vcl/window.hxx>

#include <editeng/tstpitem.hxx>
#include <editeng/lspcitem.hxx>
#include <editeng/flditem.hxx>
#include <editeng/forbiddenruleitem.hxx>
#include "impedit.hxx"
#include <editeng/editeng.hxx>
#include <editeng/editview.hxx>
#include <editeng/escapementitem.hxx>
#include <editeng/txtrange.hxx>
#include <editeng/udlnitem.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/lrspitem.hxx>
#include <editeng/ulspitem.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/wghtitem.hxx>
#include <editeng/postitem.hxx>
#include <editeng/langitem.hxx>
#include <editeng/scriptspaceitem.hxx>
#include <editeng/charscaleitem.hxx>
#include <editeng/numitem.hxx>

#include <svtools/colorcfg.hxx>
#include <svl/ctloptions.hxx>
#include <svl/asiancfg.hxx>

#include <editeng/hngpnctitem.hxx>
#include <editeng/forbiddencharacterstable.hxx>

#include <unotools/configmgr.hxx>

#include <math.h>
#include <vcl/metric.hxx>
#include <com/sun/star/i18n/BreakIterator.hpp>
#include <com/sun/star/i18n/ScriptType.hpp>
#include <com/sun/star/i18n/InputSequenceChecker.hpp>
#include <vcl/pdfextoutdevdata.hxx>
#include <i18nlangtag/mslangid.hxx>

#include <comphelper/processfactory.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <o3tl/safeint.hxx>
#include <o3tl/sorted_vector.hxx>
#include <osl/diagnose.h>
#include <comphelper/string.hxx>
#include <memory>
#include <set>

#include <vcl/outdev/ScopedStates.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::linguistic2;

constexpr OUStringLiteral CH_HYPH = u"-";

constexpr tools::Long WRONG_SHOW_MIN = 5;

namespace {

struct TabInfo
{
    bool        bValid;

    SvxTabStop  aTabStop;
    sal_Int32   nTabPortion;
    tools::Long        nStartPosX;
    tools::Long        nTabPos;

    TabInfo()
        : bValid(false)
        , nTabPortion(0)
        , nStartPosX(0)
        , nTabPos(0)
        { }

};

}

Point Rotate( const Point& rPoint, Degree10 nOrientation, const Point& rOrigin )
{
    double nRealOrientation = toRadians(nOrientation);
    double nCos = cos( nRealOrientation );
    double nSin = sin( nRealOrientation );

    Point aRotatedPos;
    Point aTranslatedPos( rPoint );

    // Translation
    aTranslatedPos -= rOrigin;

    // Rotation...
    aRotatedPos.setX( static_cast<tools::Long>( nCos*aTranslatedPos.X() + nSin*aTranslatedPos.Y() ) );
    aRotatedPos.setY( static_cast<tools::Long>(- ( nSin*aTranslatedPos.X() - nCos*aTranslatedPos.Y() )) );
    aTranslatedPos = aRotatedPos;

    // Translation...
    aTranslatedPos += rOrigin;
    return aTranslatedPos;
}

AsianCompressionFlags GetCharTypeForCompression( sal_Unicode cChar )
{
    switch ( cChar )
    {
        case 0x3008: case 0x300A: case 0x300C: case 0x300E:
        case 0x3010: case 0x3014: case 0x3016: case 0x3018:
        case 0x301A: case 0x301D:
        {
            return AsianCompressionFlags::PunctuationRight;
        }
        case 0x3001: case 0x3002: case 0x3009: case 0x300B:
        case 0x300D: case 0x300F: case 0x3011: case 0x3015:
        case 0x3017: case 0x3019: case 0x301B: case 0x301E:
        case 0x301F:
        {
            return AsianCompressionFlags::PunctuationLeft;
        }
        default:
        {
            return ( ( 0x3040 <= cChar ) && ( 0x3100 > cChar ) ) ? AsianCompressionFlags::Kana : AsianCompressionFlags::Normal;
        }
    }
}

static void lcl_DrawRedLines( OutputDevice& rOutDev,
                              tools::Long nFontHeight,
                              const Point& rPoint,
                              size_t nIndex,
                              size_t nMaxEnd,
                              o3tl::span<const sal_Int32> pDXArray,
                              WrongList const * pWrongs,
                              Degree10 nOrientation,
                              const Point& rOrigin,
                              bool bVertical,
                              bool bIsRightToLeft )
{
    // But only if font is not too small...
    tools::Long nHeight = rOutDev.LogicToPixel(Size(0, nFontHeight)).Height();
    if (WRONG_SHOW_MIN >= nHeight)
        return;

    size_t nEnd, nStart = nIndex;
    bool bWrong = pWrongs->NextWrong(nStart, nEnd);

    while (bWrong)
    {
        if (nStart >= nMaxEnd)
            break;

        if (nStart < nIndex)  // Corrected
            nStart = nIndex;

        if (nEnd > nMaxEnd)
            nEnd = nMaxEnd;

        Point aPoint1(rPoint);
        if (bVertical)
        {
            // VCL doesn't know that the text is vertical, and is manipulating
            // the positions a little bit in y direction...
            tools::Long nOnePixel = rOutDev.PixelToLogic(Size(0, 1)).Height();
            tools::Long nCorrect = 2 * nOnePixel;
            aPoint1.AdjustY(-nCorrect);
            aPoint1.AdjustX(-nCorrect);
        }
        if (nStart > nIndex)
        {
            if (!bVertical)
            {
                // since for RTL portions rPoint is on the visual right end of the portion
                // (i.e. at the start of the first RTL char) we need to subtract the offset
                // for RTL portions...
                aPoint1.AdjustX((bIsRightToLeft ? -1 : 1) * pDXArray[nStart - nIndex - 1]);
            }
            else
                aPoint1.AdjustY(pDXArray[nStart - nIndex - 1]);
        }
        Point aPoint2(rPoint);
        assert(nEnd > nIndex && "RedLine: aPnt2?");
        if (!bVertical)
        {
            // since for RTL portions rPoint is on the visual right end of the portion
            // (i.e. at the start of the first RTL char) we need to subtract the offset
            // for RTL portions...
            aPoint2.AdjustX((bIsRightToLeft ? -1 : 1) * pDXArray[nEnd - nIndex - 1]);
        }
        else
        {
            aPoint2.AdjustY(pDXArray[nEnd - nIndex - 1]);
        }

        if (nOrientation)
        {
            aPoint1 = Rotate(aPoint1, nOrientation, rOrigin);
            aPoint2 = Rotate(aPoint2, nOrientation, rOrigin);
        }

        {
            vcl::ScopedAntialiasing a(rOutDev, true);
            rOutDev.DrawWaveLine(aPoint1, aPoint2);
        }

        nStart = nEnd + 1;
        if (nEnd < nMaxEnd)
            bWrong = pWrongs->NextWrong(nStart, nEnd);
        else
            bWrong = false;
    }
}

static Point lcl_ImplCalcRotatedPos( Point rPos, Point rOrigin, double nSin, double nCos )
{
    Point aRotatedPos;
    // Translation...
    Point aTranslatedPos( rPos);
    aTranslatedPos -= rOrigin;

    aRotatedPos.setX( static_cast<tools::Long>( nCos*aTranslatedPos.X() + nSin*aTranslatedPos.Y() ) );
    aRotatedPos.setY( static_cast<tools::Long>(- ( nSin*aTranslatedPos.X() - nCos*aTranslatedPos.Y() )) );
    aTranslatedPos = aRotatedPos;
    // Translation...
    aTranslatedPos += rOrigin;

    return aTranslatedPos;
}

static bool lcl_IsLigature( sal_Unicode cCh, sal_Unicode cNextCh ) // For Kashidas from sw/source/core/text/porlay.txt
{
            // Lam + Alef
    return ( 0x644 == cCh && 0x627 == cNextCh ) ||
            // Beh + Reh
           ( 0x628 == cCh && 0x631 == cNextCh );
}

static bool lcl_ConnectToPrev( sal_Unicode cCh, sal_Unicode cPrevCh )  // For Kashidas from sw/source/core/text/porlay.txt
{
    // Alef, Dal, Thal, Reh, Zain, and Waw do not connect to the left
    bool bRet = 0x627 != cPrevCh && 0x62F != cPrevCh && 0x630 != cPrevCh &&
                0x631 != cPrevCh && 0x632 != cPrevCh && 0x648 != cPrevCh;

    // check for ligatures cPrevChar + cChar
    if ( bRet )
        bRet = ! lcl_IsLigature( cPrevCh, cCh );

    return bRet;
}



void ImpEditEngine::UpdateViews( EditView* pCurView )
{
    if ( !IsUpdateLayout() || IsFormatting() || aInvalidRect.IsEmpty() )
        return;

    DBG_ASSERT( IsFormatted(), "UpdateViews: Doc not formatted!" );

    for (EditView* pView : aEditViews)
    {
        pView->HideCursor();

        tools::Rectangle aClipRect( aInvalidRect );
        tools::Rectangle aVisArea( pView->GetVisArea() );
        aClipRect.Intersection( aVisArea );

        if ( !aClipRect.IsEmpty() )
        {
            // convert to window coordinates...
            aClipRect = pView->pImpEditView->GetWindowPos( aClipRect );

            // moved to one executing method to allow finer control
            pView->InvalidateWindow(aClipRect);

            pView->InvalidateOtherViewWindows( aClipRect );
        }
    }

    if ( pCurView )
    {
        bool bGotoCursor = pCurView->pImpEditView->DoAutoScroll();
        pCurView->ShowCursor( bGotoCursor );
    }

    aInvalidRect = tools::Rectangle();
    CallStatusHdl();
}

IMPL_LINK_NOARG(ImpEditEngine, OnlineSpellHdl, Timer *, void)
{
    if ( !Application::AnyInput( VclInputFlags::KEYBOARD ) && IsUpdateLayout() && IsFormatted() )
        DoOnlineSpelling();
    else
        aOnlineSpellTimer.Start();
}

IMPL_LINK_NOARG(ImpEditEngine, IdleFormatHdl, Timer *, void)
{
    aIdleFormatter.ResetRestarts();

    // #i97146# check if that view is still available
    // else probably the idle format timer fired while we're already
    // downing
    EditView* pView = aIdleFormatter.GetView();
    for (EditView* aEditView : aEditViews)
    {
        if( aEditView == pView )
        {
            FormatAndLayout( pView );
            break;
        }
    }
}

void ImpEditEngine::CheckIdleFormatter()
{
    aIdleFormatter.ForceTimeout();
    // If not idle, but still not formatted:
    if ( !IsFormatted() )
        FormatDoc();
}

bool ImpEditEngine::IsPageOverflow( ) const
{
    return mbNeedsChainingHandling;
}


void ImpEditEngine::FormatFullDoc()
{
    for ( sal_Int32 nPortion = 0; nPortion < GetParaPortions().Count(); nPortion++ )
        GetParaPortions()[nPortion].MarkSelectionInvalid( 0 );
    FormatDoc();
}

void ImpEditEngine::FormatDoc()
{
    if (!IsUpdateLayout() || IsFormatting())
        return;

    bIsFormatting = true;

    // Then I can also start the spell-timer...
    if ( GetStatus().DoOnlineSpelling() )
        StartOnlineSpellTimer();

    tools::Long nY = 0;
    bool bGrow = false;

    // Here already, so that not always in CreateLines...
    bool bMapChanged = ImpCheckRefMapMode();
    std::set<sal_Int32> aRepaintParas;

    for ( sal_Int32 nPara = 0; nPara < GetParaPortions().Count(); nPara++ )
    {
        ParaPortion& rParaPortion = GetParaPortions()[nPara];
        if ( rParaPortion.MustRepaint() || ( rParaPortion.IsInvalid() && rParaPortion.IsVisible() ) )
        {
            // No formatting should be necessary for MustRepaint()!
            if ( !rParaPortion.IsInvalid() || CreateLines( nPara, nY ) )
            {
                if ( !bGrow && GetTextRanger() )
                {
                    // For a change in height all below must be reformatted...
                    for ( sal_Int32 n = nPara+1; n < GetParaPortions().Count(); n++ )
                    {
                        ParaPortion& rPP = GetParaPortions()[n];
                        rPP.MarkSelectionInvalid( 0 );
                        rPP.GetLines().Reset();
                    }
                }
                bGrow = true;
                if ( IsCallParaInsertedOrDeleted() )
                {
                    GetEditEnginePtr()->ParagraphHeightChanged( nPara );

                    for (EditView* pView : aEditViews)
                    {
                        ImpEditView* pImpView = pView->pImpEditView.get();
                        pImpView->ScrollStateChange();
                    }

                }
                rParaPortion.SetMustRepaint( false );
            }

            aRepaintParas.insert(nPara);
        }
        nY += rParaPortion.GetHeight();
    }

    aInvalidRect = tools::Rectangle(); // make empty

    // One can also get into the formatting through UpdateMode ON=>OFF=>ON...
    // enable optimization first after Vobis delivery...
    {
        tools::Long nNewHeightNTP;
        tools::Long nNewHeight = CalcTextHeight(&nNewHeightNTP);
        tools::Long nDiff = nNewHeight - nCurTextHeight;
        if ( nDiff )
        {
            aInvalidRect.Union(tools::Rectangle::Justify(
                { 0, nNewHeight }, { getWidthDirectionAware(aPaperSize), nCurTextHeight }));
            aStatus.GetStatusWord() |= !IsEffectivelyVertical() ? EditStatusFlags::TextHeightChanged : EditStatusFlags::TEXTWIDTHCHANGED;
        }

        nCurTextHeight = nNewHeight;
        nCurTextHeightNTP = nNewHeightNTP;

        if ( aStatus.AutoPageSize() )
            CheckAutoPageSize();
        else if ( nDiff )
        {
            for (EditView* pView : aEditViews)
            {
                ImpEditView* pImpView = pView->pImpEditView.get();
                if ( pImpView->DoAutoHeight() )
                {
                    Size aSz( pImpView->GetOutputArea().GetWidth(), nCurTextHeight );
                    if ( aSz.Height() > aMaxAutoPaperSize.Height() )
                        aSz.setHeight( aMaxAutoPaperSize.Height() );
                    else if ( aSz.Height() < aMinAutoPaperSize.Height() )
                        aSz.setHeight( aMinAutoPaperSize.Height() );
                    pImpView->ResetOutputArea( tools::Rectangle(
                        pImpView->GetOutputArea().TopLeft(), aSz ) );
                }
            }
        }

        if (!aRepaintParas.empty())
        {
            auto CombineRepaintParasAreas = [&](const LineAreaInfo& rInfo) {
                if (aRepaintParas.count(rInfo.nPortion))
                    aInvalidRect.Union(rInfo.aArea);
                return CallbackResult::Continue;
            };
            IterateLineAreas(CombineRepaintParasAreas, IterFlag::inclILS);
        }
    }

    bIsFormatting = false;
    bFormatted = true;

    if ( bMapChanged )
        GetRefDevice()->Pop();

    CallStatusHdl();    // If Modified...
}

bool ImpEditEngine::ImpCheckRefMapMode()
{
    bool bChange = false;

    if ( aStatus.DoFormat100() )
    {
        MapMode aMapMode( GetRefDevice()->GetMapMode() );
        if ( aMapMode.GetScaleX().GetNumerator() != aMapMode.GetScaleX().GetDenominator() )
            bChange = true;
        else if ( aMapMode.GetScaleY().GetNumerator() != aMapMode.GetScaleY().GetDenominator() )
            bChange = true;

        if ( bChange )
        {
            Fraction Scale1( 1, 1 );
            aMapMode.SetScaleX( Scale1 );
            aMapMode.SetScaleY( Scale1 );
            GetRefDevice()->Push();
            GetRefDevice()->SetMapMode( aMapMode );
        }
    }

    return bChange;
}

void ImpEditEngine::CheckAutoPageSize()
{
    Size aPrevPaperSize( GetPaperSize() );
    if ( GetStatus().AutoPageWidth() )
        aPaperSize.setWidth( !IsEffectivelyVertical() ? CalcTextWidth( true ) : GetTextHeight() );
    if ( GetStatus().AutoPageHeight() )
        aPaperSize.setHeight( !IsEffectivelyVertical() ? GetTextHeight() : CalcTextWidth( true ) );

    SetValidPaperSize( aPaperSize );    // consider Min, Max

    if ( aPaperSize == aPrevPaperSize )
        return;

    if ( ( !IsEffectivelyVertical() && ( aPaperSize.Width() != aPrevPaperSize.Width() ) )
         || ( IsEffectivelyVertical() && ( aPaperSize.Height() != aPrevPaperSize.Height() ) ) )
    {
        // If ahead is centered / right or tabs...
        aStatus.GetStatusWord() |= !IsEffectivelyVertical() ? EditStatusFlags::TEXTWIDTHCHANGED : EditStatusFlags::TextHeightChanged;
        for ( sal_Int32 nPara = 0; nPara < GetParaPortions().Count(); nPara++ )
        {
            // Only paragraphs which are not aligned to the left need to be
            // reformatted, the height can not be changed here anymore.
            ParaPortion& rParaPortion = GetParaPortions()[nPara];
            SvxAdjust eJustification = GetJustification( nPara );
            if ( eJustification != SvxAdjust::Left )
            {
                rParaPortion.MarkSelectionInvalid( 0 );
                CreateLines( nPara, 0 );  // 0: For AutoPageSize no TextRange!
            }
        }
    }

    Size aInvSize = aPaperSize;
    if ( aPaperSize.Width() < aPrevPaperSize.Width() )
        aInvSize.setWidth( aPrevPaperSize.Width() );
    if ( aPaperSize.Height() < aPrevPaperSize.Height() )
        aInvSize.setHeight( aPrevPaperSize.Height() );

    Size aSz( aInvSize );
    if ( IsEffectivelyVertical() )
    {
        aSz.setWidth( aInvSize.Height() );
        aSz.setHeight( aInvSize.Width() );
    }
    aInvalidRect = tools::Rectangle( Point(), aSz );


    for (EditView* pView : aEditViews)
    {
        pView->pImpEditView->RecalcOutputArea();
    }
}

void ImpEditEngine::CheckPageOverflow()
{
    SAL_INFO("editeng.chaining", "[CONTROL_STATUS] AutoPageSize is " << (( aStatus.GetControlWord() & EEControlBits::AUTOPAGESIZE ) ? "ON" : "OFF") );

    tools::Long nBoxHeight = GetMaxAutoPaperSize().Height();
    SAL_INFO("editeng.chaining", "[OVERFLOW-CHECK] Current MaxAutoPaperHeight is " << nBoxHeight);

    tools::Long nTxtHeight = CalcTextHeight(nullptr);
    SAL_INFO("editeng.chaining", "[OVERFLOW-CHECK] Current Text Height is " << nTxtHeight);

    sal_uInt32 nParaCount = GetParaPortions().Count();
    sal_uInt32 nFirstLineCount = GetLineCount(0);
    bool bOnlyOneEmptyPara = (nParaCount == 1) &&
                            (nFirstLineCount == 1) &&
                            (GetLineLen(0,0) == 0);

    if (nTxtHeight > nBoxHeight && !bOnlyOneEmptyPara)
    {
        // which paragraph is the first to cause higher size of the box?
        ImplUpdateOverflowingParaNum( nBoxHeight); // XXX: currently only for horizontal text
        //aStatus.SetPageOverflow(true);
        mbNeedsChainingHandling = true;
    } else
    {
        // No overflow if within box boundaries
        //aStatus.SetPageOverflow(false);
        mbNeedsChainingHandling = false;
    }

}

static sal_Int32 ImplCalculateFontIndependentLineSpacing( const sal_Int32 nFontHeight )
{
    return ( nFontHeight * 12 ) / 10;   // + 20%
}

tools::Long ImpEditEngine::GetColumnWidth(const Size& rPaperSize) const
{
    assert(mnColumns >= 1);
    tools::Long nWidth = IsEffectivelyVertical() ? rPaperSize.Height() : rPaperSize.Width();
    return (nWidth - mnColumnSpacing * (mnColumns - 1)) / mnColumns;
}

bool ImpEditEngine::CreateLines( sal_Int32 nPara, sal_uInt32 nStartPosY )
{
    ParaPortion& rParaPortion = GetParaPortions()[nPara];

    // sal_Bool: Changes in the height of paragraph Yes / No - sal_True/sal_False
    assert( rParaPortion.GetNode() && "Portion without Node in CreateLines" );
    DBG_ASSERT( rParaPortion.IsVisible(), "Invisible paragraphs not formatted!" );
    DBG_ASSERT( rParaPortion.IsInvalid(), "CreateLines: Portion not invalid!" );

    bool bProcessingEmptyLine = ( rParaPortion.GetNode()->Len() == 0 );
    bool bEmptyNodeWithPolygon = ( rParaPortion.GetNode()->Len() == 0 ) && GetTextRanger();


    // Fast special treatment for empty paragraphs...

    if ( ( rParaPortion.GetNode()->Len() == 0 ) && !GetTextRanger() )
    {
        // fast special treatment...
        if ( rParaPortion.GetTextPortions().Count() )
            rParaPortion.GetTextPortions().Reset();
        if ( rParaPortion.GetLines().Count() )
            rParaPortion.GetLines().Reset();
        CreateAndInsertEmptyLine( &rParaPortion );
        return FinishCreateLines( &rParaPortion );
    }


    // Initialization...


    // Always format for 100%:
    bool bMapChanged = ImpCheckRefMapMode();

    if ( rParaPortion.GetLines().Count() == 0 )
    {
        EditLine* pL = new EditLine;
        rParaPortion.GetLines().Append(pL);
    }


    // Get Paragraph attributes...

    ContentNode* const pNode = rParaPortion.GetNode();

    bool bRightToLeftPara = IsRightToLeft( nPara );

    SvxAdjust eJustification = GetJustification( nPara );
    bool bHyphenatePara = pNode->GetContentAttribs().GetItem( EE_PARA_HYPHENATE ).GetValue();
    sal_Int32 nSpaceBefore      = 0;
    sal_Int32 nMinLabelWidth    = 0;
    sal_Int32 nSpaceBeforeAndMinLabelWidth = GetSpaceBeforeAndMinLabelWidth( pNode, &nSpaceBefore, &nMinLabelWidth );
    const SvxLRSpaceItem& rLRItem = GetLRSpaceItem( pNode );
    const SvxLineSpacingItem& rLSItem = pNode->GetContentAttribs().GetItem( EE_PARA_SBL );
    const bool bScriptSpace = pNode->GetContentAttribs().GetItem( EE_PARA_ASIANCJKSPACING ).GetValue();

    const short nInvalidDiff = rParaPortion.GetInvalidDiff();
    const sal_Int32 nInvalidStart = rParaPortion.GetInvalidPosStart();
    const sal_Int32 nInvalidEnd =  nInvalidStart + std::abs( nInvalidDiff );

    bool bQuickFormat = false;
    if ( !bEmptyNodeWithPolygon && !HasScriptType( nPara, i18n::ScriptType::COMPLEX ) )
    {
        if ( ( rParaPortion.IsSimpleInvalid() ) && ( nInvalidDiff > 0 ) &&
             ( pNode->GetString().indexOf( CH_FEATURE, nInvalidStart ) > nInvalidEnd ) )
        {
            bQuickFormat = true;
        }
        else if ( ( rParaPortion.IsSimpleInvalid() ) && ( nInvalidDiff < 0 ) )
        {
            // check if delete over the portion boundaries was done...
            sal_Int32 nStart = nInvalidStart;  // DOUBLE !!!!!!!!!!!!!!!
            sal_Int32 nEnd = nStart - nInvalidDiff;  // negative
            bQuickFormat = true;
            sal_Int32 nPos = 0;
            sal_Int32 nPortions = rParaPortion.GetTextPortions().Count();
            for ( sal_Int32 nTP = 0; nTP < nPortions; nTP++ )
            {
                // There must be no start / end in the deleted area.
                const TextPortion& rTP = rParaPortion.GetTextPortions()[ nTP ];
                nPos = nPos + rTP.GetLen();
                if ( ( nPos > nStart ) && ( nPos < nEnd ) )
                {
                    bQuickFormat = false;
                    break;
                }
            }
        }
    }

    // Saving both layout mode and language (since I'm potentially changing both)
    GetRefDevice()->Push( vcl::PushFlags::TEXTLAYOUTMODE|vcl::PushFlags::TEXTLANGUAGE );

    ImplInitLayoutMode(*GetRefDevice(), nPara, -1);

    sal_Int32 nRealInvalidStart = nInvalidStart;

    if ( bEmptyNodeWithPolygon )
    {
        TextPortion* pDummyPortion = new TextPortion( 0 );
        rParaPortion.GetTextPortions().Reset();
        rParaPortion.GetTextPortions().Append(pDummyPortion);
    }
    else if ( bQuickFormat )
    {
        // faster Method:
        RecalcTextPortion( &rParaPortion, nInvalidStart, nInvalidDiff );
    }
    else    // nRealInvalidStart can be before InvalidStart, since Portions were deleted...
    {
        CreateTextPortions( &rParaPortion, nRealInvalidStart );
    }


    // Search for line with InvalidPos, start one line before
    // Flag the line => do not remove it !


    sal_Int32 nLine = rParaPortion.GetLines().Count()-1;
    for ( sal_Int32 nL = 0; nL <= nLine; nL++ )
    {
        EditLine& rLine = rParaPortion.GetLines()[nL];
        if ( rLine.GetEnd() > nRealInvalidStart )  // not nInvalidStart!
        {
            nLine = nL;
            break;
        }
        rLine.SetValid();
    }
    // Begin one line before...
    // If it is typed at the end, the line in front cannot change.
    if ( nLine && ( !rParaPortion.IsSimpleInvalid() || ( nInvalidEnd < pNode->Len() ) || ( nInvalidDiff <= 0 ) ) )
        nLine--;

    EditLine* pLine = &rParaPortion.GetLines()[nLine];

    static const tools::Rectangle aZeroArea { Point(), Point() };
    tools::Rectangle aBulletArea( aZeroArea );
    if ( !nLine )
    {
        aBulletArea = GetEditEnginePtr()->GetBulletArea( GetParaPortions().GetPos( &rParaPortion ) );
        if ( !aBulletArea.IsWidthEmpty() && aBulletArea.Right() > 0 )
            rParaPortion.SetBulletX( static_cast<sal_Int32>(GetXValue( aBulletArea.Right() )) );
        else
            rParaPortion.SetBulletX( 0 ); // if Bullet is set incorrectly
    }


    // Reformat all lines from here...

    sal_Int32 nDelFromLine = -1;
    bool bLineBreak = false;

    sal_Int32 nIndex = pLine->GetStart();
    EditLine aSaveLine( *pLine );
    SvxFont aTmpFont( pNode->GetCharAttribs().GetDefFont() );

    ImplInitLayoutMode(*GetRefDevice(), nPara, nIndex);

    std::vector<sal_Int32> aBuf( pNode->Len() );

    bool bSameLineAgain = false;    // For TextRanger, if the height changes.
    TabInfo aCurrentTab;

    bool bForceOneRun = bEmptyNodeWithPolygon;
    bool bCompressedChars = false;

    while ( ( nIndex < pNode->Len() ) || bForceOneRun )
    {
        bForceOneRun = false;

        bool bEOL = false;
        bool bEOC = false;
        sal_Int32 nPortionStart = 0;
        sal_Int32 nPortionEnd = 0;

        tools::Long nStartX = GetXValue( rLRItem.GetTextLeft() + nSpaceBeforeAndMinLabelWidth );
        if ( nIndex == 0 )
        {
            tools::Long nFI = GetXValue( rLRItem.GetTextFirstLineOffset() );
            nStartX += nFI;

            if ( !nLine && ( rParaPortion.GetBulletX() > nStartX ) )
            {
                    nStartX = rParaPortion.GetBulletX();
            }
        }

        const bool bAutoSize = IsEffectivelyVertical() ? aStatus.AutoPageHeight() : aStatus.AutoPageWidth();
        tools::Long nMaxLineWidth = GetColumnWidth(bAutoSize ? aMaxAutoPaperSize : aPaperSize);

        nMaxLineWidth -= GetXValue( rLRItem.GetRight() );
        nMaxLineWidth -= nStartX;

        // If PaperSize == long_max, one cannot take away any negative
        // first line indent. (Overflow)
        if ( ( nMaxLineWidth < 0 ) && ( nStartX < 0 ) )
            nMaxLineWidth = GetColumnWidth(aPaperSize) - GetXValue(rLRItem.GetRight());

        // If still less than 0, it may be just the right edge.
        if ( nMaxLineWidth <= 0 )
            nMaxLineWidth = 1;

        // Problem:
        // Since formatting starts a line _before_ the invalid position,
     // the positions unfortunately have to be redefined...
        // Solution:
        // The line before can only become longer, not smaller
        // =>...
        pLine->GetCharPosArray().clear();

        sal_Int32 nTmpPos = nIndex;
        sal_Int32 nTmpPortion = pLine->GetStartPortion();
        tools::Long nTmpWidth = 0;
        tools::Long nXWidth = nMaxLineWidth;

        std::deque<tools::Long>* pTextRanges = nullptr;
        tools::Long nTextExtraYOffset = 0;
        tools::Long nTextXOffset = 0;
        tools::Long nTextLineHeight = 0;
        if ( GetTextRanger() )
        {
            GetTextRanger()->SetVertical( IsEffectivelyVertical() );

            tools::Long nTextY = nStartPosY + GetEditCursor( &rParaPortion, pLine, pLine->GetStart(), GetCursorFlags::NONE ).Top();
            if ( !bSameLineAgain )
            {
                SeekCursor( pNode, nTmpPos+1, aTmpFont );
                aTmpFont.SetPhysFont(*GetRefDevice());
                ImplInitDigitMode(*GetRefDevice(), aTmpFont.GetLanguage());

                if ( IsFixedCellHeight() )
                    nTextLineHeight = ImplCalculateFontIndependentLineSpacing( aTmpFont.GetFontHeight() );
                else
                    nTextLineHeight = aTmpFont.GetPhysTxtSize( GetRefDevice() ).Height();
                // Metrics can be greater
                FormatterFontMetric aTempFormatterMetrics;
                RecalcFormatterFontMetrics( aTempFormatterMetrics, aTmpFont );
                sal_uInt16 nLineHeight = aTempFormatterMetrics.GetHeight();
                if ( nLineHeight > nTextLineHeight )
                    nTextLineHeight = nLineHeight;
            }
            else
                nTextLineHeight = pLine->GetHeight();

            nXWidth = 0;
            while ( !nXWidth )
            {
                tools::Long nYOff = nTextY + nTextExtraYOffset;
                tools::Long nYDiff = nTextLineHeight;
                if ( IsEffectivelyVertical() )
                {
                    tools::Long nMaxPolygonX = GetTextRanger()->GetBoundRect().Right();
                    nYOff = nMaxPolygonX-nYOff;
                    nYDiff = -nTextLineHeight;
                }
                pTextRanges = GetTextRanger()->GetTextRanges( Range( nYOff, nYOff + nYDiff ) );
                assert( pTextRanges && "GetTextRanges?!" );
                tools::Long nMaxRangeWidth = 0;
                // Use the widest range...
                // The widest range could be a bit confusing, so normally it
                // is the first one. Best with gaps.
                assert(pTextRanges->size() % 2 == 0 && "textranges are always in pairs");
                if (!pTextRanges->empty())
                {
                    tools::Long nA = pTextRanges->at(0);
                    tools::Long nB = pTextRanges->at(1);
                    DBG_ASSERT( nA <= nB, "TextRange distorted?" );
                    tools::Long nW = nB - nA;
                    if ( nW > nMaxRangeWidth )
                    {
                        nMaxRangeWidth = nW;
                        nTextXOffset = nA;
                    }
                }
                nXWidth = nMaxRangeWidth;
                if ( nXWidth )
                    nMaxLineWidth = nXWidth - nStartX - GetXValue( rLRItem.GetRight() );
                else
                {
                    // Try further down in the polygon.
                    // Below the polygon use the Paper Width.
                    nTextExtraYOffset += std::max( static_cast<tools::Long>(nTextLineHeight / 10), tools::Long(1) );
                    if ( ( nTextY + nTextExtraYOffset  ) > GetTextRanger()->GetBoundRect().Bottom() )
                    {
                        nXWidth = getWidthDirectionAware(GetPaperSize());
                        if ( !nXWidth ) // AutoPaperSize
                            nXWidth = 0x7FFFFFFF;
                    }
                }
            }
        }

        // search for Portion that no longer fits in line...
        TextPortion* pPortion = nullptr;
        sal_Int32 nPortionLen = 0;
        bool bContinueLastPortion = false;
        bool bBrokenLine = false;
        bLineBreak = false;
        const EditCharAttrib* pNextFeature = pNode->GetCharAttribs().FindFeature( pLine->GetStart() );
        while ( ( nTmpWidth < nXWidth ) && !bEOL )
        {
            const sal_Int32 nTextPortions = rParaPortion.GetTextPortions().Count();
            assert(nTextPortions > 0);
            bContinueLastPortion = (nTmpPortion >= nTextPortions);
            if (bContinueLastPortion)
            {
                if (nTmpPos >= pNode->Len())
                    break;  // while

                // Continue with remainder. This only to have *some* valid
                // X-values and not endlessly create new lines until DOOM...
                // Happened in the scenario of tdf#104152 where inserting a
                // paragraph lead to a11y attempting to format the doc to
                // obtain content when notified.
                nTmpPortion = nTextPortions - 1;
                SAL_WARN("editeng","ImpEditEngine::CreateLines - continuation of a broken portion");
            }

            nPortionStart = nTmpPos;
            pPortion = &rParaPortion.GetTextPortions()[nTmpPortion];
            if ( !bContinueLastPortion && pPortion->GetKind() == PortionKind::HYPHENATOR )
            {
                // Throw away a Portion, if necessary correct the one before,
                // if the Hyph portion has swallowed a character...
                sal_Int32 nTmpLen = pPortion->GetLen();
                rParaPortion.GetTextPortions().Remove( nTmpPortion );
                if (nTmpPortion && nTmpLen)
                {
                    nTmpPortion--;
                    TextPortion& rPrev = rParaPortion.GetTextPortions()[nTmpPortion];
                    DBG_ASSERT( rPrev.GetKind() == PortionKind::TEXT, "Portion?!" );
                    nTmpWidth -= rPrev.GetSize().Width();
                    nTmpPos = nTmpPos - rPrev.GetLen();
                    rPrev.SetLen(rPrev.GetLen() + nTmpLen);
                    rPrev.GetSize().setWidth( -1 );
                }

                assert( nTmpPortion < rParaPortion.GetTextPortions().Count() && "No more Portions left!" );
                pPortion = &rParaPortion.GetTextPortions()[nTmpPortion];
            }

            if (bContinueLastPortion)
            {
                // Note that this may point behind the portion and is only to
                // be used with the node's string offsets to generate X-values.
                nPortionLen = pNode->Len() - nPortionStart;
            }
            else
            {
                nPortionLen = pPortion->GetLen();
            }

            DBG_ASSERT( pPortion->GetKind() != PortionKind::HYPHENATOR, "CreateLines: Hyphenator-Portion!" );
            DBG_ASSERT( nPortionLen || bProcessingEmptyLine, "Empty Portion in CreateLines ?!" );
            if ( pNextFeature && ( pNextFeature->GetStart() == nTmpPos ) )
            {
                SAL_WARN_IF( bContinueLastPortion,
                        "editeng","ImpEditEngine::CreateLines - feature in continued portion will be wrong");
                sal_uInt16 nWhich = pNextFeature->GetItem()->Which();
                switch ( nWhich )
                {
                    case EE_FEATURE_TAB:
                    {
                        tools::Long nOldTmpWidth = nTmpWidth;

                        // Search for Tab-Pos...
                        tools::Long nCurPos = nTmpWidth+nStartX;
                        // consider scaling
                        if ( aStatus.DoStretch() && ( nStretchX != 100 ) )
                            nCurPos = nCurPos*100/std::max(static_cast<sal_Int32>(nStretchX), static_cast<sal_Int32>(1));

                        short nAllSpaceBeforeText = static_cast< short >(rLRItem.GetTextLeft()/* + rLRItem.GetTextLeft()*/ + nSpaceBeforeAndMinLabelWidth);
                        aCurrentTab.aTabStop = pNode->GetContentAttribs().FindTabStop( nCurPos - nAllSpaceBeforeText /*rLRItem.GetTextLeft()*/, aEditDoc.GetDefTab() );
                        aCurrentTab.nTabPos = GetXValue( static_cast<tools::Long>( aCurrentTab.aTabStop.GetTabPos() + nAllSpaceBeforeText /*rLRItem.GetTextLeft()*/ ) );
                        aCurrentTab.bValid = false;

                        // Switch direction in R2L para...
                        if ( bRightToLeftPara )
                        {
                            if ( aCurrentTab.aTabStop.GetAdjustment() == SvxTabAdjust::Right )
                                aCurrentTab.aTabStop.GetAdjustment() = SvxTabAdjust::Left;
                            else if ( aCurrentTab.aTabStop.GetAdjustment() == SvxTabAdjust::Left )
                                aCurrentTab.aTabStop.GetAdjustment() = SvxTabAdjust::Right;
                        }

                        if ( ( aCurrentTab.aTabStop.GetAdjustment() == SvxTabAdjust::Right ) ||
                             ( aCurrentTab.aTabStop.GetAdjustment() == SvxTabAdjust::Center ) ||
                             ( aCurrentTab.aTabStop.GetAdjustment() == SvxTabAdjust::Decimal ) )
                        {
                            // For LEFT / DEFAULT this tab is not considered.
                            aCurrentTab.bValid = true;
                            aCurrentTab.nStartPosX = nTmpWidth;
                            aCurrentTab.nTabPortion = nTmpPortion;
                        }

                        pPortion->SetKind(PortionKind::TAB);
                        pPortion->SetExtraValue( aCurrentTab.aTabStop.GetFill() );
                        pPortion->GetSize().setWidth( aCurrentTab.nTabPos - (nTmpWidth+nStartX) );

                        // Height needed...
                        SeekCursor( pNode, nTmpPos+1, aTmpFont );
                        pPortion->GetSize().setHeight( aTmpFont.QuickGetTextSize( GetRefDevice(), OUString(), 0, 0 ).Height() );

                        DBG_ASSERT( pPortion->GetSize().Width() >= 0, "Tab incorrectly calculated!" );

                        nTmpWidth = aCurrentTab.nTabPos-nStartX;

                        // If this is the first token on the line,
                        // and nTmpWidth > aPaperSize.Width, => infinite loop!
                        if ( ( nTmpWidth >= nXWidth ) && ( nTmpPortion == pLine->GetStartPortion() ) )
                        {
                            // What now?
                            // make the tab fitting
                            pPortion->GetSize().setWidth( nXWidth-nOldTmpWidth );
                            nTmpWidth = nXWidth-1;
                            bEOL = true;
                            bBrokenLine = true;
                        }
                        EditLine::CharPosArrayType& rArray = pLine->GetCharPosArray();
                        size_t nPos = nTmpPos - pLine->GetStart();
                        rArray.insert(rArray.begin()+nPos, pPortion->GetSize().Width());
                        bCompressedChars = false;
                    }
                    break;
                    case EE_FEATURE_LINEBR:
                    {
                        assert( pPortion );
                        pPortion->GetSize().setWidth( 0 );
                        bEOL = true;
                        bLineBreak = true;
                        pPortion->SetKind( PortionKind::LINEBREAK );
                        bCompressedChars = false;
                        EditLine::CharPosArrayType& rArray = pLine->GetCharPosArray();
                        size_t nPos = nTmpPos - pLine->GetStart();
                        rArray.insert(rArray.begin()+nPos, pPortion->GetSize().Width());
                    }
                    break;
                    case EE_FEATURE_FIELD:
                    {
                        SeekCursor( pNode, nTmpPos+1, aTmpFont );
                        aTmpFont.SetPhysFont(*GetRefDevice());
                        ImplInitDigitMode(*GetRefDevice(), aTmpFont.GetLanguage());

                        OUString aFieldValue = static_cast<const EditCharAttribField*>(pNextFeature)->GetFieldValue();
                        // get size, but also DXArray to allow length information in line breaking below
                        std::vector<sal_Int32> aTmpDXArray;
                        pPortion->GetSize() = aTmpFont.QuickGetTextSize(GetRefDevice(), aFieldValue, 0, aFieldValue.getLength(), &aTmpDXArray);

                        // So no scrolling for oversized fields
                        if ( pPortion->GetSize().Width() > nXWidth )
                        {
                            // create ExtraPortionInfo on-demand, flush lineBreaksList
                            ExtraPortionInfo *pExtraInfo = pPortion->GetExtraInfos();

                            if(nullptr == pExtraInfo)
                            {
                                pExtraInfo = new ExtraPortionInfo();
                                pExtraInfo->nOrgWidth = nXWidth;
                                pPortion->SetExtraInfos(pExtraInfo);
                            }
                            else
                            {
                                pExtraInfo->lineBreaksList.clear();
                            }

                            // iterate over CellBreaks using XBreakIterator to be on the
                            // safe side with international texts/charSets
                            Reference < i18n::XBreakIterator > xBreakIterator(ImplGetBreakIterator());
                            const sal_Int32 nTextLength(aFieldValue.getLength());
                            const lang::Locale aLocale(GetLocale(EditPaM(pNode, nPortionStart)));
                            sal_Int32 nDone(0);
                            sal_Int32 nNextCellBreak(
                                xBreakIterator->nextCharacters(
                                        aFieldValue,
                                        0,
                                        aLocale,
                                        css::i18n::CharacterIteratorMode::SKIPCELL,
                                        0,
                                        nDone));
                            sal_Int32 nLastCellBreak(0);
                            sal_Int32 nLineStartX(0);

                            // always add 1st line break (safe, we already know we are larger than nXWidth)
                            pExtraInfo->lineBreaksList.push_back(0);

                            for(sal_Int32 a(0); a < nTextLength; a++)
                            {
                                if(a == nNextCellBreak)
                                {
                                    // check width
                                    if(aTmpDXArray[a] - nLineStartX > nXWidth)
                                    {
                                        // new CellBreak does not fit in current line, need to
                                        // create a break at LastCellBreak - but do not add 1st
                                        // line break twice for very tall frames
                                        if(0 != a)
                                        {
                                            pExtraInfo->lineBreaksList.push_back(a);
                                        }

                                        // moveLineStart forward in X
                                        nLineStartX = aTmpDXArray[nLastCellBreak];
                                    }

                                    // update CellBreak iteration values
                                    nLastCellBreak = a;
                                    nNextCellBreak = xBreakIterator->nextCharacters(
                                        aFieldValue,
                                        a,
                                        aLocale,
                                        css::i18n::CharacterIteratorMode::SKIPCELL,
                                        1,
                                        nDone);
                                }
                            }
                        }
                        nTmpWidth += pPortion->GetSize().Width();
                        EditLine::CharPosArrayType& rArray = pLine->GetCharPosArray();
                        size_t nPos = nTmpPos - pLine->GetStart();
                        rArray.insert(rArray.begin()+nPos, pPortion->GetSize().Width());
                        pPortion->SetKind(PortionKind::FIELD);
                        // If this is the first token on the line,
                        // and nTmpWidth > aPaperSize.Width, => infinite loop!
                        if ( ( nTmpWidth >= nXWidth ) && ( nTmpPortion == pLine->GetStartPortion() ) )
                        {
                            nTmpWidth = nXWidth-1;
                            bEOL = true;
                            bBrokenLine = true;
                        }
                        // Compression in Fields????
                        // I think this could be a little bit difficult and is not very useful
                        bCompressedChars = false;
                    }
                    break;
                    default:    OSL_FAIL( "What feature?" );
                }
                pNextFeature = pNode->GetCharAttribs().FindFeature( pNextFeature->GetStart() + 1  );
            }
            else
            {
                DBG_ASSERT( nPortionLen || bProcessingEmptyLine, "Empty Portion - Extra Space?!" );
                SeekCursor( pNode, nTmpPos+1, aTmpFont );
                aTmpFont.SetPhysFont(*GetRefDevice());
                ImplInitDigitMode(*GetRefDevice(), aTmpFont.GetLanguage());

                if (!bContinueLastPortion)
                    pPortion->SetRightToLeftLevel( GetRightToLeft( nPara, nTmpPos+1 ) );

                if (bContinueLastPortion)
                {
                     Size aSize( aTmpFont.QuickGetTextSize( GetRefDevice(),
                            rParaPortion.GetNode()->GetString(), nTmpPos, nPortionLen, &aBuf ));
                     pPortion->GetSize().AdjustWidth(aSize.Width() );
                     if (pPortion->GetSize().Height() < aSize.Height())
                         pPortion->GetSize().setHeight( aSize.Height() );
                }
                else
                {
                    pPortion->GetSize() = aTmpFont.QuickGetTextSize( GetRefDevice(),
                            rParaPortion.GetNode()->GetString(), nTmpPos, nPortionLen, &aBuf );
                }

                // #i9050# Do Kerning also behind portions...
                if ( ( aTmpFont.GetFixKerning() > 0 ) && ( ( nTmpPos + nPortionLen ) < pNode->Len() ) )
                    pPortion->GetSize().AdjustWidth(aTmpFont.GetFixKerning() );
                if ( IsFixedCellHeight() )
                    pPortion->GetSize().setHeight( ImplCalculateFontIndependentLineSpacing( aTmpFont.GetFontHeight() ) );
                // The array is  generally flattened at the beginning
                // => Always simply quick inserts.
                size_t nPos = nTmpPos - pLine->GetStart();
                EditLine::CharPosArrayType& rArray = pLine->GetCharPosArray();
                rArray.insert( rArray.begin() + nPos, aBuf.data(), aBuf.data() + nPortionLen);

                // And now check for Compression:
                if ( !bContinueLastPortion && nPortionLen && GetAsianCompressionMode() != CharCompressType::NONE )
                {
                    sal_Int32* pDXArray = rArray.data() + nTmpPos - pLine->GetStart();
                    bCompressedChars |= ImplCalcAsianCompression(
                        pNode, pPortion, nTmpPos, pDXArray, 10000, false);
                }

                nTmpWidth += pPortion->GetSize().Width();

                sal_Int32 _nPortionEnd = nTmpPos + nPortionLen;
                if( bScriptSpace && ( _nPortionEnd < pNode->Len() ) && ( nTmpWidth < nXWidth ) && IsScriptChange( EditPaM( pNode, _nPortionEnd ) ) )
                {
                    bool bAllow = false;
                    sal_uInt16 nScriptTypeLeft = GetI18NScriptType( EditPaM( pNode, _nPortionEnd ) );
                    sal_uInt16 nScriptTypeRight = GetI18NScriptType( EditPaM( pNode, _nPortionEnd+1 ) );
                    if ( ( nScriptTypeLeft == i18n::ScriptType::ASIAN ) || ( nScriptTypeRight == i18n::ScriptType::ASIAN ) )
                        bAllow = true;

                    // No spacing within L2R/R2L nesting
                    if ( bAllow )
                    {
                        tools::Long nExtraSpace = pPortion->GetSize().Height()/5;
                        nExtraSpace = GetXValue( nExtraSpace );
                        pPortion->GetSize().AdjustWidth(nExtraSpace );
                        nTmpWidth += nExtraSpace;
                    }
                }
            }

            if ( aCurrentTab.bValid && ( nTmpPortion != aCurrentTab.nTabPortion ) )
            {
                tools::Long nWidthAfterTab = 0;
                for ( sal_Int32 n = aCurrentTab.nTabPortion+1; n <= nTmpPortion; n++  )
                {
                    const TextPortion& rTP = rParaPortion.GetTextPortions()[n];
                    nWidthAfterTab += rTP.GetSize().Width();
                }
                tools::Long nW = nWidthAfterTab;   // Length before tab position
                if ( aCurrentTab.aTabStop.GetAdjustment() == SvxTabAdjust::Right )
                {
                }
                else if ( aCurrentTab.aTabStop.GetAdjustment() == SvxTabAdjust::Center )
                {
                    nW = nWidthAfterTab/2;
                }
                else if ( aCurrentTab.aTabStop.GetAdjustment() == SvxTabAdjust::Decimal )
                {
                    OUString aText = GetSelected( EditSelection(  EditPaM( rParaPortion.GetNode(), nTmpPos ),
                                                                EditPaM( rParaPortion.GetNode(), nTmpPos + nPortionLen ) ) );
                    sal_Int32 nDecPos = aText.indexOf( aCurrentTab.aTabStop.GetDecimal() );
                    if ( nDecPos != -1 )
                    {
                        nW -= rParaPortion.GetTextPortions()[nTmpPortion].GetSize().Width();
                        nW += aTmpFont.QuickGetTextSize( GetRefDevice(), rParaPortion.GetNode()->GetString(), nTmpPos, nDecPos ).Width();
                        aCurrentTab.bValid = false;
                    }
                }
                else
                {
                    OSL_FAIL( "CreateLines: Tab not handled!" );
                }
                tools::Long nMaxW = aCurrentTab.nTabPos - aCurrentTab.nStartPosX - nStartX;
                if ( nW >= nMaxW )
                {
                    nW = nMaxW;
                    aCurrentTab.bValid = false;
                }
                TextPortion& rTabPortion = rParaPortion.GetTextPortions()[aCurrentTab.nTabPortion];
                rTabPortion.GetSize().setWidth( aCurrentTab.nTabPos - aCurrentTab.nStartPosX - nW - nStartX );
                nTmpWidth = aCurrentTab.nStartPosX + rTabPortion.GetSize().Width() + nWidthAfterTab;
            }

            nTmpPos = nTmpPos + nPortionLen;
            nPortionEnd = nTmpPos;
            nTmpPortion++;
            if ( aStatus.OneCharPerLine() )
                bEOL = true;
        }

        DBG_ASSERT( pPortion, "no portion!?" );

        aCurrentTab.bValid = false;

        assert(pLine);

        // this was possibly a portion too far:
        bool bFixedEnd = false;
        if ( aStatus.OneCharPerLine() )
        {
            // State before Portion (apart from nTmpWidth):
            nTmpPos -= pPortion ? nPortionLen : 0;
            nPortionStart = nTmpPos;
            nTmpPortion--;

            bEOL = true;
            bEOC = false;

            // And now just one character:
            nTmpPos++;
            nTmpPortion++;
            nPortionEnd = nTmpPortion;
            // one Non-Feature-Portion has to be wrapped
            if ( pPortion && nPortionLen > 1 )
            {
                DBG_ASSERT( pPortion->GetKind() == PortionKind::TEXT, "Len>1, but no TextPortion?" );
                nTmpWidth -= pPortion->GetSize().Width();
                sal_Int32 nP = SplitTextPortion( &rParaPortion, nTmpPos, pLine );
                nTmpWidth += rParaPortion.GetTextPortions()[nP].GetSize().Width();
            }
        }
        else if ( nTmpWidth >= nXWidth )
        {
            nPortionEnd = nTmpPos;
            nTmpPos -= pPortion ? nPortionLen : 0;
            nPortionStart = nTmpPos;
            nTmpPortion--;
            bEOL = false;
            bEOC = false;
            if( pPortion ) switch ( pPortion->GetKind() )
            {
                case PortionKind::TEXT:
                {
                    nTmpWidth -= pPortion->GetSize().Width();
                }
                break;
                case PortionKind::FIELD:
                case PortionKind::TAB:
                {
                    nTmpWidth -= pPortion->GetSize().Width();
                    bEOL = true;
                    bFixedEnd = true;
                }
                break;
                default:
                {
                    //  A feature is not wrapped:
                    DBG_ASSERT( ( pPortion->GetKind() == PortionKind::LINEBREAK ), "What Feature ?" );
                    bEOL = true;
                    bFixedEnd = true;
                }
            }
        }
        else
        {
            bEOL = true;
            bEOC = true;
            pLine->SetEnd( nPortionEnd );
            assert( rParaPortion.GetTextPortions().Count() && "No TextPortions?" );
            pLine->SetEndPortion( rParaPortion.GetTextPortions().Count() - 1 );
        }

        if ( aStatus.OneCharPerLine() )
        {
            pLine->SetEnd( nPortionEnd );
            pLine->SetEndPortion( nTmpPortion-1 );
        }
        else if ( bFixedEnd )
        {
            pLine->SetEnd( nPortionStart );
            pLine->SetEndPortion( nTmpPortion-1 );
        }
        else if ( bLineBreak || bBrokenLine )
        {
            pLine->SetEnd( nPortionStart+1 );
            pLine->SetEndPortion( nTmpPortion-1 );
            bEOC = false; // was set above, maybe change the sequence of the if's?
        }
        else if ( !bEOL && !bContinueLastPortion )
        {
            DBG_ASSERT( pPortion && ((nPortionEnd-nPortionStart) == pPortion->GetLen()), "However, another portion?!" );
            tools::Long nRemainingWidth = nMaxLineWidth - nTmpWidth;
            bool bCanHyphenate = ( aTmpFont.GetCharSet() != RTL_TEXTENCODING_SYMBOL );
            if ( bCompressedChars && pPortion && ( pPortion->GetLen() > 1 ) && pPortion->GetExtraInfos() && pPortion->GetExtraInfos()->bCompressed )
            {
                // I need the manipulated DXArray for determining the break position...
                sal_Int32* pDXArray = pLine->GetCharPosArray().data() + (nPortionStart - pLine->GetStart());
                ImplCalcAsianCompression(
                    pNode, pPortion, nPortionStart, pDXArray, 10000, true);
            }
            if( pPortion )
                ImpBreakLine( &rParaPortion, pLine, pPortion, nPortionStart,
                                                nRemainingWidth, bCanHyphenate && bHyphenatePara );
        }


        // Line finished => adjust


        // CalcTextSize should be replaced by a continuous registering!
        Size aTextSize = pLine->CalcTextSize( rParaPortion );

        if ( aTextSize.Height() == 0 )
        {
            SeekCursor( pNode, pLine->GetStart()+1, aTmpFont );
            aTmpFont.SetPhysFont(*pRefDev);
            ImplInitDigitMode(*pRefDev, aTmpFont.GetLanguage());

            if ( IsFixedCellHeight() )
                aTextSize.setHeight( ImplCalculateFontIndependentLineSpacing( aTmpFont.GetFontHeight() ) );
            else
                aTextSize.setHeight( aTmpFont.GetPhysTxtSize( pRefDev ).Height() );
            pLine->SetHeight( static_cast<sal_uInt16>(aTextSize.Height()) );
        }

        // The font metrics can not be calculated continuously, if the font is
        // set anyway, because a large font only after wrapping suddenly ends
        // up in the next line => Font metrics too big.
        FormatterFontMetric aFormatterMetrics;
        sal_Int32 nTPos = pLine->GetStart();
        for ( sal_Int32 nP = pLine->GetStartPortion(); nP <= pLine->GetEndPortion(); nP++ )
        {
            const TextPortion& rTP = rParaPortion.GetTextPortions()[nP];
            // problem with hard font height attribute, when everything but the line break has this attribute
            if ( rTP.GetKind() != PortionKind::LINEBREAK )
            {
                SeekCursor( pNode, nTPos+1, aTmpFont );
                aTmpFont.SetPhysFont(*GetRefDevice());
                ImplInitDigitMode(*GetRefDevice(), aTmpFont.GetLanguage());
                RecalcFormatterFontMetrics( aFormatterMetrics, aTmpFont );
            }
            nTPos = nTPos + rTP.GetLen();
        }
        sal_uInt16 nLineHeight = aFormatterMetrics.GetHeight();
        if ( nLineHeight > pLine->GetHeight() )
            pLine->SetHeight( nLineHeight );
        pLine->SetMaxAscent( aFormatterMetrics.nMaxAscent );

        bSameLineAgain = false;
        if ( GetTextRanger() && ( pLine->GetHeight() > nTextLineHeight ) )
        {
            // put down with the other size!
            bSameLineAgain = true;
        }


        if ( !bSameLineAgain && !aStatus.IsOutliner() )
        {
            if ( rLSItem.GetLineSpaceRule() == SvxLineSpaceRule::Min )
            {
                sal_uInt16 nMinHeight = GetYValue( rLSItem.GetLineHeight() );
                sal_uInt16 nTxtHeight = pLine->GetHeight();
                if ( nTxtHeight < nMinHeight )
                {
                    // The Ascent has to be adjusted for the difference:
                    tools::Long nDiff = nMinHeight - nTxtHeight;
                    pLine->SetMaxAscent( static_cast<sal_uInt16>(pLine->GetMaxAscent() + nDiff) );
                    pLine->SetHeight( nMinHeight, nTxtHeight );
                }
            }
            else if ( rLSItem.GetLineSpaceRule() == SvxLineSpaceRule::Fix )
            {
                sal_uInt16 nFixHeight = GetYValue( rLSItem.GetLineHeight() );
                sal_uInt16 nTxtHeight = pLine->GetHeight();
                pLine->SetMaxAscent( static_cast<sal_uInt16>(pLine->GetMaxAscent() + ( nFixHeight - nTxtHeight ) ) );
                pLine->SetHeight( nFixHeight, nTxtHeight );
            }
            else if ( rLSItem.GetInterLineSpaceRule() == SvxInterLineSpaceRule::Prop )
            {
                // There are documents with PropLineSpace 0, why?
                // (cmc: re above question :-) such documents can be seen by importing a .ppt
                if ( rLSItem.GetPropLineSpace() && ( rLSItem.GetPropLineSpace() < 100 ) )
                {
                    // Adapted code from sw/source/core/text/itrform2.cxx
                    sal_uInt16 nPropLineSpace = rLSItem.GetPropLineSpace();
                    sal_uInt16 nAscent = pLine->GetMaxAscent();
                    sal_uInt16 nNewAscent = pLine->GetTxtHeight() * nPropLineSpace / 100 * 4 / 5; // 80%
                    if ( !nAscent || nAscent > nNewAscent )
                    {
                        pLine->SetMaxAscent( nNewAscent );
                    }
                    sal_uInt16 nHeight = pLine->GetHeight() * nPropLineSpace / 100;
                    pLine->SetHeight( nHeight, pLine->GetTxtHeight() );
                }
                else if ( rLSItem.GetPropLineSpace() && ( rLSItem.GetPropLineSpace() != 100 ) )
                {
                    sal_uInt16 nTxtHeight = pLine->GetHeight();
                    sal_Int32 nPropTextHeight = nTxtHeight * rLSItem.GetPropLineSpace() / 100;
                    // The Ascent has to be adjusted for the difference:
                    tools::Long nDiff = pLine->GetHeight() - nPropTextHeight;
                    pLine->SetMaxAscent( static_cast<sal_uInt16>( pLine->GetMaxAscent() - nDiff ) );
                    pLine->SetHeight( static_cast<sal_uInt16>( nPropTextHeight ), nTxtHeight );
                }
            }
        }

        if ( ( !IsEffectivelyVertical() && aStatus.AutoPageWidth() ) ||
             ( IsEffectivelyVertical() && aStatus.AutoPageHeight() ) )
        {
            // If the row fits within the current paper width, then this width
            // has to be used for the Alignment. If it does not fit or if it
            // will change the paper width, it will be formatted again for
            // Justification! = LEFT anyway.
            tools::Long nMaxLineWidthFix = GetColumnWidth(aPaperSize)
                                        - GetXValue( rLRItem.GetRight() ) - nStartX;
            if ( aTextSize.Width() < nMaxLineWidthFix )
                nMaxLineWidth = nMaxLineWidthFix;
        }

        if ( bCompressedChars )
        {
            tools::Long nRemainingWidth = nMaxLineWidth - aTextSize.Width();
            if ( nRemainingWidth > 0 )
            {
                ImplExpandCompressedPortions( pLine, &rParaPortion, nRemainingWidth );
                aTextSize = pLine->CalcTextSize( rParaPortion );
            }
        }

        if ( pLine->IsHangingPunctuation() )
        {
            // Width from HangingPunctuation was set to 0 in ImpBreakLine,
            // check for rel width now, maybe create compression...
            tools::Long n = nMaxLineWidth - aTextSize.Width();
            TextPortion& rTP = rParaPortion.GetTextPortions()[pLine->GetEndPortion()];
            sal_Int32 nPosInArray = pLine->GetEnd()-1-pLine->GetStart();
            tools::Long nNewValue = ( nPosInArray ? pLine->GetCharPosArray()[ nPosInArray-1 ] : 0 ) + n;
            if (o3tl::make_unsigned(nPosInArray) < pLine->GetCharPosArray().size())
            {
                pLine->GetCharPosArray()[ nPosInArray ] = nNewValue;
            }
            rTP.GetSize().AdjustWidth(n );
        }

        pLine->SetTextWidth( aTextSize.Width() );
        switch ( eJustification )
        {
            case SvxAdjust::Center:
            {
                tools::Long n = ( nMaxLineWidth - aTextSize.Width() ) / 2;
                n += nStartX;  // Indentation is kept.
                pLine->SetStartPosX( n );
            }
            break;
            case SvxAdjust::Right:
            {
                // For automatically wrapped lines, which has a blank at the end
                // the blank must not be displayed!
                tools::Long n = nMaxLineWidth - aTextSize.Width();
                n += nStartX;  // Indentation is kept.
                pLine->SetStartPosX( n );
            }
            break;
            case SvxAdjust::Block:
            {
                bool bDistLastLine = (GetJustifyMethod(nPara) == SvxCellJustifyMethod::Distribute);
                tools::Long nRemainingSpace = nMaxLineWidth - aTextSize.Width();
                pLine->SetStartPosX( nStartX );
                if ( nRemainingSpace > 0 && (!bEOC || bDistLastLine) )
                    ImpAdjustBlocks( &rParaPortion, pLine, nRemainingSpace );
            }
            break;
            default:
            {
                pLine->SetStartPosX( nStartX ); // FI, LI
            }
            break;
        }


        // Check whether the line must be re-issued...

        pLine->SetInvalid();

        // If a portion was wrapped there may be far too many positions in
        // CharPosArray:
        EditLine::CharPosArrayType& rArray = pLine->GetCharPosArray();
        size_t nLen = pLine->GetLen();
        if (rArray.size() > nLen)
            rArray.erase(rArray.begin()+nLen, rArray.end());

        if ( GetTextRanger() )
        {
            if ( nTextXOffset )
                pLine->SetStartPosX( pLine->GetStartPosX() + nTextXOffset );
            if ( nTextExtraYOffset )
            {
                pLine->SetHeight( static_cast<sal_uInt16>( pLine->GetHeight() + nTextExtraYOffset ), 0 );
                pLine->SetMaxAscent( static_cast<sal_uInt16>( pLine->GetMaxAscent() + nTextExtraYOffset ) );
            }
        }

        // for <0 think over !
        if ( rParaPortion.IsSimpleInvalid() )
        {
            // Change through simple Text changes...
            // Do not cancel formatting since Portions possibly have to be split
            // again! If at some point cancelable, then validate the following
            // line! But if applicable, mark as valid, so there is less output...
            if ( pLine->GetEnd() < nInvalidStart )
            {
                if ( *pLine == aSaveLine )
                {
                    pLine->SetValid();
                }
            }
            else
            {
                sal_Int32 nStart = pLine->GetStart();
                sal_Int32 nEnd = pLine->GetEnd();

                if ( nStart > nInvalidEnd )
                {
                    if ( ( ( nStart-nInvalidDiff ) == aSaveLine.GetStart() ) &&
                            ( ( nEnd-nInvalidDiff ) == aSaveLine.GetEnd() ) )
                    {
                        pLine->SetValid();
                        if (bQuickFormat)
                        {
                            bLineBreak = false;
                            rParaPortion.CorrectValuesBehindLastFormattedLine( nLine );
                            break;
                        }
                    }
                }
                else if (bQuickFormat && (nEnd > nInvalidEnd))
                {
                    // If the invalid line ends so that the next begins on the
                    // 'same' passage as before, i.e. not wrapped differently,
                    //  then the text width does not have to be determined anew:
                    if ( nEnd == ( aSaveLine.GetEnd() + nInvalidDiff ) )
                    {
                        bLineBreak = false;
                        rParaPortion.CorrectValuesBehindLastFormattedLine( nLine );
                        break;
                    }
                }
            }
        }

        if ( !bSameLineAgain )
        {
            nIndex = pLine->GetEnd();   // next line start = last line end
                                        // as nEnd points to the last character!

            sal_Int32 nEndPortion = pLine->GetEndPortion();

            // Next line or maybe a new line...
            pLine = nullptr;
            if ( nLine < rParaPortion.GetLines().Count()-1 )
                pLine = &rParaPortion.GetLines()[++nLine];
            if ( pLine && ( nIndex >= pNode->Len() ) )
            {
                nDelFromLine = nLine;
                break;
            }
            if ( !pLine )
            {
                if ( nIndex < pNode->Len() )
                {
                    pLine = new EditLine;
                    rParaPortion.GetLines().Insert(++nLine, pLine);
                }
                else if ( nIndex && bLineBreak && GetTextRanger() )
                {
                    // normally CreateAndInsertEmptyLine would be called, but I want to use
                    // CreateLines, so I need Polygon code only here...
                    TextPortion* pDummyPortion = new TextPortion( 0 );
                    rParaPortion.GetTextPortions().Append(pDummyPortion);
                    pLine = new EditLine;
                    rParaPortion.GetLines().Insert(++nLine, pLine);
                    bForceOneRun = true;
                    bProcessingEmptyLine = true;
                }
            }
            if ( pLine )
            {
                aSaveLine = *pLine;
                pLine->SetStart( nIndex );
                pLine->SetEnd( nIndex );
                pLine->SetStartPortion( nEndPortion+1 );
                pLine->SetEndPortion( nEndPortion+1 );
            }
        }
    }   // while ( Index < Len )

    if ( nDelFromLine >= 0 )
        rParaPortion.GetLines().DeleteFromLine( nDelFromLine );

    DBG_ASSERT( rParaPortion.GetLines().Count(), "No line after CreateLines!" );

    if ( bLineBreak )
        CreateAndInsertEmptyLine( &rParaPortion );

    bool bHeightChanged = FinishCreateLines( &rParaPortion );

    if ( bMapChanged )
        GetRefDevice()->Pop();

    GetRefDevice()->Pop();

    return bHeightChanged;
}

void ImpEditEngine::CreateAndInsertEmptyLine( ParaPortion* pParaPortion )
{
    DBG_ASSERT( !GetTextRanger(), "Don't use CreateAndInsertEmptyLine with a polygon!" );

    EditLine* pTmpLine = new EditLine;
    pTmpLine->SetStart( pParaPortion->GetNode()->Len() );
    pTmpLine->SetEnd( pParaPortion->GetNode()->Len() );
    pParaPortion->GetLines().Append(pTmpLine);

    bool bLineBreak = pParaPortion->GetNode()->Len() > 0;
    sal_Int32 nSpaceBefore = 0;
    sal_Int32 nSpaceBeforeAndMinLabelWidth = GetSpaceBeforeAndMinLabelWidth( pParaPortion->GetNode(), &nSpaceBefore );
    const SvxLRSpaceItem& rLRItem = GetLRSpaceItem( pParaPortion->GetNode() );
    const SvxLineSpacingItem& rLSItem = pParaPortion->GetNode()->GetContentAttribs().GetItem( EE_PARA_SBL );
    tools::Long nStartX = GetXValue( rLRItem.GetTextLeft() + rLRItem.GetTextFirstLineOffset() + nSpaceBefore );

    tools::Rectangle aBulletArea { Point(), Point() };
    if ( bLineBreak )
    {
        nStartX = GetXValue( rLRItem.GetTextLeft() + rLRItem.GetTextFirstLineOffset() + nSpaceBeforeAndMinLabelWidth );
    }
    else
    {
        aBulletArea = GetEditEnginePtr()->GetBulletArea( GetParaPortions().GetPos( pParaPortion ) );
        if ( !aBulletArea.IsEmpty() && aBulletArea.Right() > 0 )
            pParaPortion->SetBulletX( static_cast<sal_Int32>(GetXValue( aBulletArea.Right() )) );
        else
            pParaPortion->SetBulletX( 0 ); // If Bullet set incorrectly.
        if ( pParaPortion->GetBulletX() > nStartX )
        {
            nStartX = GetXValue( rLRItem.GetTextLeft() + rLRItem.GetTextFirstLineOffset() + nSpaceBeforeAndMinLabelWidth );
            if ( pParaPortion->GetBulletX() > nStartX )
                nStartX = pParaPortion->GetBulletX();
        }
    }

    SvxFont aTmpFont;
    SeekCursor( pParaPortion->GetNode(), bLineBreak ? pParaPortion->GetNode()->Len() : 0, aTmpFont );
    aTmpFont.SetPhysFont(*pRefDev);

    TextPortion* pDummyPortion = new TextPortion( 0 );
    pDummyPortion->GetSize() = aTmpFont.GetPhysTxtSize( pRefDev );
    if ( IsFixedCellHeight() )
        pDummyPortion->GetSize().setHeight( ImplCalculateFontIndependentLineSpacing( aTmpFont.GetFontHeight() ) );
    pParaPortion->GetTextPortions().Append(pDummyPortion);
    FormatterFontMetric aFormatterMetrics;
    RecalcFormatterFontMetrics( aFormatterMetrics, aTmpFont );
    pTmpLine->SetMaxAscent( aFormatterMetrics.nMaxAscent );
    pTmpLine->SetHeight( static_cast<sal_uInt16>(pDummyPortion->GetSize().Height()) );
    sal_uInt16 nLineHeight = aFormatterMetrics.GetHeight();
    if ( nLineHeight > pTmpLine->GetHeight() )
        pTmpLine->SetHeight( nLineHeight );

    if ( !aStatus.IsOutliner() )
    {
        sal_Int32 nPara = GetParaPortions().GetPos( pParaPortion );
        SvxAdjust eJustification = GetJustification( nPara );
        tools::Long nMaxLineWidth = GetColumnWidth(aPaperSize);
        nMaxLineWidth -= GetXValue( rLRItem.GetRight() );
        if ( nMaxLineWidth < 0 )
            nMaxLineWidth = 1;
        if ( eJustification ==  SvxAdjust::Center )
            nStartX = nMaxLineWidth / 2;
        else if ( eJustification ==  SvxAdjust::Right )
            nStartX = nMaxLineWidth;
    }

    pTmpLine->SetStartPosX( nStartX );

    if ( !aStatus.IsOutliner() )
    {
        if ( rLSItem.GetLineSpaceRule() == SvxLineSpaceRule::Min )
        {
            sal_uInt16 nMinHeight = rLSItem.GetLineHeight();
            sal_uInt16 nTxtHeight = pTmpLine->GetHeight();
            if ( nTxtHeight < nMinHeight )
            {
                // The Ascent has to be adjusted for the difference:
                tools::Long nDiff = nMinHeight - nTxtHeight;
                pTmpLine->SetMaxAscent( static_cast<sal_uInt16>(pTmpLine->GetMaxAscent() + nDiff) );
                pTmpLine->SetHeight( nMinHeight, nTxtHeight );
            }
        }
        else if ( rLSItem.GetLineSpaceRule() == SvxLineSpaceRule::Fix )
        {
            sal_uInt16 nFixHeight = rLSItem.GetLineHeight();
            sal_uInt16 nTxtHeight = pTmpLine->GetHeight();

            pTmpLine->SetMaxAscent( static_cast<sal_uInt16>(pTmpLine->GetMaxAscent() + ( nFixHeight - nTxtHeight ) ) );
            pTmpLine->SetHeight( nFixHeight, nTxtHeight );
        }
        else if ( rLSItem.GetInterLineSpaceRule() == SvxInterLineSpaceRule::Prop )
        {
            sal_Int32 nPara = GetParaPortions().GetPos( pParaPortion );
            if ( nPara || pTmpLine->GetStartPortion() ) // Not the very first line
            {
                // There are documents with PropLineSpace 0, why?
                // (cmc: re above question :-) such documents can be seen by importing a .ppt
                if ( rLSItem.GetPropLineSpace() && ( rLSItem.GetPropLineSpace() != 100 ) )
                {
                    sal_uInt16 nTxtHeight = pTmpLine->GetHeight();
                    sal_Int32 nH = nTxtHeight;
                    nH *= rLSItem.GetPropLineSpace();
                    nH /= 100;
                    // The Ascent has to be adjusted for the difference:
                    tools::Long nDiff = pTmpLine->GetHeight() - nH;
                    if ( nDiff > pTmpLine->GetMaxAscent() )
                        nDiff = pTmpLine->GetMaxAscent();
                    pTmpLine->SetMaxAscent( static_cast<sal_uInt16>(pTmpLine->GetMaxAscent() - nDiff) );
                    pTmpLine->SetHeight( static_cast<sal_uInt16>(nH), nTxtHeight );
                }
            }
        }
    }

    if ( !bLineBreak )
    {
        tools::Long nMinHeight = aBulletArea.GetHeight();
        if ( nMinHeight > static_cast<tools::Long>(pTmpLine->GetHeight()) )
        {
            tools::Long nDiff = nMinHeight - static_cast<tools::Long>(pTmpLine->GetHeight());
            // distribute nDiff upwards and downwards
            pTmpLine->SetMaxAscent( static_cast<sal_uInt16>(pTmpLine->GetMaxAscent() + nDiff/2) );
            pTmpLine->SetHeight( static_cast<sal_uInt16>(nMinHeight) );
        }
    }
    else
    {
        // -2: The new one is already inserted.
#ifdef DBG_UTIL
        EditLine& rLastLine = pParaPortion->GetLines()[pParaPortion->GetLines().Count()-2];
        DBG_ASSERT( rLastLine.GetEnd() == pParaPortion->GetNode()->Len(), "different anyway?" );
#endif
        sal_Int32 nPos = pParaPortion->GetTextPortions().Count() - 1 ;
        pTmpLine->SetStartPortion( nPos );
        pTmpLine->SetEndPortion( nPos );
    }
}

bool ImpEditEngine::FinishCreateLines( ParaPortion* pParaPortion )
{
//  CalcCharPositions( pParaPortion );
    pParaPortion->SetValid();
    tools::Long nOldHeight = pParaPortion->GetHeight();
    CalcHeight( pParaPortion );

    DBG_ASSERT( pParaPortion->GetTextPortions().Count(), "FinishCreateLines: No Text-Portion?" );
    bool bRet = ( pParaPortion->GetHeight() != nOldHeight );
    return bRet;
}

void ImpEditEngine::ImpBreakLine( ParaPortion* pParaPortion, EditLine* pLine, TextPortion const * pPortion, sal_Int32 nPortionStart, tools::Long nRemainingWidth, bool bCanHyphenate )
{
    ContentNode* const pNode = pParaPortion->GetNode();

    sal_Int32 nBreakInLine = nPortionStart - pLine->GetStart();
    sal_Int32 nMax = nBreakInLine + pPortion->GetLen();
    while ( ( nBreakInLine < nMax ) && ( pLine->GetCharPosArray()[nBreakInLine] < nRemainingWidth ) )
        nBreakInLine++;

    sal_Int32 nMaxBreakPos = nBreakInLine + pLine->GetStart();
    sal_Int32 nBreakPos = SAL_MAX_INT32;

    bool bCompressBlank = false;
    bool bHyphenated = false;
    bool bHangingPunctuation = false;
    sal_Unicode cAlternateReplChar = 0;
    sal_Unicode cAlternateExtraChar = 0;
    bool bAltFullLeft = false;
    bool bAltFullRight = false;
    sal_uInt32 nAltDelChar = 0;

    if ( ( nMaxBreakPos < ( nMax + pLine->GetStart() ) ) && ( pNode->GetChar( nMaxBreakPos ) == ' ' ) )
    {
        // Break behind the blank, blank will be compressed...
        nBreakPos = nMaxBreakPos + 1;
        bCompressBlank = true;
    }
    else
    {
        sal_Int32 nMinBreakPos = pLine->GetStart();
        const CharAttribList::AttribsType& rAttrs = pNode->GetCharAttribs().GetAttribs();
        for (size_t nAttr = rAttrs.size(); nAttr; )
        {
            const EditCharAttrib& rAttr = *rAttrs[--nAttr];
            if (rAttr.IsFeature() && rAttr.GetEnd() > nMinBreakPos && rAttr.GetEnd() <= nMaxBreakPos)
            {
                nMinBreakPos = rAttr.GetEnd();
                break;
            }
        }
        assert(nMinBreakPos <= nMaxBreakPos);

        lang::Locale aLocale = GetLocale( EditPaM( pNode, nMaxBreakPos ) );

        Reference < i18n::XBreakIterator > _xBI( ImplGetBreakIterator() );
        const bool bAllowPunctuationOutsideMargin = static_cast<const SfxBoolItem&>(
                pNode->GetContentAttribs().GetItem( EE_PARA_HANGINGPUNCTUATION )).GetValue();

        if (nMinBreakPos == nMaxBreakPos)
        {
            nBreakPos = nMinBreakPos;
        }
        else
        {
            Reference< XHyphenator > xHyph;
            if ( bCanHyphenate )
                xHyph = GetHyphenator();
            i18n::LineBreakHyphenationOptions aHyphOptions( xHyph, Sequence< PropertyValue >(), 1 );
            i18n::LineBreakUserOptions aUserOptions;

            const i18n::ForbiddenCharacters* pForbidden = GetForbiddenCharsTable()->GetForbiddenCharacters( LanguageTag::convertToLanguageType( aLocale ), true );
            aUserOptions.forbiddenBeginCharacters = pForbidden->beginLine;
            aUserOptions.forbiddenEndCharacters = pForbidden->endLine;
            aUserOptions.applyForbiddenRules = static_cast<const SfxBoolItem&>(pNode->GetContentAttribs().GetItem( EE_PARA_FORBIDDENRULES )).GetValue();
            aUserOptions.allowPunctuationOutsideMargin = bAllowPunctuationOutsideMargin;
            aUserOptions.allowHyphenateEnglish = false;

            i18n::LineBreakResults aLBR = _xBI->getLineBreak(
                pNode->GetString(), nMaxBreakPos, aLocale, nMinBreakPos, aHyphOptions, aUserOptions );
            nBreakPos = aLBR.breakIndex;

            // BUG in I18N - under special condition (break behind field, #87327#) breakIndex is < nMinBreakPos
            if ( nBreakPos < nMinBreakPos )
            {
                nBreakPos = nMinBreakPos;
            }
            else if ( ( nBreakPos > nMaxBreakPos ) && !aUserOptions.allowPunctuationOutsideMargin )
            {
                OSL_FAIL( "I18N: XBreakIterator::getLineBreak returns position > Max" );
                nBreakPos = nMaxBreakPos;
            }
            // Hanging punctuation is the only case that increases nBreakPos and makes
            // nBreakPos > nMaxBreakPos. It's expected that the hanging punctuation goes over
            // the border of the object.
        }

        // BUG in I18N - the japanese dot is in the next line!
        // !!!  Test!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if ( (nBreakPos + ( bAllowPunctuationOutsideMargin ? 0 : 1 ) ) <= nMaxBreakPos )
        {
            sal_Unicode cFirstInNextLine = ( (nBreakPos+1) < pNode->Len() ) ? pNode->GetChar( nBreakPos ) : 0;
            if ( cFirstInNextLine == 12290 )
                nBreakPos++;
        }

        bHangingPunctuation = nBreakPos > nMaxBreakPos;
        pLine->SetHangingPunctuation( bHangingPunctuation );

        // Whether a separator or not, push the word after the separator through
        // hyphenation... NMaxBreakPos is the last character that fits into
        // the line, nBreakPos is the beginning of the word.
        // There is a problem if the Doc is so narrow that a word is broken
        // into more than two lines...
        if ( !bHangingPunctuation && bCanHyphenate && GetHyphenator().is() )
        {
            i18n::Boundary aBoundary = _xBI->getWordBoundary(
                pNode->GetString(), nBreakPos, GetLocale( EditPaM( pNode, nBreakPos ) ), css::i18n::WordType::DICTIONARY_WORD, true);
            sal_Int32 nWordStart = nBreakPos;
            sal_Int32 nWordEnd = aBoundary.endPos;
            DBG_ASSERT( nWordEnd >= nWordStart, "Start >= End?" );

            sal_Int32 nWordLen = nWordEnd - nWordStart;
            if ( ( nWordEnd >= nMaxBreakPos ) && ( nWordLen > 3 ) )
            {
                // May happen, because getLineBreak may differ from getWordBoundary with DICTIONARY_WORD
                const OUString aWord = pNode->GetString().copy(nWordStart, nWordLen);
                sal_Int32 nMinTrail = nWordEnd-nMaxBreakPos+1; //+1: Before the dickey letter
                Reference< XHyphenatedWord > xHyphWord;
                if (xHyphenator.is())
                    xHyphWord = xHyphenator->hyphenate( aWord, aLocale, aWord.getLength() - nMinTrail, Sequence< PropertyValue >() );
                if (xHyphWord.is())
                {
                    bool bAlternate = xHyphWord->isAlternativeSpelling();
                    sal_Int32 _nWordLen = 1 + xHyphWord->getHyphenPos();

                    if ( ( _nWordLen >= 2 ) && ( (nWordStart+_nWordLen) >= (pLine->GetStart() + 2 ) ) )
                    {
                        if ( !bAlternate )
                        {
                            bHyphenated = true;
                            nBreakPos = nWordStart + _nWordLen;
                        }
                        else
                        {
                            // TODO: handle all alternative hyphenations (see hyphen-1.2.8/tests/unicode.*)
                            OUString aAlt( xHyphWord->getHyphenatedWord() );
                            OUString aAltLeft(aAlt.copy(0, _nWordLen));
                            OUString aAltRight(aAlt.copy(_nWordLen));
                            bAltFullLeft = aWord.startsWith(aAltLeft);
                            bAltFullRight = aWord.endsWith(aAltRight);
                            nAltDelChar = aWord.getLength() - aAlt.getLength() + static_cast<int>(!bAltFullLeft) + static_cast<int>(!bAltFullRight);

                            // NOTE: improved for other cases, see fdo#63711

                            // We expect[ed] the two cases:
                            // 1) packen becomes pak-ken
                            // 2) Schiffahrt becomes Schiff-fahrt
                            // In case 1, a character has to be replaced
                            // in case 2 a character is added.
                            // The identification is complicated by long
                            // compound words because the Hyphenator separates
                            // all position of the word. [This is not true for libhyphen.]
                            // "Schiffahrtsbrennesseln" -> "Schifffahrtsbrennnesseln"
                            // We can thus actually not directly connect the index of the
                            // AlternativeWord to aWord. The whole issue will be simplified
                            // by a function in the  Hyphenator as soon as AMA builds this in...
                            sal_Int32 nAltStart = _nWordLen - 1;
                            sal_Int32 nTxtStart = nAltStart - (aAlt.getLength() - aWord.getLength());
                            sal_Int32 nTxtEnd = nTxtStart;
                            sal_Int32 nAltEnd = nAltStart;

                            // The regions between the nStart and nEnd is the
                            // difference between alternative and original string.
                            while( nTxtEnd < aWord.getLength() && nAltEnd < aAlt.getLength() &&
                                   aWord[nTxtEnd] != aAlt[nAltEnd] )
                            {
                                ++nTxtEnd;
                                ++nAltEnd;
                            }

                            // If a character is added, then we notice it now:
                            if( nAltEnd > nTxtEnd && nAltStart == nAltEnd &&
                                aWord[ nTxtEnd ] == aAlt[nAltEnd] )
                            {
                                ++nAltEnd;
                                ++nTxtStart;
                                ++nTxtEnd;
                            }

                            DBG_ASSERT( ( nAltEnd - nAltStart ) == 1, "Alternate: Wrong assumption!" );

                            if ( nTxtEnd > nTxtStart )
                                cAlternateReplChar = aAlt[nAltStart];
                            else
                                cAlternateExtraChar = aAlt[nAltStart];

                            bHyphenated = true;
                            nBreakPos = nWordStart + nTxtStart;
                            if ( cAlternateReplChar || aAlt.getLength() < aWord.getLength() || !bAltFullRight) // also for "oma-tje", "re-eel"
                                nBreakPos++;
                        }
                    }
                }
            }
        }

        if ( nBreakPos <= pLine->GetStart() )
        {
            // No separator in line => Chop!
            nBreakPos = nMaxBreakPos;
            // I18N nextCharacters !
            if ( nBreakPos <= pLine->GetStart() )
                nBreakPos = pLine->GetStart() + 1;  // Otherwise infinite loop!
        }
    }

    // the dickey portion is the end portion
    pLine->SetEnd( nBreakPos );

    sal_Int32 nEndPortion = SplitTextPortion( pParaPortion, nBreakPos, pLine );

    if ( !bCompressBlank && !bHangingPunctuation )
    {
        // When justification is not SvxAdjust::Left, it's important to compress
        // the trailing space even if there is enough room for the space...
        // Don't check for SvxAdjust::Left, doesn't matter to compress in this case too...
        assert( nBreakPos > pLine->GetStart() && "ImpBreakLines - BreakPos not expected!" );
        if ( pNode->GetChar( nBreakPos-1 ) == ' ' )
            bCompressBlank = true;
    }

    if ( bCompressBlank || bHangingPunctuation )
    {
        TextPortion& rTP = pParaPortion->GetTextPortions()[nEndPortion];
        DBG_ASSERT( rTP.GetKind() == PortionKind::TEXT, "BlankRubber: No TextPortion!" );
        DBG_ASSERT( nBreakPos > pLine->GetStart(), "SplitTextPortion at the beginning of the line?" );
        sal_Int32 nPosInArray = nBreakPos - 1 - pLine->GetStart();
        rTP.GetSize().setWidth( ( nPosInArray && ( rTP.GetLen() > 1 ) ) ? pLine->GetCharPosArray()[ nPosInArray-1 ] : 0 );
        if (o3tl::make_unsigned(nPosInArray) < pLine->GetCharPosArray().size())
        {
            pLine->GetCharPosArray()[ nPosInArray ] = rTP.GetSize().Width();
        }
    }
    else if ( bHyphenated )
    {
        // A portion for inserting the separator...
        TextPortion* pHyphPortion = new TextPortion( 0 );
        pHyphPortion->SetKind( PortionKind::HYPHENATOR );
        if ( (cAlternateReplChar || cAlternateExtraChar) && bAltFullRight ) // alternation after the break doesn't supported
        {
            TextPortion& rPrev = pParaPortion->GetTextPortions()[nEndPortion];
            DBG_ASSERT( rPrev.GetLen(), "Hyphenate: Prev portion?!" );
            rPrev.SetLen( rPrev.GetLen() - nAltDelChar );
            pHyphPortion->SetLen( nAltDelChar );
            if (cAlternateReplChar && !bAltFullLeft) pHyphPortion->SetExtraValue( cAlternateReplChar );
            // Correct width of the portion above:
            rPrev.GetSize().setWidth(
                pLine->GetCharPosArray()[ nBreakPos-1 - pLine->GetStart() - nAltDelChar ] );
        }

        // Determine the width of the Hyph-Portion:
        SvxFont aFont;
        SeekCursor( pParaPortion->GetNode(), nBreakPos, aFont );
        aFont.SetPhysFont(*GetRefDevice());
        pHyphPortion->GetSize().setHeight( GetRefDevice()->GetTextHeight() );
        pHyphPortion->GetSize().setWidth( GetRefDevice()->GetTextWidth( CH_HYPH ) );

        pParaPortion->GetTextPortions().Insert(++nEndPortion, pHyphPortion);
    }
    pLine->SetEndPortion( nEndPortion );
}

void ImpEditEngine::ImpAdjustBlocks( ParaPortion* pParaPortion, EditLine* pLine, tools::Long nRemainingSpace )
{
    DBG_ASSERT( nRemainingSpace > 0, "AdjustBlocks: Somewhat too little..." );
    assert( pLine && "AdjustBlocks: Line ?!" );
    if ( ( nRemainingSpace < 0 ) || pLine->IsEmpty() )
        return ;

    const sal_Int32 nFirstChar = pLine->GetStart();
    const sal_Int32 nLastChar = pLine->GetEnd() -1;    // Last points behind
    ContentNode* pNode = pParaPortion->GetNode();

    DBG_ASSERT( nLastChar < pNode->Len(), "AdjustBlocks: Out of range!" );

    // Search blanks or Kashidas...
    std::vector<sal_Int32> aPositions;
    sal_uInt16 nLastScript = i18n::ScriptType::LATIN;
    for ( sal_Int32 nChar = nFirstChar; nChar <= nLastChar; nChar++ )
    {
        EditPaM aPaM( pNode, nChar+1 );
        LanguageType eLang = GetLanguage(aPaM);
        sal_uInt16 nScript = GetI18NScriptType(aPaM);
        if ( MsLangId::getPrimaryLanguage( eLang) == LANGUAGE_ARABIC_PRIMARY_ONLY )
            // Arabic script is handled later.
            continue;

        if ( pNode->GetChar(nChar) == ' ' )
        {
            // Normal latin script.
            aPositions.push_back( nChar );
        }
        else if (nChar > nFirstChar)
        {
            if (nLastScript == i18n::ScriptType::ASIAN)
            {
                // Set break position between this and the last character if
                // the last character is asian script.
                aPositions.push_back( nChar-1 );
            }
            else if (nScript == i18n::ScriptType::ASIAN)
            {
                // Set break position between a latin script and asian script.
                aPositions.push_back( nChar-1 );
            }
        }

        nLastScript = nScript;
    }

    // Kashidas ?
    ImpFindKashidas( pNode, nFirstChar, nLastChar, aPositions );

    if ( aPositions.empty() )
        return;

    // If the last character is a blank, it is rejected!
    // The width must be distributed to the blockers in front...
    // But not if it is the only one.
    if ( ( pNode->GetChar( nLastChar ) == ' ' ) && ( aPositions.size() > 1 ) &&
         ( MsLangId::getPrimaryLanguage( GetLanguage( EditPaM( pNode, nLastChar ) ) ) != LANGUAGE_ARABIC_PRIMARY_ONLY ) )
    {
        aPositions.pop_back();
        sal_Int32 nPortionStart, nPortion;
        nPortion = pParaPortion->GetTextPortions().FindPortion( nLastChar+1, nPortionStart );
        TextPortion& rLastPortion = pParaPortion->GetTextPortions()[ nPortion ];
        tools::Long nRealWidth = pLine->GetCharPosArray()[nLastChar-nFirstChar];
        tools::Long nBlankWidth = nRealWidth;
        if ( nLastChar > nPortionStart )
            nBlankWidth -= pLine->GetCharPosArray()[nLastChar-nFirstChar-1];
        // Possibly the blank has already been deducted in ImpBreakLine:
        if ( nRealWidth == rLastPortion.GetSize().Width() )
        {
            // For the last character the portion must stop behind the blank
            // => Simplify correction:
            DBG_ASSERT( ( nPortionStart + rLastPortion.GetLen() ) == ( nLastChar+1 ), "Blank actually not at the end of the portion!?");
            rLastPortion.GetSize().AdjustWidth( -nBlankWidth );
            nRemainingSpace += nBlankWidth;
        }
        pLine->GetCharPosArray()[nLastChar-nFirstChar] -= nBlankWidth;
    }

    size_t nGaps = aPositions.size();
    const tools::Long nMore4Everyone = nRemainingSpace / nGaps;
    tools::Long nSomeExtraSpace = nRemainingSpace - nMore4Everyone*nGaps;

    DBG_ASSERT( nSomeExtraSpace < static_cast<tools::Long>(nGaps), "AdjustBlocks: ExtraSpace too large" );
    DBG_ASSERT( nSomeExtraSpace >= 0, "AdjustBlocks: ExtraSpace < 0 " );

    // Correct the positions in the Array and the portion widths:
    // Last character won't be considered...
    for (auto const& nChar : aPositions)
    {
        if ( nChar < nLastChar )
        {
            sal_Int32 nPortionStart, nPortion;
            nPortion = pParaPortion->GetTextPortions().FindPortion( nChar, nPortionStart, true );
            TextPortion& rLastPortion = pParaPortion->GetTextPortions()[ nPortion ];

            // The width of the portion:
            rLastPortion.GetSize().AdjustWidth(nMore4Everyone );
            if ( nSomeExtraSpace )
                rLastPortion.GetSize().AdjustWidth( 1 );

            // Correct positions in array
            // Even for kashidas just change positions, VCL will then draw the kashida automatically
            sal_Int32 nPortionEnd = nPortionStart + rLastPortion.GetLen();
            for ( sal_Int32 _n = nChar; _n < nPortionEnd; _n++ )
            {
                pLine->GetCharPosArray()[_n-nFirstChar] += nMore4Everyone;
                if ( nSomeExtraSpace )
                    pLine->GetCharPosArray()[_n-nFirstChar]++;
            }

            if ( nSomeExtraSpace )
                nSomeExtraSpace--;
        }
    }

    // Now the text width contains the extra width...
    pLine->SetTextWidth( pLine->GetTextWidth() + nRemainingSpace );
}

void ImpEditEngine::ImpFindKashidas( ContentNode* pNode, sal_Int32 nStart, sal_Int32 nEnd, std::vector<sal_Int32>& rArray )
{
    // the search has to be performed on a per word base

    EditSelection aWordSel( EditPaM( pNode, nStart ) );
    aWordSel = SelectWord( aWordSel, css::i18n::WordType::DICTIONARY_WORD );
    if ( aWordSel.Min().GetIndex() < nStart )
       aWordSel.Min().SetIndex( nStart );

    while ( ( aWordSel.Min().GetNode() == pNode ) && ( aWordSel.Min().GetIndex() < nEnd ) )
    {
        const sal_Int32 nSavPos = aWordSel.Max().GetIndex();
        if ( aWordSel.Max().GetIndex() > nEnd )
           aWordSel.Max().SetIndex( nEnd );

        OUString aWord = GetSelected( aWordSel );

        // restore selection for proper iteration at the end of the function
        aWordSel.Max().SetIndex( nSavPos );

        sal_Int32 nIdx = 0;
        sal_Int32 nKashidaPos = -1;
        sal_Unicode cCh;
        sal_Unicode cPrevCh = 0;

        while ( nIdx < aWord.getLength() )
        {
            cCh = aWord[ nIdx ];

            // 1. Priority:
            // after user inserted kashida
            if ( 0x640 == cCh )
            {
                nKashidaPos = aWordSel.Min().GetIndex() + nIdx;
                break;
            }

            // 2. Priority:
            // after a Seen or Sad
            if ( nIdx + 1 < aWord.getLength() &&
                 ( 0x633 == cCh || 0x635 == cCh ) )
            {
                nKashidaPos = aWordSel.Min().GetIndex() + nIdx;
                break;
            }

            // 3. Priority:
            // before final form of the Marbuta, Hah, Dal
            // 4. Priority:
            // before final form of Alef, Lam or Kaf
            if ( nIdx && nIdx + 1 == aWord.getLength() &&
                 ( 0x629 == cCh || 0x62D == cCh || 0x62F == cCh ||
                   0x627 == cCh || 0x644 == cCh || 0x643 == cCh ) )
            {
                DBG_ASSERT( 0 != cPrevCh, "No previous character" );

                // check if character is connectable to previous character,
                if ( lcl_ConnectToPrev( cCh, cPrevCh ) )
                {
                    nKashidaPos = aWordSel.Min().GetIndex() + nIdx - 1;
                    break;
                }
            }

            // 5. Priority:
            // before media Bah
            if ( nIdx && nIdx + 1 < aWord.getLength() && 0x628 == cCh )
            {
                DBG_ASSERT( 0 != cPrevCh, "No previous character" );

                // check if next character is Reh, Yeh or Alef Maksura
                sal_Unicode cNextCh = aWord[ nIdx + 1 ];

                if ( 0x631 == cNextCh || 0x64A == cNextCh ||
                     0x649 == cNextCh )
                {
                    // check if character is connectable to previous character,
                    if ( lcl_ConnectToPrev( cCh, cPrevCh ) )
                        nKashidaPos = aWordSel.Min().GetIndex() + nIdx - 1;
                }
            }

            // 6. Priority:
            // other connecting possibilities
            if ( nIdx && nIdx + 1 == aWord.getLength() &&
                 0x60C <= cCh && 0x6FE >= cCh )
            {
                DBG_ASSERT( 0 != cPrevCh, "No previous character" );

                // check if character is connectable to previous character,
                if ( lcl_ConnectToPrev( cCh, cPrevCh ) )
                {
                    // only choose this position if we did not find
                    // a better one:
                    if ( nKashidaPos<0 )
                        nKashidaPos = aWordSel.Min().GetIndex() + nIdx - 1;
                    break;
                }
            }

            // Do not consider Fathatan, Dammatan, Kasratan, Fatha,
            // Damma, Kasra, Shadda and Sukun when checking if
            // a character can be connected to previous character.
            if ( cCh < 0x64B || cCh > 0x652 )
                cPrevCh = cCh;

            ++nIdx;
        } // end of current word

        if ( nKashidaPos>=0 )
            rArray.push_back( nKashidaPos );

        aWordSel = WordRight( aWordSel.Max(), css::i18n::WordType::DICTIONARY_WORD );
        aWordSel = SelectWord( aWordSel, css::i18n::WordType::DICTIONARY_WORD );
    }
}

sal_Int32 ImpEditEngine::SplitTextPortion( ParaPortion* pPortion, sal_Int32 nPos, EditLine* pCurLine )
{
    // The portion at nPos is split, if there is not a transition at nPos anyway
    if ( nPos == 0 )
        return 0;

    assert( pPortion && "SplitTextPortion: Which ?" );

    sal_Int32 nSplitPortion;
    sal_Int32 nTmpPos = 0;
    TextPortion* pTextPortion = nullptr;
    sal_Int32 nPortions = pPortion->GetTextPortions().Count();
    for ( nSplitPortion = 0; nSplitPortion < nPortions; nSplitPortion++ )
    {
        TextPortion& rTP = pPortion->GetTextPortions()[nSplitPortion];
        nTmpPos = nTmpPos + rTP.GetLen();
        if ( nTmpPos >= nPos )
        {
            if ( nTmpPos == nPos )  // then nothing needs to be split
            {
                return nSplitPortion;
            }
            pTextPortion = &rTP;
            break;
        }
    }

    DBG_ASSERT( pTextPortion, "Position outside the area!" );

    if (!pTextPortion)
        return 0;

    DBG_ASSERT( pTextPortion->GetKind() == PortionKind::TEXT, "SplitTextPortion: No TextPortion!" );

    sal_Int32 nOverlapp = nTmpPos - nPos;
    pTextPortion->SetLen( pTextPortion->GetLen() - nOverlapp );
    TextPortion* pNewPortion = new TextPortion( nOverlapp );
    pPortion->GetTextPortions().Insert(nSplitPortion+1, pNewPortion);
    // Set sizes
    if ( pCurLine )
    {
        // No new GetTextSize, instead use values from the Array:
        assert( nPos > pCurLine->GetStart() && "SplitTextPortion at the beginning of the line?" );
        pTextPortion->GetSize().setWidth( pCurLine->GetCharPosArray()[ nPos-pCurLine->GetStart()-1 ] );

        if ( pTextPortion->GetExtraInfos() && pTextPortion->GetExtraInfos()->bCompressed )
        {
            // We need the original size from the portion
            sal_Int32 nTxtPortionStart = pPortion->GetTextPortions().GetStartPos( nSplitPortion );
            SvxFont aTmpFont( pPortion->GetNode()->GetCharAttribs().GetDefFont() );
            SeekCursor( pPortion->GetNode(), nTxtPortionStart+1, aTmpFont );
            aTmpFont.SetPhysFont(*GetRefDevice());
            GetRefDevice()->Push( vcl::PushFlags::TEXTLANGUAGE );
            ImplInitDigitMode(*GetRefDevice(), aTmpFont.GetLanguage());
            Size aSz = aTmpFont.QuickGetTextSize( GetRefDevice(), pPortion->GetNode()->GetString(), nTxtPortionStart, pTextPortion->GetLen() );
            GetRefDevice()->Pop();
            pTextPortion->GetExtraInfos()->nOrgWidth = aSz.Width();
        }
    }
    else
        pTextPortion->GetSize().setWidth( -1 );

    return nSplitPortion;
}

void ImpEditEngine::CreateTextPortions( ParaPortion* pParaPortion, sal_Int32& rStart )
{
    sal_Int32 nStartPos = rStart;
    ContentNode* pNode = pParaPortion->GetNode();
    DBG_ASSERT( pNode->Len(), "CreateTextPortions should not be used for empty paragraphs!" );

    o3tl::sorted_vector< sal_Int32 > aPositions;
    aPositions.insert( 0 );

    for (sal_uInt16 nAttr = 0;; ++nAttr)
    {
        // Insert Start and End into the Array...
        // The Insert method does not allow for duplicate values...
        EditCharAttrib* pAttrib = GetAttrib(pNode->GetCharAttribs().GetAttribs(), nAttr);
        if (!pAttrib)
            break;
        aPositions.insert( pAttrib->GetStart() );
        aPositions.insert( pAttrib->GetEnd() );
    }
    aPositions.insert( pNode->Len() );

    if ( pParaPortion->aScriptInfos.empty() )
        InitScriptTypes( GetParaPortions().GetPos( pParaPortion ) );

    const ScriptTypePosInfos& rTypes = pParaPortion->aScriptInfos;
    for (const ScriptTypePosInfo& rType : rTypes)
        aPositions.insert( rType.nStartPos );

    const WritingDirectionInfos& rWritingDirections = pParaPortion->aWritingDirectionInfos;
    for (const WritingDirectionInfo & rWritingDirection : rWritingDirections)
        aPositions.insert( rWritingDirection.nStartPos );

    if ( mpIMEInfos && mpIMEInfos->nLen && mpIMEInfos->pAttribs && ( mpIMEInfos->aPos.GetNode() == pNode ) )
    {
        ExtTextInputAttr nLastAttr = ExtTextInputAttr(0xFFFF);
        for( sal_Int32 n = 0; n < mpIMEInfos->nLen; n++ )
        {
            if ( mpIMEInfos->pAttribs[n] != nLastAttr )
            {
                aPositions.insert( mpIMEInfos->aPos.GetIndex() + n );
                nLastAttr = mpIMEInfos->pAttribs[n];
            }
        }
        aPositions.insert( mpIMEInfos->aPos.GetIndex() + mpIMEInfos->nLen );
    }

    // From ... Delete:
    // Unfortunately, the number of text portions does not have to match
    // aPositions.Count(), since there might be line breaks...
    sal_Int32 nPortionStart = 0;
    sal_Int32 nInvPortion = 0;
    sal_Int32 nP;
    for ( nP = 0; nP < pParaPortion->GetTextPortions().Count(); nP++ )
    {
        const TextPortion& rTmpPortion = pParaPortion->GetTextPortions()[nP];
        nPortionStart = nPortionStart + rTmpPortion.GetLen();
        if ( nPortionStart >= nStartPos )
        {
            nPortionStart = nPortionStart - rTmpPortion.GetLen();
            rStart = nPortionStart;
            nInvPortion = nP;
            break;
        }
    }
    DBG_ASSERT( nP < pParaPortion->GetTextPortions().Count() || !pParaPortion->GetTextPortions().Count(), "Nothing to delete: CreateTextPortions" );
    if ( nInvPortion && ( nPortionStart+pParaPortion->GetTextPortions()[nInvPortion].GetLen() > nStartPos ) )
    {
        // prefer one in front...
        // But only if it was in the middle of the portion of, otherwise it
        // might be the only one in the row in front!
        nInvPortion--;
        nPortionStart = nPortionStart - pParaPortion->GetTextPortions()[nInvPortion].GetLen();
    }
    pParaPortion->GetTextPortions().DeleteFromPortion( nInvPortion );

    // A portion may also have been formed by a line break:
    aPositions.insert( nPortionStart );

    auto nInvPos = aPositions.find(  nPortionStart );
    DBG_ASSERT( (nInvPos != aPositions.end()), "InvPos ?!" );

    auto i = nInvPos;
    ++i;
    while ( i != aPositions.end() )
    {
        TextPortion* pNew = new TextPortion( (*i++) - *nInvPos++ );
        pParaPortion->GetTextPortions().Append(pNew);
    }

    DBG_ASSERT( pParaPortion->GetTextPortions().Count(), "No Portions?!" );
#if OSL_DEBUG_LEVEL > 0
    OSL_ENSURE( ParaPortion::DbgCheckTextPortions(*pParaPortion), "Portion is broken?" );
#endif
}

void ImpEditEngine::RecalcTextPortion( ParaPortion* pParaPortion, sal_Int32 nStartPos, sal_Int32 nNewChars )
{
    DBG_ASSERT( pParaPortion->GetTextPortions().Count(), "No Portions!" );
    DBG_ASSERT( nNewChars, "RecalcTextPortion with Diff == 0" );

    ContentNode* const pNode = pParaPortion->GetNode();
    if ( nNewChars > 0 )
    {
        // If an Attribute begins/ends at nStartPos, then a new portion starts
        // otherwise the portion is extended at nStartPos.
        if ( pNode->GetCharAttribs().HasBoundingAttrib( nStartPos ) || IsScriptChange( EditPaM( pNode, nStartPos ) ) )
        {
            sal_Int32 nNewPortionPos = 0;
            if ( nStartPos )
                nNewPortionPos = SplitTextPortion( pParaPortion, nStartPos ) + 1;

            // A blank portion may be here, if the paragraph was empty,
            // or if a line was created by a hard line break.
            if ( ( nNewPortionPos < pParaPortion->GetTextPortions().Count() ) &&
                    !pParaPortion->GetTextPortions()[nNewPortionPos].GetLen() )
            {
                TextPortion& rTP = pParaPortion->GetTextPortions()[nNewPortionPos];
                DBG_ASSERT( rTP.GetKind() == PortionKind::TEXT, "the empty portion was no TextPortion!" );
                rTP.SetLen( rTP.GetLen() + nNewChars );
            }
            else
            {
                TextPortion* pNewPortion = new TextPortion( nNewChars );
                pParaPortion->GetTextPortions().Insert(nNewPortionPos, pNewPortion);
            }
        }
        else
        {
            sal_Int32 nPortionStart;
            const sal_Int32 nTP = pParaPortion->GetTextPortions().
                FindPortion( nStartPos, nPortionStart );
            TextPortion& rTP = pParaPortion->GetTextPortions()[ nTP ];
            rTP.SetLen( rTP.GetLen() + nNewChars );
            rTP.GetSize().setWidth( -1 );
        }
    }
    else
    {
        // Shrink or remove portion if necessary.
        // Before calling this method it must be ensured that no portions were
        // in the deleted area!

        // There must be no portions extending into the area or portions starting in
        // the area, so it must be:
        //    nStartPos <= nPos <= nStartPos - nNewChars(neg.)
        sal_Int32 nPortion = 0;
        sal_Int32 nPos = 0;
        sal_Int32 nEnd = nStartPos-nNewChars;
        sal_Int32 nPortions = pParaPortion->GetTextPortions().Count();
        TextPortion* pTP = nullptr;
        for ( nPortion = 0; nPortion < nPortions; nPortion++ )
        {
            pTP = &pParaPortion->GetTextPortions()[ nPortion ];
            if ( ( nPos+pTP->GetLen() ) > nStartPos )
            {
                DBG_ASSERT( nPos <= nStartPos, "Wrong Start!" );
                DBG_ASSERT( nPos+pTP->GetLen() >= nEnd, "Wrong End!" );
                break;
            }
            nPos = nPos + pTP->GetLen();
        }
        assert( pTP && "RecalcTextPortion: Portion not found" );
        if ( ( nPos == nStartPos ) && ( (nPos+pTP->GetLen()) == nEnd ) )
        {
            // Remove portion;
            PortionKind nType = pTP->GetKind();
            pParaPortion->GetTextPortions().Remove( nPortion );
            if ( nType == PortionKind::LINEBREAK )
            {
                TextPortion& rNext = pParaPortion->GetTextPortions()[ nPortion ];
                if ( !rNext.GetLen() )
                {
                    // Remove dummy portion
                    pParaPortion->GetTextPortions().Remove( nPortion );
                }
            }
        }
        else
        {
            DBG_ASSERT( pTP->GetLen() > (-nNewChars), "Portion too small to shrink! ");
            pTP->SetLen( pTP->GetLen() + nNewChars );
        }

        sal_Int32 nPortionCount = pParaPortion->GetTextPortions().Count();
        assert( nPortionCount );
        if (nPortionCount)
        {
            // No HYPHENATOR portion is allowed to get stuck right at the end...
            sal_Int32 nLastPortion = nPortionCount - 1;
            pTP = &pParaPortion->GetTextPortions()[nLastPortion];
            if ( pTP->GetKind() == PortionKind::HYPHENATOR )
            {
                // Discard portion; if possible, correct the ones before,
                // if the Hyphenator portion has swallowed one character...
                if ( nLastPortion && pTP->GetLen() )
                {
                    TextPortion& rPrev = pParaPortion->GetTextPortions()[nLastPortion - 1];
                    DBG_ASSERT( rPrev.GetKind() == PortionKind::TEXT, "Portion?!" );
                    rPrev.SetLen( rPrev.GetLen() + pTP->GetLen() );
                    rPrev.GetSize().setWidth( -1 );
                }
                pParaPortion->GetTextPortions().Remove( nLastPortion );
            }
        }
    }
#if OSL_DEBUG_LEVEL > 0
    OSL_ENSURE( ParaPortion::DbgCheckTextPortions(*pParaPortion), "Portions are broken?" );
#endif
}

void ImpEditEngine::SetTextRanger( std::unique_ptr<TextRanger> pRanger )
{
    pTextRanger = std::move(pRanger);

    for ( sal_Int32 nPara = 0; nPara < GetParaPortions().Count(); nPara++ )
    {
        ParaPortion& rParaPortion = GetParaPortions()[nPara];
        rParaPortion.MarkSelectionInvalid( 0 );
        rParaPortion.GetLines().Reset();
    }

    FormatFullDoc();
    UpdateViews( GetActiveView() );
    if ( IsUpdateLayout() && GetActiveView() )
        pActiveView->ShowCursor(false, false);
}

void ImpEditEngine::SetVertical( bool bVertical)
{
    if ( IsEffectivelyVertical() != bVertical)
    {
        GetEditDoc().SetVertical(bVertical);
        bool bUseCharAttribs = bool(aStatus.GetControlWord() & EEControlBits::USECHARATTRIBS);
        GetEditDoc().CreateDefFont( bUseCharAttribs );
        if ( IsFormatted() )
        {
            FormatFullDoc();
            UpdateViews( GetActiveView() );
        }
    }
}

void ImpEditEngine::SetRotation(TextRotation nRotation)
{
    if (GetEditDoc().GetRotation() == nRotation)
        return; // not modified
    GetEditDoc().SetRotation(nRotation);
    bool bUseCharAttribs = bool(aStatus.GetControlWord() & EEControlBits::USECHARATTRIBS);
    GetEditDoc().CreateDefFont( bUseCharAttribs );
    if ( IsFormatted() )
    {
        FormatFullDoc();
        UpdateViews( GetActiveView() );
    }
}

void ImpEditEngine::SetTextColumns(sal_Int16 nColumns, sal_Int32 nSpacing)
{
    if (mnColumns != nColumns || mnColumnSpacing != nSpacing)
    {
        mnColumns = nColumns;
        mnColumnSpacing = nSpacing;
        if (IsFormatted())
        {
            FormatFullDoc();
            UpdateViews(GetActiveView());
        }
    }
}

void ImpEditEngine::SetFixedCellHeight( bool bUseFixedCellHeight )
{
    if ( IsFixedCellHeight() != bUseFixedCellHeight )
    {
        GetEditDoc().SetFixedCellHeight( bUseFixedCellHeight );
        if ( IsFormatted() )
        {
            FormatFullDoc();
            UpdateViews( GetActiveView() );
        }
    }
}

void ImpEditEngine::SeekCursor( ContentNode* pNode, sal_Int32 nPos, SvxFont& rFont, OutputDevice* pOut )
{
    // It was planned, SeekCursor( nStartPos, nEndPos,... ), so that it would
    // only be searched anew at the StartPosition.
    // Problem: There would be two lists to consider/handle:
    // OrderedByStart,OrderedByEnd.

    if ( nPos > pNode->Len() )
        nPos = pNode->Len();

    rFont = pNode->GetCharAttribs().GetDefFont();

    /*
     * Set attributes for script types Asian and Complex
    */
    short nScriptTypeI18N = GetI18NScriptType( EditPaM( pNode, nPos ) );
    SvtScriptType nScriptType = SvtLanguageOptions::FromI18NToSvtScriptType(nScriptTypeI18N);
    if ( ( nScriptTypeI18N == i18n::ScriptType::ASIAN ) || ( nScriptTypeI18N == i18n::ScriptType::COMPLEX ) )
    {
        const SvxFontItem& rFontItem = static_cast<const SvxFontItem&>(pNode->GetContentAttribs().GetItem( GetScriptItemId( EE_CHAR_FONTINFO, nScriptType ) ));
        rFont.SetFamilyName( rFontItem.GetFamilyName() );
        rFont.SetFamily( rFontItem.GetFamily() );
        rFont.SetPitch( rFontItem.GetPitch() );
        rFont.SetCharSet( rFontItem.GetCharSet() );
        Size aSz( rFont.GetFontSize() );
        aSz.setHeight( static_cast<const SvxFontHeightItem&>(pNode->GetContentAttribs().GetItem( GetScriptItemId( EE_CHAR_FONTHEIGHT, nScriptType ) ) ).GetHeight() );
        rFont.SetFontSize( aSz );
        rFont.SetWeight( static_cast<const SvxWeightItem&>(pNode->GetContentAttribs().GetItem( GetScriptItemId( EE_CHAR_WEIGHT, nScriptType ))).GetWeight() );
        rFont.SetItalic( static_cast<const SvxPostureItem&>(pNode->GetContentAttribs().GetItem( GetScriptItemId( EE_CHAR_ITALIC, nScriptType ))).GetPosture() );
        rFont.SetLanguage( static_cast<const SvxLanguageItem&>(pNode->GetContentAttribs().GetItem( GetScriptItemId( EE_CHAR_LANGUAGE, nScriptType ))).GetLanguage() );
    }

    sal_uInt16 nRelWidth = pNode->GetContentAttribs().GetItem( EE_CHAR_FONTWIDTH).GetValue();

    /*
     * Set output device's line and overline colors
    */
    if ( pOut )
    {
        const SvxUnderlineItem& rTextLineColor = pNode->GetContentAttribs().GetItem( EE_CHAR_UNDERLINE );
        if ( rTextLineColor.GetColor() != COL_TRANSPARENT )
            pOut->SetTextLineColor( rTextLineColor.GetColor() );
        else
            pOut->SetTextLineColor();

        const SvxOverlineItem& rOverlineColor = pNode->GetContentAttribs().GetItem( EE_CHAR_OVERLINE );
        if ( rOverlineColor.GetColor() != COL_TRANSPARENT )
            pOut->SetOverlineColor( rOverlineColor.GetColor() );
        else
            pOut->SetOverlineColor();
    }

    const SvxLanguageItem* pCJKLanguageItem = nullptr;

    /*
     * Scan through char attributes of pNode
    */
    if ( aStatus.UseCharAttribs() )
    {
        CharAttribList::AttribsType& rAttribs = pNode->GetCharAttribs().GetAttribs();
        size_t nAttr = 0;
        EditCharAttrib* pAttrib = GetAttrib(rAttribs, nAttr);
        while ( pAttrib && ( pAttrib->GetStart() <= nPos ) )
        {
            // when seeking, ignore attributes which start there! Empty attributes
            // are considered (used) as these are just set. But do not use empty
            // attributes: When just set and empty => no effect on font
            // In a blank paragraph, set characters take effect immediately.
            if ( ( pAttrib->Which() != 0 ) &&
                 ( ( ( pAttrib->GetStart() < nPos ) && ( pAttrib->GetEnd() >= nPos ) )
                     || ( !pNode->Len() ) ) )
            {
                DBG_ASSERT( ( pAttrib->Which() >= EE_CHAR_START ) && ( pAttrib->Which() <= EE_FEATURE_END ), "Invalid Attribute in Seek() " );
                if ( IsScriptItemValid( pAttrib->Which(), nScriptTypeI18N ) )
                {
                    pAttrib->SetFont( rFont, pOut );
                    // #i1550# hard color attrib should win over text color from field
                    if ( pAttrib->Which() == EE_FEATURE_FIELD )
                    {
                        EditCharAttrib* pColorAttr = pNode->GetCharAttribs().FindAttrib( EE_CHAR_COLOR, nPos );
                        if ( pColorAttr )
                            pColorAttr->SetFont( rFont, pOut );
                    }
                }
                if ( pAttrib->Which() == EE_CHAR_FONTWIDTH )
                    nRelWidth = static_cast<const SvxCharScaleWidthItem*>(pAttrib->GetItem())->GetValue();
                if ( pAttrib->Which() == EE_CHAR_LANGUAGE_CJK )
                    pCJKLanguageItem = static_cast<const SvxLanguageItem*>( pAttrib->GetItem() );
            }
            pAttrib = GetAttrib( rAttribs, ++nAttr );
        }
    }

    if ( !pCJKLanguageItem )
        pCJKLanguageItem = &pNode->GetContentAttribs().GetItem( EE_CHAR_LANGUAGE_CJK );

    rFont.SetCJKContextLanguage( pCJKLanguageItem->GetLanguage() );

    if ( (rFont.GetKerning() != FontKerning::NONE) && IsKernAsianPunctuation() && ( nScriptTypeI18N == i18n::ScriptType::ASIAN ) )
        rFont.SetKerning( rFont.GetKerning() | FontKerning::Asian );

    if ( aStatus.DoNotUseColors() )
    {
        rFont.SetColor( /* rColorItem.GetValue() */ COL_BLACK );
    }

    if ( aStatus.DoStretch() || ( nRelWidth != 100 ) )
    {
        // For the current Output device, because otherwise if RefDev=Printer its looks
        // ugly on the screen!
        OutputDevice* pDev = pOut ? pOut : GetRefDevice();
        rFont.SetPhysFont(*pDev);
        FontMetric aMetric( pDev->GetFontMetric() );

        // before forcing nPropr to 100%, calculate a new escapement relative to this fake size.
        sal_uInt8 nPropr = rFont.GetPropr();
        sal_Int16 nEsc = rFont.GetEscapement();
        if ( nPropr && nEsc && nPropr != 100 && abs(nEsc) != DFLT_ESC_AUTO_SUPER )
            rFont.SetEscapement( 100.0/nPropr * nEsc );

        // Set the font as we want it to look like & reset the Propr attribute
        // so that it is not counted twice.
        Size aRealSz( aMetric.GetFontSize() );
        rFont.SetPropr( 100 );

        if ( aStatus.DoStretch() )
        {
            if ( nStretchY != 100 )
            {
                aRealSz.setHeight( aRealSz.Height() * nStretchY );
                aRealSz.setHeight( aRealSz.Height() / 100 );
            }
            if ( nStretchX != 100 )
            {
                if ( nStretchX == nStretchY &&
                     nRelWidth == 100 )
                {
                    aRealSz.setWidth( 0 );
                }
                else
                {
                    aRealSz.setWidth( aRealSz.Width() * nStretchX );
                    aRealSz.setWidth( aRealSz.Width() / 100 );

                    // Also the Kerning: (long due to handle Interim results)
                    tools::Long nKerning = rFont.GetFixKerning();
/*
  The consideration was: If negative kerning, but StretchX = 200
  => Do not double the kerning, thus pull the letters closer together
  ---------------------------
  Kern  StretchX    =>Kern
  ---------------------------
  >0        <100        < (Proportional)
  <0        <100        < (Proportional)
  >0        >100        > (Proportional)
  <0        >100        < (The amount, thus disproportional)
*/
                    if ( ( nKerning < 0  ) && ( nStretchX > 100 ) )
                    {
                        // disproportional
                        nKerning *= 100;
                        nKerning /= nStretchX;
                    }
                    else if ( nKerning )
                    {
                        // Proportional
                        nKerning *= nStretchX;
                        nKerning /= 100;
                    }
                    rFont.SetFixKerning( static_cast<short>(nKerning) );
                }
            }
        }
        if ( nRelWidth != 100 )
        {
            aRealSz.setWidth( aRealSz.Width() * nRelWidth );
            aRealSz.setWidth( aRealSz.Width() / 100 );
        }
        rFont.SetFontSize( aRealSz );
        // Font is not restored...
    }

    if ( ( ( rFont.GetColor() == COL_AUTO ) || ( IsForceAutoColor() ) ) && pOut )
    {
        // #i75566# Do not use AutoColor when printing OR Pdf export
        const bool bPrinting(OUTDEV_PRINTER == pOut->GetOutDevType());
        const bool bPDFExporting(OUTDEV_PDF == pOut->GetOutDevType());

        if ( IsAutoColorEnabled() && !bPrinting && !bPDFExporting)
        {
            // Never use WindowTextColor on the printer
            rFont.SetColor( GetAutoColor() );
        }
        else
        {
            if ( ( GetBackgroundColor() != COL_AUTO ) && GetBackgroundColor().IsDark() )
                rFont.SetColor( COL_WHITE );
            else
                rFont.SetColor( COL_BLACK );
        }
    }

    if ( !(mpIMEInfos && mpIMEInfos->pAttribs && ( mpIMEInfos->aPos.GetNode() == pNode ) &&
        ( nPos > mpIMEInfos->aPos.GetIndex() ) && ( nPos <= ( mpIMEInfos->aPos.GetIndex() + mpIMEInfos->nLen ) )) )
        return;

    ExtTextInputAttr nAttr = mpIMEInfos->pAttribs[ nPos - mpIMEInfos->aPos.GetIndex() - 1 ];
    if ( nAttr & ExtTextInputAttr::Underline )
        rFont.SetUnderline( LINESTYLE_SINGLE );
    else if ( nAttr & ExtTextInputAttr::BoldUnderline )
        rFont.SetUnderline( LINESTYLE_BOLD );
    else if ( nAttr & ExtTextInputAttr::DottedUnderline )
        rFont.SetUnderline( LINESTYLE_DOTTED );
    else if ( nAttr & ExtTextInputAttr::DashDotUnderline )
        rFont.SetUnderline( LINESTYLE_DOTTED );
    else if ( nAttr & ExtTextInputAttr::RedText )
        rFont.SetColor( COL_RED );
    else if ( nAttr & ExtTextInputAttr::HalfToneText )
        rFont.SetColor( COL_LIGHTGRAY );
    if ( nAttr & ExtTextInputAttr::Highlight )
    {
        const StyleSettings& rStyleSettings = Application::GetSettings().GetStyleSettings();
        rFont.SetColor( rStyleSettings.GetHighlightTextColor() );
        rFont.SetFillColor( rStyleSettings.GetHighlightColor() );
        rFont.SetTransparent( false );
    }
    else if ( nAttr & ExtTextInputAttr::GrayWaveline )
    {
        rFont.SetUnderline( LINESTYLE_WAVE );
        if( pOut )
            pOut->SetTextLineColor( COL_LIGHTGRAY );
    }
}

void ImpEditEngine::RecalcFormatterFontMetrics( FormatterFontMetric& rCurMetrics, SvxFont& rFont )
{
    // for line height at high / low first without Propr!
    sal_uInt16 nPropr = rFont.GetPropr();
    DBG_ASSERT( ( nPropr == 100 ) || rFont.GetEscapement(), "Propr without Escape?!" );
    if ( nPropr != 100 )
    {
        rFont.SetPropr( 100 );
        rFont.SetPhysFont(*pRefDev);
    }
    sal_uInt16 nAscent, nDescent;

    FontMetric aMetric( pRefDev->GetFontMetric() );
    nAscent = static_cast<sal_uInt16>(aMetric.GetAscent());
    if ( IsAddExtLeading() )
        nAscent = sal::static_int_cast< sal_uInt16 >(
            nAscent + aMetric.GetExternalLeading() );
    nDescent = static_cast<sal_uInt16>(aMetric.GetDescent());

    if ( IsFixedCellHeight() )
    {
        nAscent = sal::static_int_cast< sal_uInt16 >( rFont.GetFontHeight() );
        nDescent= sal::static_int_cast< sal_uInt16 >( ImplCalculateFontIndependentLineSpacing( rFont.GetFontHeight() ) - nAscent );
    }
    else
    {
        sal_uInt16 nIntLeading = ( aMetric.GetInternalLeading() > 0 ) ? static_cast<sal_uInt16>(aMetric.GetInternalLeading()) : 0;
        // Fonts without leading cause problems
        if ( ( nIntLeading == 0 ) && ( pRefDev->GetOutDevType() == OUTDEV_PRINTER ) )
        {
            // Lets see what Leading one gets on the screen
            VclPtr<VirtualDevice> pVDev = GetVirtualDevice( pRefDev->GetMapMode(), pRefDev->GetDrawMode() );
            rFont.SetPhysFont(*pVDev);
            aMetric = pVDev->GetFontMetric();

            // This is so that the Leading does not count itself out again,
            // if the whole line has the font, nTmpLeading.
            nAscent = static_cast<sal_uInt16>(aMetric.GetAscent());
            nDescent = static_cast<sal_uInt16>(aMetric.GetDescent());
        }
    }
    if ( nAscent > rCurMetrics.nMaxAscent )
        rCurMetrics.nMaxAscent = nAscent;
    if ( nDescent > rCurMetrics.nMaxDescent )
        rCurMetrics.nMaxDescent= nDescent;
    // Special treatment of high/low:
    if ( !rFont.GetEscapement() )
        return;

    // Now in consideration of Escape/Propr
    // possibly enlarge Ascent or Descent
    short nDiff = static_cast<short>(rFont.GetFontSize().Height()*rFont.GetEscapement()/100);
    if ( rFont.GetEscapement() > 0 )
    {
        nAscent = static_cast<sal_uInt16>(static_cast<tools::Long>(nAscent)*nPropr/100 + nDiff);
        if ( nAscent > rCurMetrics.nMaxAscent )
            rCurMetrics.nMaxAscent = nAscent;
    }
    else    // has to be < 0
    {
        nDescent = static_cast<sal_uInt16>(static_cast<tools::Long>(nDescent)*nPropr/100 - nDiff);
        if ( nDescent > rCurMetrics.nMaxDescent )
            rCurMetrics.nMaxDescent= nDescent;
    }
}

tools::Long ImpEditEngine::getWidthDirectionAware(const Size& sz) const
{
    return !IsEffectivelyVertical() ? sz.Width() : sz.Height();
}

tools::Long ImpEditEngine::getHeightDirectionAware(const Size& sz) const
{
    return !IsEffectivelyVertical() ? sz.Height() : sz.Width();
}

void ImpEditEngine::adjustXDirectionAware(Point& pt, tools::Long x) const
{
    if (!IsEffectivelyVertical())
        pt.AdjustX(x);
    else
        pt.AdjustY(IsTopToBottom() ? x : -x);
}

void ImpEditEngine::adjustYDirectionAware(Point& pt, tools::Long y) const
{
    if (!IsEffectivelyVertical())
        pt.AdjustY(y);
    else
        pt.AdjustX(IsTopToBottom() ? -y : y);
}

void ImpEditEngine::setXDirectionAwareFrom(Point& ptDest, const Point& ptSrc) const
{
    if (!IsEffectivelyVertical())
        ptDest.setX(ptSrc.X());
    else
        ptDest.setY(ptSrc.Y());
}

void ImpEditEngine::setYDirectionAwareFrom(Point& ptDest, const Point& ptSrc) const
{
    if (!IsEffectivelyVertical())
        ptDest.setY(ptSrc.Y());
    else
        ptDest.setX(ptSrc.Y());
}

tools::Long ImpEditEngine::getYOverflowDirectionAware(const Point& pt,
                                                      const tools::Rectangle& rectMax) const
{
    tools::Long nRes;
    if (!IsEffectivelyVertical())
        nRes = pt.Y() - rectMax.Bottom();
    else if (IsTopToBottom())
        nRes = rectMax.Left() - pt.X();
    else
        nRes = pt.X() - rectMax.Right();
    return std::max(nRes, tools::Long(0));
}

bool ImpEditEngine::isXOverflowDirectionAware(const Point& pt, const tools::Rectangle& rectMax) const
{
    if (!IsEffectivelyVertical())
        return pt.X() > rectMax.Right();

    if (IsTopToBottom())
        return pt.Y() > rectMax.Bottom();
    else
        return pt.Y() < rectMax.Top();
}

tools::Long ImpEditEngine::getBottomDocOffset(const tools::Rectangle& rect) const
{
    if (!IsEffectivelyVertical())
        return rect.Bottom();

    if (IsTopToBottom())
        return -rect.Left();
    else
        return rect.Right();
}

Size ImpEditEngine::getTopLeftDocOffset(const tools::Rectangle& rect) const
{
    if (!IsEffectivelyVertical())
        return { rect.Left(), rect.Top() };

    if (IsTopToBottom())
        return { rect.Top(), -rect.Right() };
    else
        return { -rect.Bottom(), rect.Left() };
}

// Returns the resulting shift for the point; allows to apply the same shift to other points
Point ImpEditEngine::MoveToNextLine(
    Point& rMovePos, // [in, out] Point that will move to the next line
    tools::Long nLineHeight, // [in] Y-direction move distance (direction-aware)
    sal_Int16& rColumn, // [in, out] current column number
    Point aOrigin, // [in] Origin point to calculate limits and initial Y position in a new column
    tools::Long* pnHeightNeededToNotWrap // On column wrap, returns how much more height is needed
) const
{
    const Point aOld = rMovePos;

    // Move the point by the requested distance in Y direction
    adjustYDirectionAware(rMovePos, nLineHeight);
    // Check if the resulting position has moved beyond the limits, and more columns left.
    // The limits are defined by a rectangle starting from aOrigin with width of aPaperSize
    // and height of nCurTextHeight
    Point aOtherCorner = aOrigin;
    adjustXDirectionAware(aOtherCorner, getWidthDirectionAware(aPaperSize));
    adjustYDirectionAware(aOtherCorner, nCurTextHeight);
    tools::Long nNeeded
        = getYOverflowDirectionAware(rMovePos, tools::Rectangle::Justify(aOrigin, aOtherCorner));
    if (pnHeightNeededToNotWrap)
        *pnHeightNeededToNotWrap = nNeeded;
    if (nNeeded && rColumn < mnColumns)
    {
        ++rColumn;
        // If we didn't fit into the last column, indicate that only by setting the column number
        // to the total number of columns; do not adjust
        if (rColumn < mnColumns)
        {
            // Set Y position of the point to that of aOrigin
            setYDirectionAwareFrom(rMovePos, aOrigin);
            // Move the point by the requested distance in Y direction
            adjustYDirectionAware(rMovePos, nLineHeight);
            // Move the point by the column+spacing distance in X direction
            adjustXDirectionAware(rMovePos, GetColumnWidth(aPaperSize) + mnColumnSpacing);
        }
    }

    return rMovePos - aOld;
}

// TODO: use IterateLineAreas in ImpEditEngine::Paint, to avoid algorithm duplication

void ImpEditEngine::Paint( OutputDevice& rOutDev, tools::Rectangle aClipRect, Point aStartPos, bool bStripOnly, Degree10 nOrientation )
{
    if ( !IsUpdateLayout() && !bStripOnly )
        return;

    if ( !IsFormatted() )
        FormatDoc();

    tools::Long nFirstVisXPos = - rOutDev.GetMapMode().GetOrigin().X();
    tools::Long nFirstVisYPos = - rOutDev.GetMapMode().GetOrigin().Y();

    DBG_ASSERT( GetParaPortions().Count(), "No ParaPortion?!" );
    SvxFont aTmpFont( GetParaPortions()[0].GetNode()->GetCharAttribs().GetDefFont() );
    vcl::PDFExtOutDevData* const pPDFExtOutDevData = dynamic_cast< vcl::PDFExtOutDevData* >( rOutDev.GetExtOutDevData() );

    // In the case of rotated text is aStartPos considered TopLeft because
    // other information is missing, and since the whole object is shown anyway
    // un-scrolled.
    // The rectangle is infinite.
    const Point aOrigin( aStartPos );
    double nCos = 0.0, nSin = 0.0;
    if ( nOrientation )
    {
        double nRealOrientation = toRadians(nOrientation);
        nCos = cos( nRealOrientation );
        nSin = sin( nRealOrientation );
    }

    // #110496# Added some more optional metafile comments. This
    // change: factored out some duplicated code.
    GDIMetaFile* pMtf = rOutDev.GetConnectMetaFile();
    const bool bMetafileValid( pMtf != nullptr );

    const tools::Long nVertLineSpacing = CalcVertLineSpacing(aStartPos);

    sal_Int16 nColumn = 0;

    // Over all the paragraphs...

    for ( sal_Int32 n = 0; n < GetParaPortions().Count(); n++ )
    {
        const ParaPortion& rPortion = GetParaPortions()[n];
        // if when typing idle formatting,  asynchronous Paint.
        // Invisible Portions may be invalid.
        if ( rPortion.IsVisible() && rPortion.IsInvalid() )
            return;

        if ( pPDFExtOutDevData )
            pPDFExtOutDevData->BeginStructureElement( vcl::PDFWriter::Paragraph );

        const tools::Long nParaHeight = rPortion.GetHeight();
        if ( rPortion.IsVisible() && (
                ( !IsEffectivelyVertical() && ( ( aStartPos.Y() + nParaHeight ) > aClipRect.Top() ) ) ||
                ( IsEffectivelyVertical() && IsTopToBottom() && ( ( aStartPos.X() - nParaHeight ) < aClipRect.Right() ) ) ||
                ( IsEffectivelyVertical() && !IsTopToBottom() && ( ( aStartPos.X() + nParaHeight ) > aClipRect.Left() ) ) ) )

        {
            Point aTmpPos;

            // Over the lines of the paragraph...

            const sal_Int32 nLines = rPortion.GetLines().Count();
            const sal_Int32 nLastLine = nLines-1;

            bool bEndOfParagraphWritten(false);

            adjustYDirectionAware(aStartPos, rPortion.GetFirstLineOffset());

            const SvxLineSpacingItem& rLSItem = rPortion.GetNode()->GetContentAttribs().GetItem( EE_PARA_SBL );
            sal_uInt16 nSBL = ( rLSItem.GetInterLineSpaceRule() == SvxInterLineSpaceRule::Fix )
                                ? GetYValue( rLSItem.GetInterLineSpace() ) : 0;
            bool bPaintBullet (false);

            for ( sal_Int32 nLine = 0; nLine < nLines; nLine++ )
            {
                const EditLine* const pLine = &rPortion.GetLines()[nLine];
                assert( pLine && "NULL-Pointer in the line iterator in UpdateViews" );
                sal_Int32 nIndex = pLine->GetStart();
                tools::Long nLineHeight = pLine->GetHeight();
                if (nLine != nLastLine)
                    nLineHeight += nVertLineSpacing;
                MoveToNextLine(aStartPos, nLineHeight, nColumn, aOrigin);
                aTmpPos = aStartPos;
                adjustXDirectionAware(aTmpPos, pLine->GetStartPosX());
                adjustYDirectionAware(aTmpPos, pLine->GetMaxAscent() - nLineHeight);

                if ( ( !IsEffectivelyVertical() && ( aStartPos.Y() > aClipRect.Top() ) )
                    || ( IsEffectivelyVertical() && IsTopToBottom() && aStartPos.X() < aClipRect.Right() )
                    || ( IsEffectivelyVertical() && !IsTopToBottom() && aStartPos.X() > aClipRect.Left() ) )
                {
                    bPaintBullet = false;

                    // Why not just also call when stripping portions? This will give the correct values
                    // and needs no position corrections in OutlinerEditEng::DrawingText which tries to call
                    // PaintBullet correctly; exactly what GetEditEnginePtr()->PaintingFirstLine
                    // does, too. No change for not-layouting (painting).
                    if(0 == nLine) // && !bStripOnly)
                    {
                        Point aLineStart(aStartPos);
                        adjustYDirectionAware(aLineStart, -nLineHeight);
                        GetEditEnginePtr()->PaintingFirstLine(n, aLineStart, aOrigin, nOrientation, rOutDev);

                        // Remember whether a bullet was painted.
                        const SfxBoolItem& rBulletState = pEditEngine->GetParaAttrib(n, EE_PARA_BULLETSTATE);
                        bPaintBullet = rBulletState.GetValue();
                    }


                    // Over the Portions of the line...

                    bool bParsingFields = false;
                    std::vector< sal_Int32 >::iterator itSubLines;

                    for ( sal_Int32 nPortion = pLine->GetStartPortion(); nPortion <= pLine->GetEndPortion(); nPortion++ )
                    {
                        DBG_ASSERT( rPortion.GetTextPortions().Count(), "Line without Textportion in Paint!" );
                        const TextPortion& rTextPortion = rPortion.GetTextPortions()[nPortion];

                        const tools::Long nPortionXOffset = GetPortionXOffset( &rPortion, pLine, nPortion );
                        setXDirectionAwareFrom(aTmpPos, aStartPos);
                        adjustXDirectionAware(aTmpPos, nPortionXOffset);
                        if (isXOverflowDirectionAware(aTmpPos, aClipRect))
                            break; // No further output in line necessary

                        switch ( rTextPortion.GetKind() )
                        {
                            case PortionKind::TEXT:
                            case PortionKind::FIELD:
                            case PortionKind::HYPHENATOR:
                            {
                                SeekCursor( rPortion.GetNode(), nIndex+1, aTmpFont, &rOutDev );

                                bool bDrawFrame = false;

                                if ( ( rTextPortion.GetKind() == PortionKind::FIELD ) && !aTmpFont.IsTransparent() &&
                                     ( GetBackgroundColor() != COL_AUTO ) && GetBackgroundColor().IsDark() &&
                                     ( IsAutoColorEnabled() && ( rOutDev.GetOutDevType() != OUTDEV_PRINTER ) ) )
                                {
                                    aTmpFont.SetTransparent( true );
                                    rOutDev.SetFillColor();
                                    rOutDev.SetLineColor( GetAutoColor() );
                                    bDrawFrame = true;
                                }

#if OSL_DEBUG_LEVEL > 2
                                // Do we really need this if statement?
                                if ( rTextPortion.GetKind() == PortionKind::HYPHENATOR )
                                {
                                    aTmpFont.SetFillColor( COL_LIGHTGRAY );
                                    aTmpFont.SetTransparent( sal_False );
                                }
                                if ( rTextPortion.IsRightToLeft()  )
                                {
                                    aTmpFont.SetFillColor( COL_LIGHTGRAY );
                                    aTmpFont.SetTransparent( sal_False );
                                }
                                else if ( GetI18NScriptType( EditPaM( rPortion.GetNode(), nIndex+1 ) ) == i18n::ScriptType::COMPLEX )
                                {
                                    aTmpFont.SetFillColor( COL_LIGHTCYAN );
                                    aTmpFont.SetTransparent( sal_False );
                                }
#endif
                                aTmpFont.SetPhysFont(rOutDev);

                                // #114278# Saving both layout mode and language (since I'm
                                // potentially changing both)
                                rOutDev.Push( vcl::PushFlags::TEXTLAYOUTMODE|vcl::PushFlags::TEXTLANGUAGE );
                                ImplInitLayoutMode(rOutDev, n, nIndex);
                                ImplInitDigitMode(rOutDev, aTmpFont.GetLanguage());

                                OUString aText;
                                sal_Int32 nTextStart = 0;
                                sal_Int32 nTextLen = 0;
                                o3tl::span<const sal_Int32> pDXArray;
                                std::vector<sal_Int32> aTmpDXArray;

                                if ( rTextPortion.GetKind() == PortionKind::TEXT )
                                {
                                    aText = rPortion.GetNode()->GetString();
                                    nTextStart = nIndex;
                                    nTextLen = rTextPortion.GetLen();
                                    pDXArray = o3tl::span(pLine->GetCharPosArray().data() + (nIndex - pLine->GetStart()),
                                                    pLine->GetCharPosArray().size() - (nIndex - pLine->GetStart()));

                                    // Paint control characters (#i55716#)
                                    /* XXX: Given that there's special handling
                                     * only for some specific characters
                                     * (U+200B ZERO WIDTH SPACE and U+2060 WORD
                                     * JOINER) it is assumed to be not relevant
                                     * for MarkUrlFields(). */
                                    if ( aStatus.MarkNonUrlFields() )
                                    {
                                        sal_Int32 nTmpIdx;
                                        const sal_Int32 nTmpEnd = nTextStart + rTextPortion.GetLen();

                                        for ( nTmpIdx = nTextStart; nTmpIdx <= nTmpEnd ; ++nTmpIdx )
                                        {
                                            const sal_Unicode cChar = ( nTmpIdx != aText.getLength() && ( nTmpIdx != nTextStart || 0 == nTextStart ) ) ?
                                                                        aText[nTmpIdx] :
                                                                        0;

                                            if ( 0x200B == cChar || 0x2060 == cChar )
                                            {
                                                tools::Long nHalfBlankWidth = aTmpFont.QuickGetTextSize( &rOutDev, " ", 0, 1 ).Width() / 2;

                                                const tools::Long nAdvanceX = ( nTmpIdx == nTmpEnd ?
                                                                         rTextPortion.GetSize().Width() :
                                                                         pDXArray[ nTmpIdx - nTextStart ] ) - nHalfBlankWidth;
                                                const tools::Long nAdvanceY = -pLine->GetMaxAscent();

                                                Point aTopLeftRectPos( aTmpPos );
                                                adjustXDirectionAware(aTopLeftRectPos, nAdvanceX);
                                                adjustYDirectionAware(aTopLeftRectPos, nAdvanceY);

                                                Point aBottomRightRectPos( aTopLeftRectPos );
                                                adjustXDirectionAware(aBottomRightRectPos, 2 * nHalfBlankWidth);
                                                adjustYDirectionAware(aBottomRightRectPos, pLine->GetHeight());

                                                rOutDev.Push( vcl::PushFlags::FILLCOLOR );
                                                rOutDev.Push( vcl::PushFlags::LINECOLOR );
                                                rOutDev.SetFillColor( COL_LIGHTGRAY );
                                                rOutDev.SetLineColor( COL_LIGHTGRAY );

                                                const tools::Rectangle aBackRect( aTopLeftRectPos, aBottomRightRectPos );
                                                rOutDev.DrawRect( aBackRect );

                                                rOutDev.Pop();
                                                rOutDev.Pop();

                                                if ( 0x200B == cChar )
                                                {
                                                    const OUString aSlash( '/' );
                                                    const short nOldEscapement = aTmpFont.GetEscapement();
                                                    const sal_uInt8 nOldPropr = aTmpFont.GetPropr();

                                                    aTmpFont.SetEscapement( -20 );
                                                    aTmpFont.SetPropr( 25 );
                                                    aTmpFont.SetPhysFont(rOutDev);

                                                    const Size aSlashSize = aTmpFont.QuickGetTextSize( &rOutDev, aSlash, 0, 1 );
                                                    Point aSlashPos( aTmpPos );
                                                    const tools::Long nAddX = nHalfBlankWidth - aSlashSize.Width() / 2;
                                                    setXDirectionAwareFrom(aSlashPos, aTopLeftRectPos);
                                                    adjustXDirectionAware(aSlashPos, nAddX);

                                                    aTmpFont.QuickDrawText( &rOutDev, aSlashPos, aSlash, 0, 1 );

                                                    aTmpFont.SetEscapement( nOldEscapement );
                                                    aTmpFont.SetPropr( nOldPropr );
                                                    aTmpFont.SetPhysFont(rOutDev);
                                                }
                                            }
                                        }
                                    }
                                }
                                else if ( rTextPortion.GetKind() == PortionKind::FIELD )
                                {
                                    const EditCharAttrib* pAttr = rPortion.GetNode()->GetCharAttribs().FindFeature(nIndex);
                                    assert( pAttr && "Field not found");
                                    DBG_ASSERT( dynamic_cast< const SvxFieldItem* >( pAttr->GetItem() ) !=  nullptr, "Field of the wrong type! ");
                                    aText = static_cast<const EditCharAttribField*>(pAttr)->GetFieldValue();
                                    nTextStart = 0;
                                    nTextLen = aText.getLength();
                                    ExtraPortionInfo *pExtraInfo = rTextPortion.GetExtraInfos();
                                    // Do not split the Fields into different lines while editing
                                    // With EditView on Overlay bStripOnly is now set for stripping to
                                    // primitives. To stay compatible in EditMode use pActiveView to detect
                                    // when we are in EditMode. For whatever reason URLs are drawn as single
                                    // line in edit mode, originally clipped against edit area (which is no
                                    // longer done in Overlay mode and allows to *read* the URL).
                                    // It would be difficult to change this due to needed adaptations in
                                    // EditEngine (look for lineBreaksList creation)
                                    if( nullptr == pActiveView && bStripOnly && !bParsingFields && pExtraInfo && !pExtraInfo->lineBreaksList.empty() )
                                    {
                                        bParsingFields = true;
                                        itSubLines = pExtraInfo->lineBreaksList.begin();
                                    }

                                    if( bParsingFields )
                                    {
                                        if( itSubLines != pExtraInfo->lineBreaksList.begin() )
                                        {
                                            // only use GetMaxAscent(), pLine->GetHeight() will not
                                            // proceed as needed (see PortionKind::TEXT above and nAdvanceY)
                                            // what will lead to a compressed look with multiple lines
                                            const sal_uInt16 nMaxAscent(pLine->GetMaxAscent());

                                            aTmpPos += MoveToNextLine(aStartPos, nMaxAscent,
                                                                      nColumn, aOrigin);
                                        }
                                        std::vector< sal_Int32 >::iterator curIt = itSubLines;
                                        ++itSubLines;
                                        if( itSubLines != pExtraInfo->lineBreaksList.end() )
                                        {
                                            nTextStart = *curIt;
                                            nTextLen = *itSubLines - nTextStart;
                                        }
                                        else
                                        {
                                            nTextStart = *curIt;
                                            nTextLen = nTextLen - nTextStart;
                                            bParsingFields = false;
                                        }
                                    }

                                    aTmpFont.SetPhysFont(*GetRefDevice());
                                    aTmpFont.QuickGetTextSize( GetRefDevice(), aText, nTextStart, nTextLen, &aTmpDXArray );
                                    pDXArray = aTmpDXArray;

                                    // add a meta file comment if we record to a metafile
                                    if( bMetafileValid )
                                    {
                                        const SvxFieldItem* pFieldItem = dynamic_cast<const SvxFieldItem*>(pAttr->GetItem());
                                        if( pFieldItem )
                                        {
                                            const SvxFieldData* pFieldData = pFieldItem->GetField();
                                            if( pFieldData )
                                                pMtf->AddAction( pFieldData->createBeginComment() );
                                        }
                                    }

                                }
                                else if ( rTextPortion.GetKind() == PortionKind::HYPHENATOR )
                                {
                                    if ( rTextPortion.GetExtraValue() )
                                        aText = OUString(rTextPortion.GetExtraValue());
                                    aText += CH_HYPH;
                                    nTextStart = 0;
                                    nTextLen = aText.getLength();

                                    // crash when accessing 0 pointer in pDXArray
                                    aTmpFont.SetPhysFont(*GetRefDevice());
                                    aTmpFont.QuickGetTextSize( GetRefDevice(), aText, 0, aText.getLength(), &aTmpDXArray );
                                    pDXArray = aTmpDXArray;
                                }

                                tools::Long nTxtWidth = rTextPortion.GetSize().Width();

                                Point aOutPos( aTmpPos );
                                Point aRedLineTmpPos = aTmpPos;
                                // In RTL portions spell markup pos should be at the start of the
                                // first chara as well. That is on the right end of the portion
                                if (rTextPortion.IsRightToLeft())
                                    aRedLineTmpPos.AdjustX(rTextPortion.GetSize().Width() );

                                if ( bStripOnly )
                                {
                                    EEngineData::WrongSpellVector aWrongSpellVector;

                                    if(GetStatus().DoOnlineSpelling() && rTextPortion.GetLen())
                                    {
                                        WrongList* pWrongs = rPortion.GetNode()->GetWrongList();

                                        if(pWrongs && !pWrongs->empty())
                                        {
                                            size_t nStart = nIndex, nEnd = 0;
                                            bool bWrong = pWrongs->NextWrong(nStart, nEnd);
                                            const size_t nMaxEnd(nIndex + rTextPortion.GetLen());

                                            while(bWrong)
                                            {
                                                if(nStart >= nMaxEnd)
                                                {
                                                    break;
                                                }

                                                if(nStart < o3tl::make_unsigned(nIndex))
                                                {
                                                    nStart = nIndex;
                                                }

                                                if(nEnd > nMaxEnd)
                                                {
                                                    nEnd = nMaxEnd;
                                                }

                                                // add to vector
                                                aWrongSpellVector.emplace_back(nStart, nEnd);

                                                // goto next index
                                                nStart = nEnd + 1;

                                                if(nEnd < nMaxEnd)
                                                {
                                                    bWrong = pWrongs->NextWrong(nStart, nEnd);
                                                }
                                                else
                                                {
                                                    bWrong = false;
                                                }
                                            }
                                        }
                                    }

                                    const SvxFieldData* pFieldData = nullptr;

                                    if(PortionKind::FIELD == rTextPortion.GetKind())
                                    {
                                        const EditCharAttrib* pAttr = rPortion.GetNode()->GetCharAttribs().FindFeature(nIndex);
                                        const SvxFieldItem* pFieldItem = dynamic_cast<const SvxFieldItem*>(pAttr->GetItem());

                                        if(pFieldItem)
                                        {
                                            pFieldData = pFieldItem->GetField();
                                        }
                                    }

                                    // support for EOC, EOW, EOS TEXT comments. To support that,
                                    // the locale is needed. With the locale and a XBreakIterator it is
                                    // possible to re-create the text marking info on primitive level
                                    const lang::Locale aLocale(GetLocale(EditPaM(rPortion.GetNode(), nIndex + 1)));

                                    // create EOL and EOP bools
                                    const bool bEndOfLine(nPortion == pLine->GetEndPortion());
                                    const bool bEndOfParagraph(bEndOfLine && nLine + 1 == nLines);

                                    // get Overline color (from ((const SvxOverlineItem*)GetItem())->GetColor() in
                                    // consequence, but also already set at rOutDev)
                                    const Color aOverlineColor(rOutDev.GetOverlineColor());

                                    // get TextLine color (from ((const SvxUnderlineItem*)GetItem())->GetColor() in
                                    // consequence, but also already set at rOutDev)
                                    const Color aTextLineColor(rOutDev.GetTextLineColor());

                                    // Unicode code points conversion according to ctl text numeral setting
                                    aText = convertDigits(aText, nTextStart, nTextLen,
                                        ImplCalcDigitLang(aTmpFont.GetLanguage()));

                                    // StripPortions() data callback
                                    GetEditEnginePtr()->DrawingText( aOutPos, aText, nTextStart, nTextLen, pDXArray,
                                        aTmpFont, n, rTextPortion.GetRightToLeftLevel(),
                                        !aWrongSpellVector.empty() ? &aWrongSpellVector : nullptr,
                                        pFieldData,
                                        bEndOfLine, bEndOfParagraph, // support for EOL/EOP TEXT comments
                                        &aLocale,
                                        aOverlineColor,
                                        aTextLineColor);

                                    // #108052# remember that EOP is written already for this ParaPortion
                                    if(bEndOfParagraph)
                                    {
                                        bEndOfParagraphWritten = true;
                                    }
                                }
                                else
                                {
                                    short nEsc = aTmpFont.GetEscapement();
                                    if ( nOrientation )
                                    {
                                        // In case of high/low do it yourself:
                                        if ( aTmpFont.GetEscapement() )
                                        {
                                            tools::Long nDiff = aTmpFont.GetFontSize().Height() * aTmpFont.GetEscapement() / 100L;
                                            adjustYDirectionAware(aOutPos, -nDiff);
                                            aRedLineTmpPos = aOutPos;
                                            aTmpFont.SetEscapement( 0 );
                                        }

                                        aOutPos = lcl_ImplCalcRotatedPos( aOutPos, aOrigin, nSin, nCos );
                                        aTmpFont.SetOrientation( aTmpFont.GetOrientation()+nOrientation );
                                        aTmpFont.SetPhysFont(rOutDev);

                                    }

                                    // Take only what begins in the visible range:
                                    // Important, because of a bug in some graphic cards
                                    // when transparent font, output when negative
                                    if ( nOrientation || ( !IsEffectivelyVertical() && ( ( aTmpPos.X() + nTxtWidth ) >= nFirstVisXPos ) )
                                            || ( IsEffectivelyVertical() && ( ( aTmpPos.Y() + nTxtWidth ) >= nFirstVisYPos ) ) )
                                    {
                                        if ( nEsc && ( aTmpFont.GetUnderline() != LINESTYLE_NONE ) )
                                        {
                                            // Paint the high/low without underline,
                                            // Display the Underline on the
                                            // base line of the original font height...
                                            // But only if there was something underlined before!
                                            bool bSpecialUnderline = false;
                                            EditCharAttrib* pPrev = rPortion.GetNode()->GetCharAttribs().FindAttrib( EE_CHAR_ESCAPEMENT, nIndex );
                                            if ( pPrev )
                                            {
                                                SvxFont aDummy;
                                                // Underscore in front?
                                                if ( pPrev->GetStart() )
                                                {
                                                    SeekCursor( rPortion.GetNode(), pPrev->GetStart(), aDummy );
                                                    if ( aDummy.GetUnderline() != LINESTYLE_NONE )
                                                        bSpecialUnderline = true;
                                                }
                                                if ( !bSpecialUnderline && ( pPrev->GetEnd() < rPortion.GetNode()->Len() ) )
                                                {
                                                    SeekCursor( rPortion.GetNode(), pPrev->GetEnd()+1, aDummy );
                                                    if ( aDummy.GetUnderline() != LINESTYLE_NONE )
                                                        bSpecialUnderline = true;
                                                }
                                            }
                                            if ( bSpecialUnderline )
                                            {
                                                Size aSz = aTmpFont.GetPhysTxtSize( &rOutDev, aText, nTextStart, nTextLen );
                                                sal_uInt8 nProp = aTmpFont.GetPropr();
                                                aTmpFont.SetEscapement( 0 );
                                                aTmpFont.SetPropr( 100 );
                                                aTmpFont.SetPhysFont(rOutDev);
                                                OUStringBuffer aBlanks(nTextLen);
                                                comphelper::string::padToLength( aBlanks, nTextLen, ' ' );
                                                Point aUnderlinePos( aOutPos );
                                                if ( nOrientation )
                                                    aUnderlinePos = lcl_ImplCalcRotatedPos( aTmpPos, aOrigin, nSin, nCos );
                                                rOutDev.DrawStretchText( aUnderlinePos, aSz.Width(), aBlanks.makeStringAndClear(), 0, nTextLen );

                                                aTmpFont.SetUnderline( LINESTYLE_NONE );
                                                if ( !nOrientation )
                                                    aTmpFont.SetEscapement( nEsc );
                                                aTmpFont.SetPropr( nProp );
                                                aTmpFont.SetPhysFont(rOutDev);
                                            }
                                        }
                                        Point aRealOutPos( aOutPos );
                                        if ( ( rTextPortion.GetKind() == PortionKind::TEXT )
                                               && rTextPortion.GetExtraInfos() && rTextPortion.GetExtraInfos()->bCompressed
                                               && rTextPortion.GetExtraInfos()->bFirstCharIsRightPunktuation )
                                        {
                                            aRealOutPos.AdjustX(rTextPortion.GetExtraInfos()->nPortionOffsetX );
                                        }

                                        // RTL portions with (#i37132#)
                                        // compressed blank should not paint this blank:
                                        if ( rTextPortion.IsRightToLeft() && nTextLen >= 2 &&
                                             pDXArray[ nTextLen - 1 ] ==
                                             pDXArray[ nTextLen - 2 ] &&
                                             ' ' == aText[nTextStart + nTextLen - 1] )
                                            --nTextLen;

                                        // output directly
                                        aTmpFont.QuickDrawText( &rOutDev, aRealOutPos, aText, nTextStart, nTextLen, pDXArray );

                                        if ( bDrawFrame )
                                        {
                                            Point aTopLeft( aTmpPos );
                                            aTopLeft.AdjustY( -(pLine->GetMaxAscent()) );
                                            if ( nOrientation )
                                                aTopLeft = lcl_ImplCalcRotatedPos( aTopLeft, aOrigin, nSin, nCos );
                                            tools::Rectangle aRect( aTopLeft, rTextPortion.GetSize() );
                                            rOutDev.DrawRect( aRect );
                                        }

                                        // PDF export:
                                        if ( pPDFExtOutDevData )
                                        {
                                            if ( rTextPortion.GetKind() == PortionKind::FIELD )
                                            {
                                                const EditCharAttrib* pAttr = rPortion.GetNode()->GetCharAttribs().FindFeature(nIndex);
                                                const SvxFieldItem* pFieldItem = dynamic_cast<const SvxFieldItem*>(pAttr->GetItem());
                                                if( pFieldItem )
                                                {
                                                    const SvxFieldData* pFieldData = pFieldItem->GetField();
                                                    if ( auto pUrlField = dynamic_cast< const SvxURLField* >( pFieldData ) )
                                                    {
                                                        Point aTopLeft( aTmpPos );
                                                        aTopLeft.AdjustY( -(pLine->GetMaxAscent()) );

                                                        tools::Rectangle aRect( aTopLeft, rTextPortion.GetSize() );
                                                        vcl::PDFExtOutDevBookmarkEntry aBookmark;
                                                        aBookmark.nLinkId = pPDFExtOutDevData->CreateLink( aRect );
                                                        aBookmark.aBookmark = pUrlField->GetURL();
                                                        std::vector< vcl::PDFExtOutDevBookmarkEntry >& rBookmarks = pPDFExtOutDevData->GetBookmarks();
                                                        rBookmarks.push_back( aBookmark );
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    const WrongList* const pWrongList = rPortion.GetNode()->GetWrongList();
                                    if ( GetStatus().DoOnlineSpelling() && pWrongList && !pWrongList->empty() && rTextPortion.GetLen() )
                                    {
                                        {//#105750# adjust LinePos for superscript or subscript text
                                            short _nEsc = aTmpFont.GetEscapement();
                                            if( _nEsc )
                                            {
                                                tools::Long nShift = (_nEsc * aTmpFont.GetFontSize().Height()) / 100L;
                                                adjustYDirectionAware(aRedLineTmpPos, -nShift);
                                            }
                                        }
                                        Color aOldColor( rOutDev.GetLineColor() );
                                        rOutDev.SetLineColor( GetColorConfig().GetColorValue( svtools::SPELL ).nColor );
                                        lcl_DrawRedLines( rOutDev, aTmpFont.GetFontSize().Height(), aRedLineTmpPos, static_cast<size_t>(nIndex), static_cast<size_t>(nIndex) + rTextPortion.GetLen(), pDXArray, rPortion.GetNode()->GetWrongList(), nOrientation, aOrigin, IsEffectivelyVertical(), rTextPortion.IsRightToLeft() );
                                        rOutDev.SetLineColor( aOldColor );
                                    }
                                }

                                rOutDev.Pop();

                                if ( rTextPortion.GetKind() == PortionKind::FIELD )
                                {
                                    // add a meta file comment if we record to a metafile
                                    if( bMetafileValid )
                                    {
                                        const EditCharAttrib* pAttr = rPortion.GetNode()->GetCharAttribs().FindFeature(nIndex);
                                        assert( pAttr && "Field not found" );

                                        const SvxFieldItem* pFieldItem = dynamic_cast<const SvxFieldItem*>(pAttr->GetItem());
                                        DBG_ASSERT( pFieldItem !=  nullptr, "Wrong type of field!" );

                                        if( pFieldItem )
                                        {
                                            const SvxFieldData* pFieldData = pFieldItem->GetField();
                                            if( pFieldData )
                                                pMtf->AddAction( SvxFieldData::createEndComment() );
                                        }
                                    }

                                }

                            }
                            break;
                            case PortionKind::TAB:
                            {
                                if ( rTextPortion.GetExtraValue() && ( rTextPortion.GetExtraValue() != ' ' ) )
                                {
                                    SeekCursor( rPortion.GetNode(), nIndex+1, aTmpFont, &rOutDev );
                                    aTmpFont.SetTransparent( false );
                                    aTmpFont.SetEscapement( 0 );
                                    aTmpFont.SetPhysFont(rOutDev);
                                    tools::Long nCharWidth = aTmpFont.QuickGetTextSize( &rOutDev,
                                        OUString(rTextPortion.GetExtraValue()), 0, 1 ).Width();
                                    sal_Int32 nChars = 2;
                                    if( nCharWidth )
                                        nChars = rTextPortion.GetSize().Width() / nCharWidth;
                                    if ( nChars < 2 )
                                        nChars = 2; // is compressed by DrawStretchText.
                                    else if ( nChars == 2 )
                                        nChars = 3; // looks better

                                    OUStringBuffer aBuf(nChars);
                                    comphelper::string::padToLength(aBuf, nChars, rTextPortion.GetExtraValue());
                                    OUString aText(aBuf.makeStringAndClear());
                                    aTmpFont.QuickDrawText( &rOutDev, aTmpPos, aText, 0, aText.getLength() );
                                    rOutDev.DrawStretchText( aTmpPos, rTextPortion.GetSize().Width(), aText );

                                    if ( bStripOnly )
                                    {
                                        // create EOL and EOP bools
                                        const bool bEndOfLine(nPortion == pLine->GetEndPortion());
                                        const bool bEndOfParagraph(bEndOfLine && nLine + 1 == nLines);

                                        const Color aOverlineColor(rOutDev.GetOverlineColor());
                                        const Color aTextLineColor(rOutDev.GetTextLineColor());

                                        // StripPortions() data callback
                                        GetEditEnginePtr()->DrawingTab( aTmpPos,
                                            rTextPortion.GetSize().Width(),
                                            OUString(rTextPortion.GetExtraValue()),
                                            aTmpFont, n, rTextPortion.GetRightToLeftLevel(),
                                            bEndOfLine, bEndOfParagraph,
                                            aOverlineColor, aTextLineColor);
                                    }
                                }
                                else if ( bStripOnly )
                                {
                                    // #i108052# When stripping, a callback for _empty_ paragraphs is also needed.
                                    // This was optimized away (by not rendering the space-only tab portion), so do
                                    // it manually here.
                                    const bool bEndOfLine(nPortion == pLine->GetEndPortion());
                                    const bool bEndOfParagraph(bEndOfLine && nLine + 1 == nLines);

                                    const Color aOverlineColor(rOutDev.GetOverlineColor());
                                    const Color aTextLineColor(rOutDev.GetTextLineColor());

                                    GetEditEnginePtr()->DrawingText(
                                        aTmpPos, OUString(), 0, 0, {},
                                        aTmpFont, n, 0,
                                        nullptr,
                                        nullptr,
                                        bEndOfLine, bEndOfParagraph,
                                        nullptr,
                                        aOverlineColor,
                                        aTextLineColor);
                                }
                            }
                            break;
                            case PortionKind::LINEBREAK: break;
                        }
                        if( bParsingFields )
                            nPortion--;
                        else
                            nIndex = nIndex + rTextPortion.GetLen();

                    }
                }

                if ( ( nLine != nLastLine ) && !aStatus.IsOutliner() )
                {
                    adjustYDirectionAware(aStartPos, nSBL);
                }

                // no more visible actions?
                if (getYOverflowDirectionAware(aStartPos, aClipRect))
                    break;
            }

            if ( !aStatus.IsOutliner() )
            {
                const SvxULSpaceItem& rULItem = rPortion.GetNode()->GetContentAttribs().GetItem( EE_PARA_ULSPACE );
                tools::Long nUL = GetYValue( rULItem.GetLower() );
                adjustYDirectionAware(aStartPos, nUL);
            }

            // #108052# Safer way for #i108052# and #i118881#: If for the current ParaPortion
            // EOP is not written, do it now. This will be safer than before. It has shown
            // that the reason for #i108052# was fixed/removed again, so this is a try to fix
            // the number of paragraphs (and counting empty ones) now independent from the
            // changes in EditEngine behaviour.
            if(!bEndOfParagraphWritten && !bPaintBullet && bStripOnly)
            {
                const Color aOverlineColor(rOutDev.GetOverlineColor());
                const Color aTextLineColor(rOutDev.GetTextLineColor());

                GetEditEnginePtr()->DrawingText(
                    aTmpPos, OUString(), 0, 0, {},
                    aTmpFont, n, 0,
                    nullptr,
                    nullptr,
                    false, true, // support for EOL/EOP TEXT comments
                    nullptr,
                    aOverlineColor,
                    aTextLineColor);
            }
        }
        else
        {
            adjustYDirectionAware(aStartPos, nParaHeight);
        }

        if ( pPDFExtOutDevData )
            pPDFExtOutDevData->EndStructureElement();

        // no more visible actions?
        if (getYOverflowDirectionAware(aStartPos, aClipRect))
            break;
    }
}

void ImpEditEngine::Paint( ImpEditView* pView, const tools::Rectangle& rRect, OutputDevice* pTargetDevice )
{
    if ( !IsUpdateLayout() || IsInUndo() )
        return;

    assert( pView && "No View - No Paint!" );

    // Intersection of paint area and output area.
    tools::Rectangle aClipRect( pView->GetOutputArea() );
    aClipRect.Intersection( rRect );

    OutputDevice& rTarget = pTargetDevice ? *pTargetDevice : *pView->GetWindow()->GetOutDev();

    Point aStartPos;
    if ( !IsEffectivelyVertical() )
        aStartPos = pView->GetOutputArea().TopLeft();
    else
    {
        if( IsTopToBottom() )
            aStartPos = pView->GetOutputArea().TopRight();
        else
            aStartPos = pView->GetOutputArea().BottomLeft();
    }
    adjustXDirectionAware(aStartPos, -(pView->GetVisDocLeft()));
    adjustYDirectionAware(aStartPos, -(pView->GetVisDocTop()));

    // If Doc-width < Output Area,Width and not wrapped fields,
    // the fields usually protrude if > line.
    // (Not at the top, since there the Doc-width from formatting is already
    // there)
    if ( !IsEffectivelyVertical() && ( pView->GetOutputArea().GetWidth() > GetPaperSize().Width() ) )
    {
        tools::Long nMaxX = pView->GetOutputArea().Left() + GetPaperSize().Width();
        if ( aClipRect.Left() > nMaxX )
            return;
        if ( aClipRect.Right() > nMaxX )
            aClipRect.SetRight( nMaxX );
    }

    bool bClipRegion = rTarget.IsClipRegion();
    vcl::Region aOldRegion = rTarget.GetClipRegion();
    rTarget.IntersectClipRegion( aClipRect );

    Paint(rTarget, aClipRect, aStartPos);

    if ( bClipRegion )
        rTarget.SetClipRegion( aOldRegion );
    else
        rTarget.SetClipRegion();

    pView->DrawSelectionXOR(pView->GetEditSelection(), nullptr, &rTarget);
}

void ImpEditEngine::InsertContent( ContentNode* pNode, sal_Int32 nPos )
{
    DBG_ASSERT( pNode, "NULL-Pointer in InsertContent! " );
    DBG_ASSERT( IsInUndo(), "InsertContent only for Undo()!" );
    GetParaPortions().Insert(nPos, ParaPortion( pNode ));
    aEditDoc.Insert(nPos, pNode);
    if ( IsCallParaInsertedOrDeleted() )
        GetEditEnginePtr()->ParagraphInserted( nPos );
}

EditPaM ImpEditEngine::SplitContent( sal_Int32 nNode, sal_Int32 nSepPos )
{
    ContentNode* pNode = aEditDoc.GetObject( nNode );
    DBG_ASSERT( pNode, "Invalid Node in SplitContent" );
    DBG_ASSERT( IsInUndo(), "SplitContent only for Undo()!" );
    DBG_ASSERT( nSepPos <= pNode->Len(), "Index out of range: SplitContent" );
    EditPaM aPaM( pNode, nSepPos );
    return ImpInsertParaBreak( aPaM );
}

EditPaM ImpEditEngine::ConnectContents( sal_Int32 nLeftNode, bool bBackward )
{
    ContentNode* pLeftNode = aEditDoc.GetObject( nLeftNode );
    ContentNode* pRightNode = aEditDoc.GetObject( nLeftNode+1 );
    DBG_ASSERT( pLeftNode, "Invalid left node in ConnectContents ");
    DBG_ASSERT( pRightNode, "Invalid right node in ConnectContents ");
    return ImpConnectParagraphs( pLeftNode, pRightNode, bBackward );
}

bool ImpEditEngine::SetUpdateLayout( bool bUp, EditView* pCurView, bool bForceUpdate )
{
    const bool bPrevUpdateLayout = bUpdateLayout;
    const bool bChanged = (bUpdateLayout != bUp);

    // When switching from true to false, all selections were visible,
    // => paint over
    // the other hand, were all invisible => paint
    // If !bFormatted, e.g. after SetText, then if UpdateMode=true
    // formatting is not needed immediately, probably because more text is coming.
    // At latest it is formatted at a Paint/CalcTextWidth.
    bUpdateLayout = bUp;
    if ( bUpdateLayout && ( bChanged || bForceUpdate ) )
        FormatAndLayout( pCurView );
    return bPrevUpdateLayout;
}

void ImpEditEngine::ShowParagraph( sal_Int32 nParagraph, bool bShow )
{
    ParaPortion* pPPortion = GetParaPortions().SafeGetObject( nParagraph );
    DBG_ASSERT( pPPortion, "ShowParagraph: Paragraph does not exist! ");
    if ( !(pPPortion && ( pPPortion->IsVisible() != bShow )) )
        return;

    pPPortion->SetVisible( bShow );

    if ( !bShow )
    {
        // Mark as deleted, so that no selection will end or begin at
        // this paragraph...
        aDeletedNodes.push_back(std::make_unique<DeletedNodeInfo>( pPPortion->GetNode(), nParagraph ));
        UpdateSelections();
        // The region below will not be invalidated if UpdateMode = sal_False!
        // If anyway, then save as sal_False before SetVisible !
    }

    if ( bShow && ( pPPortion->IsInvalid() || !pPPortion->nHeight ) )
    {
        if ( !GetTextRanger() )
        {
            if ( pPPortion->IsInvalid() )
            {
                CreateLines( nParagraph, 0 );   // 0: No TextRanger
            }
            else
            {
                CalcHeight( pPPortion );
            }
            nCurTextHeight += pPPortion->GetHeight();
        }
        else
        {
            nCurTextHeight = 0x7fffffff;
        }
    }

    pPPortion->SetMustRepaint( true );
    if ( IsUpdateLayout() && !IsInUndo() && !GetTextRanger() )
    {
        aInvalidRect = tools::Rectangle(    Point( 0, GetParaPortions().GetYOffset( pPPortion ) ),
                                    Point( GetPaperSize().Width(), nCurTextHeight ) );
        UpdateViews( GetActiveView() );
    }
}

EditSelection ImpEditEngine::MoveParagraphs( Range aOldPositions, sal_Int32 nNewPos, EditView* pCurView )
{
    DBG_ASSERT( GetParaPortions().Count() != 0, "No paragraphs found: MoveParagraphs" );
    if ( GetParaPortions().Count() == 0 )
        return EditSelection();
    aOldPositions.Justify();

    EditSelection aSel( ImpMoveParagraphs( aOldPositions, nNewPos ) );

    if ( nNewPos >= GetParaPortions().Count() )
        nNewPos = GetParaPortions().Count() - 1;

    // Where the paragraph was inserted it has to be properly redrawn:
    // Where the paragraph was removed it has to be properly redrawn:
    // ( and correspondingly in between as well...)
    if ( pCurView && IsUpdateLayout() )
    {
        // in this case one can redraw directly without invalidating the
        // Portions
        sal_Int32 nFirstPortion = std::min( static_cast<sal_Int32>(aOldPositions.Min()), nNewPos );
        sal_Int32 nLastPortion = std::max( static_cast<sal_Int32>(aOldPositions.Max()), nNewPos );

        ParaPortion* pUpperPortion = GetParaPortions().SafeGetObject( nFirstPortion );
        ParaPortion* pLowerPortion = GetParaPortions().SafeGetObject( nLastPortion );
        if (pUpperPortion && pLowerPortion)
        {
            aInvalidRect = tools::Rectangle();  // make empty
            aInvalidRect.SetLeft( 0 );
            aInvalidRect.SetRight(GetColumnWidth(aPaperSize));
            aInvalidRect.SetTop( GetParaPortions().GetYOffset( pUpperPortion ) );
            aInvalidRect.SetBottom( GetParaPortions().GetYOffset( pLowerPortion ) + pLowerPortion->GetHeight() );

            UpdateViews( pCurView );
        }
    }
    else
    {
        // redraw from the upper invalid position
        sal_Int32 nFirstInvPara = std::min( static_cast<sal_Int32>(aOldPositions.Min()), nNewPos );
        InvalidateFromParagraph( nFirstInvPara );
    }
    return aSel;
}

void ImpEditEngine::InvalidateFromParagraph( sal_Int32 nFirstInvPara )
{
    // The following paragraphs are not invalidated, since ResetHeight()
    // => size change => all the following are re-issued anyway.
    if ( nFirstInvPara != 0 )
    {
        ParaPortion& rTmpPortion = GetParaPortions()[nFirstInvPara-1];
        rTmpPortion.MarkInvalid( rTmpPortion.GetNode()->Len(), 0 );
        rTmpPortion.ResetHeight();
    }
    else
    {
        ParaPortion& rTmpPortion = GetParaPortions()[0];
        rTmpPortion.MarkSelectionInvalid( 0 );
        rTmpPortion.ResetHeight();
    }
}

IMPL_LINK_NOARG(ImpEditEngine, StatusTimerHdl, Timer *, void)
{
    CallStatusHdl();
}

void ImpEditEngine::CallStatusHdl()
{
    if ( aStatusHdlLink.IsSet() && bool(aStatus.GetStatusWord()) )
    {
        // The Status has to be reset before the Call,
        // since other Flags might be set in the handler...
        EditStatus aTmpStatus( aStatus );
        aStatus.Clear();
        aStatusHdlLink.Call( aTmpStatus );
        aStatusTimer.Stop();    // If called by hand...
    }
}

ContentNode* ImpEditEngine::GetPrevVisNode( ContentNode const * pCurNode )
{
    const ParaPortion& rPortion1 = FindParaPortion( pCurNode );
    const ParaPortion* pPortion2 = GetPrevVisPortion( &rPortion1 );
    if ( pPortion2 )
        return pPortion2->GetNode();
    return nullptr;
}

ContentNode* ImpEditEngine::GetNextVisNode( ContentNode const * pCurNode )
{
    const ParaPortion& rPortion = FindParaPortion( pCurNode );
    const ParaPortion* pPortion = GetNextVisPortion( &rPortion );
    if ( pPortion )
        return pPortion->GetNode();
    return nullptr;
}

const ParaPortion* ImpEditEngine::GetPrevVisPortion( const ParaPortion* pCurPortion ) const
{
    sal_Int32 nPara = GetParaPortions().GetPos( pCurPortion );
    const ParaPortion* pPortion = nPara ? &GetParaPortions()[--nPara] : nullptr;
    while ( pPortion && !pPortion->IsVisible() )
        pPortion = nPara ? &GetParaPortions()[--nPara] : nullptr;

    return pPortion;
}

const ParaPortion* ImpEditEngine::GetNextVisPortion( const ParaPortion* pCurPortion ) const
{
    sal_Int32 nPara = GetParaPortions().GetPos( pCurPortion );
    DBG_ASSERT( nPara < GetParaPortions().Count() , "Portion not found: GetPrevVisNode" );
    const ParaPortion* pPortion = GetParaPortions().SafeGetObject( ++nPara );
    while ( pPortion && !pPortion->IsVisible() )
        pPortion = GetParaPortions().SafeGetObject( ++nPara );

    return pPortion;
}

tools::Long ImpEditEngine::CalcVertLineSpacing(Point& rStartPos) const
{
    tools::Long nTotalOccupiedHeight = 0;
    sal_Int32 nTotalLineCount = 0;
    const ParaPortionList& rParaPortions = GetParaPortions();
    sal_Int32 nParaCount = rParaPortions.Count();

    for (sal_Int32 i = 0; i < nParaCount; ++i)
    {
        if (GetVerJustification(i) != SvxCellVerJustify::Block)
            // All paragraphs must have the block justification set.
            return 0;

        const ParaPortion* pPortion = &rParaPortions[i];
        nTotalOccupiedHeight += pPortion->GetFirstLineOffset();

        const SvxLineSpacingItem& rLSItem = pPortion->GetNode()->GetContentAttribs().GetItem(EE_PARA_SBL);
        sal_uInt16 nSBL = ( rLSItem.GetInterLineSpaceRule() == SvxInterLineSpaceRule::Fix )
                            ? GetYValue( rLSItem.GetInterLineSpace() ) : 0;

        const SvxULSpaceItem& rULItem = pPortion->GetNode()->GetContentAttribs().GetItem(EE_PARA_ULSPACE);
        tools::Long nUL = GetYValue( rULItem.GetLower() );

        const EditLineList& rLines = pPortion->GetLines();
        sal_Int32 nLineCount = rLines.Count();
        nTotalLineCount += nLineCount;
        for (sal_Int32 j = 0; j < nLineCount; ++j)
        {
            const EditLine& rLine = rLines[j];
            nTotalOccupiedHeight += rLine.GetHeight();
            if (j < nLineCount-1)
                nTotalOccupiedHeight += nSBL;
            nTotalOccupiedHeight += nUL;
        }
    }

    tools::Long nTotalSpace = getHeightDirectionAware(aPaperSize);
    nTotalSpace -= nTotalOccupiedHeight;
    if (nTotalSpace <= 0 || nTotalLineCount <= 1)
        return 0;

    // Shift the text to the right for the asian layout mode.
    if (IsEffectivelyVertical())
        adjustYDirectionAware(rStartPos, -nTotalSpace);

    return nTotalSpace / (nTotalLineCount-1);
}

EditPaM ImpEditEngine::InsertParagraph( sal_Int32 nPara )
{
    EditPaM aPaM;
    if ( nPara != 0 )
    {
        ContentNode* pNode = GetEditDoc().GetObject( nPara-1 );
        if ( !pNode )
            pNode = GetEditDoc().GetObject( GetEditDoc().Count() - 1 );
        assert(pNode && "Not a single paragraph in InsertParagraph ?");
        aPaM = EditPaM( pNode, pNode->Len() );
    }
    else
    {
        ContentNode* pNode = GetEditDoc().GetObject( 0 );
        aPaM = EditPaM( pNode, 0 );
    }

    return ImpInsertParaBreak( aPaM );
}

std::optional<EditSelection> ImpEditEngine::SelectParagraph( sal_Int32 nPara )
{
    std::optional<EditSelection> pSel;
    ContentNode* pNode = GetEditDoc().GetObject( nPara );
    SAL_WARN_IF( !pNode, "editeng", "Paragraph does not exist: SelectParagraph" );
    if ( pNode )
        pSel.emplace( EditPaM( pNode, 0 ), EditPaM( pNode, pNode->Len() ) );

    return pSel;
}

void ImpEditEngine::FormatAndLayout( EditView* pCurView, bool bCalledFromUndo )
{
    if ( bDowning )
        return ;

    if ( IsInUndo() )
        IdleFormatAndLayout( pCurView );
    else
    {
        if (bCalledFromUndo)
            // in order to make bullet points that have had their styles changed, redraw themselves
            for ( sal_Int32 nPortion = 0; nPortion < GetParaPortions().Count(); nPortion++ )
                GetParaPortions()[nPortion].MarkInvalid( 0, 0 );
        FormatDoc();
        UpdateViews( pCurView );
    }

    EENotify aNotify(EE_NOTIFY_PROCESSNOTIFICATIONS);
    GetNotifyHdl().Call(aNotify);
}

void ImpEditEngine::SetFlatMode( bool bFlat )
{
    if ( bFlat != aStatus.UseCharAttribs() )
        return;

    if ( !bFlat )
        aStatus.TurnOnFlags( EEControlBits::USECHARATTRIBS );
    else
        aStatus.TurnOffFlags( EEControlBits::USECHARATTRIBS );

    aEditDoc.CreateDefFont( !bFlat );

    FormatFullDoc();
    UpdateViews();
    if ( pActiveView )
        pActiveView->ShowCursor();
}

void ImpEditEngine::SetCharStretching( sal_uInt16 nX, sal_uInt16 nY )
{
    bool bChanged;
    if ( !IsEffectivelyVertical() )
    {
        bChanged = nStretchX!=nX || nStretchY!=nY;
        nStretchX = nX;
        nStretchY = nY;
    }
    else
    {
        bChanged = nStretchX!=nY || nStretchY!=nX;
        nStretchX = nY;
        nStretchY = nX;
    }

    if (bChanged && aStatus.DoStretch())
    {
        FormatFullDoc();
        // (potentially) need everything redrawn
        aInvalidRect=tools::Rectangle(0,0,1000000,1000000);
        UpdateViews( GetActiveView() );
    }
}

const SvxNumberFormat* ImpEditEngine::GetNumberFormat( const ContentNode *pNode ) const
{
    const SvxNumberFormat *pRes = nullptr;

    if (pNode)
    {
        // get index of paragraph
        sal_Int32 nPara = GetEditDoc().GetPos( pNode );
        DBG_ASSERT( nPara < EE_PARA_NOT_FOUND, "node not found in array" );
        if (nPara < EE_PARA_NOT_FOUND)
        {
            // the called function may be overridden by an OutlinerEditEng
            // object to provide
            // access to the SvxNumberFormat of the Outliner.
            // The EditEngine implementation will just return 0.
            pRes = pEditEngine->GetNumberFormat( nPara );
        }
    }

    return pRes;
}

sal_Int32 ImpEditEngine::GetSpaceBeforeAndMinLabelWidth(
    const ContentNode *pNode,
    sal_Int32 *pnSpaceBefore, sal_Int32 *pnMinLabelWidth ) const
{
    // nSpaceBefore     matches the ODF attribute text:space-before
    // nMinLabelWidth   matches the ODF attribute text:min-label-width

    const SvxNumberFormat *pNumFmt = GetNumberFormat( pNode );

    // if no number format was found we have no Outliner or the numbering level
    // within the Outliner is -1 which means no number format should be applied.
    // Thus the default values to be returned are 0.
    sal_Int32 nSpaceBefore   = 0;
    sal_Int32 nMinLabelWidth = 0;

    if (pNumFmt)
    {
        nMinLabelWidth = -pNumFmt->GetFirstLineOffset();
        nSpaceBefore   = pNumFmt->GetAbsLSpace() - nMinLabelWidth;
        DBG_ASSERT( nMinLabelWidth >= 0, "ImpEditEngine::GetSpaceBeforeAndMinLabelWidth: min-label-width < 0 encountered" );
    }
    if (pnSpaceBefore)
        *pnSpaceBefore      = nSpaceBefore;
    if (pnMinLabelWidth)
        *pnMinLabelWidth    = nMinLabelWidth;

    return nSpaceBefore + nMinLabelWidth;
}

const SvxLRSpaceItem& ImpEditEngine::GetLRSpaceItem( ContentNode* pNode )
{
    return pNode->GetContentAttribs().GetItem( aStatus.IsOutliner() ? EE_PARA_OUTLLRSPACE : EE_PARA_LRSPACE );
}

// select a representative text language for the digit type according to the
// text numeral setting:
LanguageType ImpEditEngine::ImplCalcDigitLang(LanguageType eCurLang) const
{
    if (utl::ConfigManager::IsFuzzing())
        return LANGUAGE_ENGLISH_US;

    // #114278# Also setting up digit language from Svt options
    // (cannot reliably inherit the outdev's setting)
    if( !pCTLOptions )
        pCTLOptions.reset( new SvtCTLOptions );

    LanguageType eLang = eCurLang;
    const SvtCTLOptions::TextNumerals nCTLTextNumerals = pCTLOptions->GetCTLTextNumerals();

    if ( SvtCTLOptions::NUMERALS_HINDI == nCTLTextNumerals )
        eLang = LANGUAGE_ARABIC_SAUDI_ARABIA;
    else if ( SvtCTLOptions::NUMERALS_ARABIC == nCTLTextNumerals )
        eLang = LANGUAGE_ENGLISH;
    else if ( SvtCTLOptions::NUMERALS_SYSTEM == nCTLTextNumerals )
        eLang = Application::GetSettings().GetLanguageTag().getLanguageType();

    return eLang;
}

OUString ImpEditEngine::convertDigits(std::u16string_view rString, sal_Int32 nStt, sal_Int32 nLen, LanguageType eDigitLang)
{
    OUStringBuffer aBuf(rString);
    for (sal_Int32 nIdx = nStt, nEnd = nStt + nLen; nIdx < nEnd; ++nIdx)
    {
        sal_Unicode cChar = aBuf[nIdx];
        if (cChar >= '0' && cChar <= '9')
            aBuf[nIdx] = GetLocalizedChar(cChar, eDigitLang);
    }
    return aBuf.makeStringAndClear();
}

// Either sets the digit mode at the output device
void ImpEditEngine::ImplInitDigitMode(OutputDevice& rOutDev, LanguageType eCurLang)
{
    rOutDev.SetDigitLanguage(ImplCalcDigitLang(eCurLang));
}

void ImpEditEngine::ImplInitLayoutMode(OutputDevice& rOutDev, sal_Int32 nPara, sal_Int32 nIndex)
{
    bool bCTL = false;
    bool bR2L = false;
    if ( nIndex == -1 )
    {
        bCTL = HasScriptType( nPara, i18n::ScriptType::COMPLEX );
        bR2L = IsRightToLeft( nPara );
    }
    else
    {
        ContentNode* pNode = GetEditDoc().GetObject( nPara );
        short nScriptType = GetI18NScriptType( EditPaM( pNode, nIndex+1 ) );
        bCTL = nScriptType == i18n::ScriptType::COMPLEX;
        // this change was discussed in issue 37190
        bR2L = (GetRightToLeft( nPara, nIndex + 1) % 2) != 0;
        // it also works for issue 55927
    }

    vcl::text::ComplexTextLayoutFlags nLayoutMode = rOutDev.GetLayoutMode();

    // We always use the left position for DrawText()
    nLayoutMode &= ~vcl::text::ComplexTextLayoutFlags::BiDiRtl;

    if ( !bCTL && !bR2L)
    {
        // No Bidi checking necessary
        nLayoutMode |= vcl::text::ComplexTextLayoutFlags::BiDiStrong;
    }
    else
    {
        // Bidi checking necessary
        // Don't use BIDI_STRONG, VCL must do some checks.
        nLayoutMode &= ~vcl::text::ComplexTextLayoutFlags::BiDiStrong;

        if ( bR2L )
            nLayoutMode |= vcl::text::ComplexTextLayoutFlags::BiDiRtl|vcl::text::ComplexTextLayoutFlags::TextOriginLeft;
    }

    rOutDev.SetLayoutMode( nLayoutMode );

    // #114278# Also setting up digit language from Svt options
    // (cannot reliably inherit the outdev's setting)
    LanguageType eLang = Application::GetSettings().GetLanguageTag().getLanguageType();
    ImplInitDigitMode(rOutDev, eLang);
}

Reference < i18n::XBreakIterator > const & ImpEditEngine::ImplGetBreakIterator() const
{
    if ( !xBI.is() )
    {
        Reference< uno::XComponentContext > xContext( ::comphelper::getProcessComponentContext() );
        xBI = i18n::BreakIterator::create( xContext );
    }
    return xBI;
}

Reference < i18n::XExtendedInputSequenceChecker > const & ImpEditEngine::ImplGetInputSequenceChecker() const
{
    if ( !xISC.is() )
    {
        Reference< uno::XComponentContext > xContext( ::comphelper::getProcessComponentContext() );
        xISC = i18n::InputSequenceChecker::create( xContext );
    }
    return xISC;
}

Color ImpEditEngine::GetAutoColor() const
{
    Color aColor = GetColorConfig().GetColorValue(svtools::FONTCOLOR).nColor;

    if ( GetBackgroundColor() != COL_AUTO )
    {
        if ( GetBackgroundColor().IsDark() && aColor.IsDark() )
            aColor = COL_WHITE;
        else if ( GetBackgroundColor().IsBright() && aColor.IsBright() )
            aColor = COL_BLACK;
    }

    return aColor;
}

bool ImpEditEngine::ImplCalcAsianCompression(ContentNode* pNode,
                                             TextPortion* pTextPortion, sal_Int32 nStartPos,
                                             sal_Int32* pDXArray, sal_uInt16 n100thPercentFromMax,
                                             bool bManipulateDXArray)
{
    DBG_ASSERT( GetAsianCompressionMode() != CharCompressType::NONE, "ImplCalcAsianCompression - Why?" );
    DBG_ASSERT( pTextPortion->GetLen(), "ImplCalcAsianCompression - Empty Portion?" );

    // Percent is 1/100 Percent...
    if ( n100thPercentFromMax == 10000 )
        pTextPortion->SetExtraInfos( nullptr );

    bool bCompressed = false;

    if ( GetI18NScriptType( EditPaM( pNode, nStartPos+1 ) ) == i18n::ScriptType::ASIAN )
    {
        tools::Long nNewPortionWidth = pTextPortion->GetSize().Width();
        sal_Int32 nPortionLen = pTextPortion->GetLen();
        for ( sal_Int32 n = 0; n < nPortionLen; n++ )
        {
            AsianCompressionFlags nType = GetCharTypeForCompression( pNode->GetChar( n+nStartPos ) );

            bool bCompressPunctuation = ( nType == AsianCompressionFlags::PunctuationLeft ) || ( nType == AsianCompressionFlags::PunctuationRight );
            bool bCompressKana = ( nType == AsianCompressionFlags::Kana ) && ( GetAsianCompressionMode() == CharCompressType::PunctuationAndKana );

            // create Extra infos only if needed...
            if ( bCompressPunctuation || bCompressKana )
            {
                if ( !pTextPortion->GetExtraInfos() )
                {
                    ExtraPortionInfo* pExtraInfos = new ExtraPortionInfo;
                    pTextPortion->SetExtraInfos( pExtraInfos );
                    pExtraInfos->nOrgWidth = pTextPortion->GetSize().Width();
                    pExtraInfos->nAsianCompressionTypes = AsianCompressionFlags::Normal;
                }
                pTextPortion->GetExtraInfos()->nMaxCompression100thPercent = n100thPercentFromMax;
                pTextPortion->GetExtraInfos()->nAsianCompressionTypes |= nType;

                tools::Long nOldCharWidth;
                if ( (n+1) < nPortionLen )
                {
                    nOldCharWidth = pDXArray[n];
                }
                else
                {
                    if ( bManipulateDXArray )
                        nOldCharWidth = nNewPortionWidth - pTextPortion->GetExtraInfos()->nPortionOffsetX;
                    else
                        nOldCharWidth = pTextPortion->GetExtraInfos()->nOrgWidth;
                }
                nOldCharWidth -= ( n ? pDXArray[n-1] : 0 );

                tools::Long nCompress = 0;

                if ( bCompressPunctuation )
                {
                    nCompress = nOldCharWidth / 2;
                }
                else // Kana
                {
                    nCompress = nOldCharWidth / 10;
                }

                if ( n100thPercentFromMax != 10000 )
                {
                    nCompress *= n100thPercentFromMax;
                    nCompress /= 10000;
                }

                if ( nCompress )
                {
                    bCompressed = true;
                    nNewPortionWidth -= nCompress;
                    pTextPortion->GetExtraInfos()->bCompressed = true;


                    // Special handling for rightpunctuation: For the 'compression' we must
                    // start the output before the normal char position...
                    if ( bManipulateDXArray && ( pTextPortion->GetLen() > 1 ) )
                    {
                        if ( !pTextPortion->GetExtraInfos()->pOrgDXArray )
                            pTextPortion->GetExtraInfos()->SaveOrgDXArray( pDXArray, pTextPortion->GetLen()-1 );

                        if ( nType == AsianCompressionFlags::PunctuationRight )
                        {
                            // If it's the first char, I must handle it in Paint()...
                            if ( n )
                            {
                                // -1: No entry for the last character
                                for ( sal_Int32 i = n-1; i < (nPortionLen-1); i++ )
                                    pDXArray[i] -= nCompress;
                            }
                            else
                            {
                                pTextPortion->GetExtraInfos()->bFirstCharIsRightPunktuation = true;
                                pTextPortion->GetExtraInfos()->nPortionOffsetX = -nCompress;
                            }
                        }
                        else
                        {
                            // -1: No entry for the last character
                            for ( sal_Int32 i = n; i < (nPortionLen-1); i++ )
                                pDXArray[i] -= nCompress;
                        }
                    }
                }
            }
        }

        if ( bCompressed && ( n100thPercentFromMax == 10000 ) )
            pTextPortion->GetExtraInfos()->nWidthFullCompression = nNewPortionWidth;

        pTextPortion->GetSize().setWidth( nNewPortionWidth );

        if ( pTextPortion->GetExtraInfos() && ( n100thPercentFromMax != 10000 ) )
        {
            // Maybe rounding errors in nNewPortionWidth, assure that width not bigger than expected
            tools::Long nShrink = pTextPortion->GetExtraInfos()->nOrgWidth - pTextPortion->GetExtraInfos()->nWidthFullCompression;
            nShrink *= n100thPercentFromMax;
            nShrink /= 10000;
            tools::Long nNewWidth = pTextPortion->GetExtraInfos()->nOrgWidth - nShrink;
            if ( nNewWidth < pTextPortion->GetSize().Width() )
                pTextPortion->GetSize().setWidth( nNewWidth );
        }
    }
    return bCompressed;
}


void ImpEditEngine::ImplExpandCompressedPortions( EditLine* pLine, ParaPortion* pParaPortion, tools::Long nRemainingWidth )
{
    bool bFoundCompressedPortion = false;
    tools::Long nCompressed = 0;
    std::vector<TextPortion*> aCompressedPortions;

    sal_Int32 nPortion = pLine->GetEndPortion();
    TextPortion* pTP = &pParaPortion->GetTextPortions()[ nPortion ];
    while ( pTP && ( pTP->GetKind() == PortionKind::TEXT ) )
    {
        if ( pTP->GetExtraInfos() && pTP->GetExtraInfos()->bCompressed )
        {
            bFoundCompressedPortion = true;
            nCompressed += pTP->GetExtraInfos()->nOrgWidth - pTP->GetSize().Width();
            aCompressedPortions.push_back(pTP);
        }
        pTP = ( nPortion > pLine->GetStartPortion() ) ? &pParaPortion->GetTextPortions()[ --nPortion ] : nullptr;
    }

    if ( !bFoundCompressedPortion )
        return;

    tools::Long nCompressPercent = 0;
    if ( nCompressed > nRemainingWidth )
    {
        nCompressPercent = nCompressed - nRemainingWidth;
        DBG_ASSERT( nCompressPercent < 200000, "ImplExpandCompressedPortions - Overflow!" );
        nCompressPercent *= 10000;
        nCompressPercent /= nCompressed;
    }

    for (TextPortion* pTP2 : aCompressedPortions)
    {
        pTP = pTP2;
        pTP->GetExtraInfos()->bCompressed = false;
        pTP->GetSize().setWidth( pTP->GetExtraInfos()->nOrgWidth );
        if ( nCompressPercent )
        {
            sal_Int32 nTxtPortion = pParaPortion->GetTextPortions().GetPos( pTP );
            sal_Int32 nTxtPortionStart = pParaPortion->GetTextPortions().GetStartPos( nTxtPortion );
            DBG_ASSERT( nTxtPortionStart >= pLine->GetStart(), "Portion doesn't belong to the line!!!" );
            sal_Int32* pDXArray = pLine->GetCharPosArray().data() + (nTxtPortionStart - pLine->GetStart());
            if ( pTP->GetExtraInfos()->pOrgDXArray )
                memcpy( pDXArray, pTP->GetExtraInfos()->pOrgDXArray.get(), (pTP->GetLen()-1)*sizeof(sal_Int32) );
            ImplCalcAsianCompression( pParaPortion->GetNode(), pTP, nTxtPortionStart, pDXArray, static_cast<sal_uInt16>(nCompressPercent), true );
        }
    }
}

void ImpEditEngine::ImplUpdateOverflowingParaNum(tools::Long nPaperHeight)
{
    tools::Long nY = 0;
    tools::Long nPH;

    for ( sal_Int32 nPara = 0; nPara < GetParaPortions().Count(); nPara++ ) {
        ParaPortion& rPara = GetParaPortions()[nPara];
        nPH = rPara.GetHeight();
        nY += nPH;
        if ( nY > nPaperHeight /*nCurTextHeight*/ ) // found first paragraph overflowing
        {
            mnOverflowingPara = nPara;
            SAL_INFO("editeng.chaining", "[CHAINING] Setting first overflowing #Para#: " << nPara);
            ImplUpdateOverflowingLineNum( nPaperHeight, nPara, nY-nPH);
            return;
        }
    }
}

void ImpEditEngine::ImplUpdateOverflowingLineNum(tools::Long nPaperHeight,
                                             sal_uInt32 nOverflowingPara,
                                             tools::Long nHeightBeforeOverflowingPara)
{
    tools::Long nY = nHeightBeforeOverflowingPara;
    tools::Long nLH;

    ParaPortion& rPara = GetParaPortions()[nOverflowingPara];

    // Like UpdateOverflowingParaNum but for each line in the first
    //  overflowing paragraph.
    for ( sal_Int32 nLine = 0; nLine < rPara.GetLines().Count(); nLine++ ) {
        // XXX: We must use a reference here because the copy constructor resets the height
        EditLine &aLine = rPara.GetLines()[nLine];
        nLH = aLine.GetHeight();
        nY += nLH;

        // Debugging output
        if (nLine == 0) {
            SAL_INFO("editeng.chaining", "[CHAINING] First line has height " << nLH);
        }

        if ( nY > nPaperHeight ) // found first line overflowing
        {
            mnOverflowingLine = nLine;
            SAL_INFO("editeng.chaining", "[CHAINING] Setting first overflowing -Line- to: " << nLine);
            return;
        }
    }

    assert(false && "You should never get here");
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
