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

#include <memory>
#include <utility>
#include <scitems.hxx>
#include <editeng/adjustitem.hxx>
#include <editeng/boxitem.hxx>
#include <editeng/lineitem.hxx>
#include <editeng/brushitem.hxx>
#include <editeng/charreliefitem.hxx>
#include <editeng/contouritem.hxx>
#include <svtools/colorcfg.hxx>
#include <editeng/colritem.hxx>
#include <editeng/crossedoutitem.hxx>
#include <editeng/eeitem.hxx>
#include <editeng/emphasismarkitem.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/forbiddenruleitem.hxx>
#include <editeng/frmdiritem.hxx>
#include <editeng/langitem.hxx>
#include <editeng/postitem.hxx>
#include <svx/rotmodit.hxx>
#include <editeng/scriptspaceitem.hxx>
#include <editeng/shaditem.hxx>
#include <editeng/shdditem.hxx>
#include <editeng/udlnitem.hxx>
#include <editeng/wghtitem.hxx>
#include <editeng/wrlmitem.hxx>
#include <editeng/justifyitem.hxx>
#include <svl/intitem.hxx>
#include <svl/numformat.hxx>
#include <svl/zforlist.hxx>
#include <vcl/outdev.hxx>
#include <tools/fract.hxx>
#include <tools/UnitConversion.hxx>
#include <osl/diagnose.h>

#include <attrib.hxx>
#include <patattr.hxx>
#include <docpool.hxx>
#include <stlsheet.hxx>
#include <stlpool.hxx>
#include <document.hxx>
#include <globstr.hrc>
#include <scresid.hxx>
#include <validat.hxx>
#include <scmod.hxx>
#include <fillinfo.hxx>
#include <boost/functional/hash.hpp>

ScPatternAttr::ScPatternAttr( SfxItemSet&& pItemSet, const OUString& rStyleName )
    :   SfxSetItem  ( ATTR_PATTERN, std::move(pItemSet) ),
        pName       ( rStyleName ),
        pStyle      ( nullptr ),
        mnKey(0)
{
}

ScPatternAttr::ScPatternAttr( SfxItemSet&& pItemSet )
    :   SfxSetItem  ( ATTR_PATTERN, std::move(pItemSet) ),
        pStyle      ( nullptr ),
        mnKey(0)
{
}

ScPatternAttr::ScPatternAttr( SfxItemPool* pItemPool )
    :   SfxSetItem  ( ATTR_PATTERN, SfxItemSetFixed<ATTR_PATTERN_START, ATTR_PATTERN_END>( *pItemPool ) ),
        pStyle      ( nullptr ),
        mnKey(0)
{
}

ScPatternAttr::ScPatternAttr( const ScPatternAttr& rPatternAttr )
    :   SfxSetItem  ( rPatternAttr ),
        pName       ( rPatternAttr.pName ),
        pStyle      ( rPatternAttr.pStyle ),
        mnKey(rPatternAttr.mnKey)
{
}

ScPatternAttr::~ScPatternAttr()
{
}

ScPatternAttr* ScPatternAttr::Clone( SfxItemPool *pPool ) const
{
    ScPatternAttr* pPattern = new ScPatternAttr( GetItemSet().CloneAsValue(true, pPool) );

    pPattern->pStyle = pStyle;
    pPattern->pName = pName;

    return pPattern;
}

static bool StrCmp( const OUString* pStr1, const OUString* pStr2 )
{
    if (pStr1 == pStr2)
        return true;
    if (pStr1 && !pStr2)
        return false;
    if (!pStr1 && pStr2)
        return false;
    return *pStr1 == *pStr2;
}

static bool EqualPatternSets( const SfxItemSet& rSet1, const SfxItemSet& rSet2 )
{
    // #i62090# The SfxItemSet in the SfxSetItem base class always has the same ranges
    // (single range from ATTR_PATTERN_START to ATTR_PATTERN_END), and the items are pooled,
    // so it's enough to compare just the pointers (Count just because it's even faster).

    if ( rSet1.Count() != rSet2.Count() )
        return false;

    SfxPoolItem const ** pItems1 = rSet1.GetItems_Impl();   // inline method of SfxItemSet
    SfxPoolItem const ** pItems2 = rSet2.GetItems_Impl();

    return ( 0 == memcmp( pItems1, pItems2, (ATTR_PATTERN_END - ATTR_PATTERN_START + 1) * sizeof(pItems1[0]) ) );
}

bool ScPatternAttr::operator==( const SfxPoolItem& rCmp ) const
{
    // #i62090# Use quick comparison between ScPatternAttr's ItemSets

    if (!SfxPoolItem::operator==(rCmp) )
        return false;
    if (!mxHashCode)
        CalcHashCode();
    auto const & rOther = static_cast<const ScPatternAttr&>(rCmp);
    if (!rOther.mxHashCode)
        rOther.CalcHashCode();
    if (*mxHashCode != *rOther.mxHashCode)
        return false;
    return EqualPatternSets( GetItemSet(), rOther.GetItemSet() ) &&
            StrCmp( GetStyleName(), rOther.GetStyleName() );
}

size_t ScPatternAttr::LookupHashCode() const
{
    if (!mxHashCode)
        CalcHashCode();
    return *mxHashCode;
}

SvxCellOrientation ScPatternAttr::GetCellOrientation( const SfxItemSet& rItemSet, const SfxItemSet* pCondSet )
{
    SvxCellOrientation eOrient = SvxCellOrientation::Standard;

    if( GetItem( ATTR_STACKED, rItemSet, pCondSet ).GetValue() )
    {
        eOrient = SvxCellOrientation::Stacked;
    }
    else
    {
        Degree100 nAngle = GetItem( ATTR_ROTATE_VALUE, rItemSet, pCondSet ).GetValue();
        if( nAngle == 9000_deg100 )
            eOrient = SvxCellOrientation::BottomUp;
        else if( nAngle == 27000_deg100 )
            eOrient = SvxCellOrientation::TopBottom;
    }

    return eOrient;
}

SvxCellOrientation ScPatternAttr::GetCellOrientation( const SfxItemSet* pCondSet ) const
{
    return GetCellOrientation( GetItemSet(), pCondSet );
}

namespace {

void getFontIDsByScriptType(SvtScriptType nScript,
sal_uInt16& nFontId, sal_uInt16& nHeightId, sal_uInt16& nWeightId, sal_uInt16& nPostureId, sal_uInt16& nLangId)
{
    if ( nScript == SvtScriptType::ASIAN )
    {
        nFontId    = ATTR_CJK_FONT;
        nHeightId  = ATTR_CJK_FONT_HEIGHT;
        nWeightId  = ATTR_CJK_FONT_WEIGHT;
        nPostureId = ATTR_CJK_FONT_POSTURE;
        nLangId    = ATTR_CJK_FONT_LANGUAGE;
    }
    else if ( nScript == SvtScriptType::COMPLEX )
    {
        nFontId    = ATTR_CTL_FONT;
        nHeightId  = ATTR_CTL_FONT_HEIGHT;
        nWeightId  = ATTR_CTL_FONT_WEIGHT;
        nPostureId = ATTR_CTL_FONT_POSTURE;
        nLangId    = ATTR_CTL_FONT_LANGUAGE;
    }
    else
    {
        nFontId    = ATTR_FONT;
        nHeightId  = ATTR_FONT_HEIGHT;
        nWeightId  = ATTR_FONT_WEIGHT;
        nPostureId = ATTR_FONT_POSTURE;
        nLangId    = ATTR_FONT_LANGUAGE;
    }
}

}

void ScPatternAttr::GetFont(
        vcl::Font& rFont, const SfxItemSet& rItemSet, ScAutoFontColorMode eAutoMode,
        const OutputDevice* pOutDev, const Fraction* pScale,
        const SfxItemSet* pCondSet, SvtScriptType nScript,
        const Color* pBackConfigColor, const Color* pTextConfigColor )
{
    // Read items

    const SvxFontItem* pFontAttr;
    sal_uInt32 nFontHeight;
    FontWeight eWeight;
    FontItalic eItalic;
    FontLineStyle eUnder;
    FontLineStyle eOver;
    bool bWordLine;
    FontStrikeout eStrike;
    bool bOutline;
    bool bShadow;
    FontEmphasisMark eEmphasis;
    FontRelief eRelief;
    Color aColor;
    LanguageType eLang;

    sal_uInt16 nFontId, nHeightId, nWeightId, nPostureId, nLangId;
    getFontIDsByScriptType(nScript, nFontId, nHeightId, nWeightId, nPostureId, nLangId);

    if ( pCondSet )
    {
        const SfxPoolItem* pItem;

        if ( pCondSet->GetItemState( nFontId, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( nFontId );
        pFontAttr = static_cast<const SvxFontItem*>(pItem);

        if ( pCondSet->GetItemState( nHeightId, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( nHeightId );
        nFontHeight = static_cast<const SvxFontHeightItem*>(pItem)->GetHeight();

        if ( pCondSet->GetItemState( nWeightId, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( nWeightId );
        eWeight = static_cast<const SvxWeightItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( nPostureId, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( nPostureId );
        eItalic = static_cast<const SvxPostureItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_UNDERLINE, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( ATTR_FONT_UNDERLINE );
        eUnder = static_cast<const SvxUnderlineItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_OVERLINE, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( ATTR_FONT_OVERLINE );
        eOver = static_cast<const SvxOverlineItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_WORDLINE, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( ATTR_FONT_WORDLINE );
        bWordLine = static_cast<const SvxWordLineModeItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_CROSSEDOUT, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( ATTR_FONT_CROSSEDOUT );
        eStrike = static_cast<const SvxCrossedOutItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_CONTOUR, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( ATTR_FONT_CONTOUR );
        bOutline = static_cast<const SvxContourItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_SHADOWED, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( ATTR_FONT_SHADOWED );
        bShadow = static_cast<const SvxShadowedItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_EMPHASISMARK, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( ATTR_FONT_EMPHASISMARK );
        eEmphasis = static_cast<const SvxEmphasisMarkItem*>(pItem)->GetEmphasisMark();

        if ( pCondSet->GetItemState( ATTR_FONT_RELIEF, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( ATTR_FONT_RELIEF );
        eRelief = static_cast<const SvxCharReliefItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_COLOR, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( ATTR_FONT_COLOR );
        aColor = static_cast<const SvxColorItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( nLangId, true, &pItem ) != SfxItemState::SET )
            pItem = &rItemSet.Get( nLangId );
        eLang = static_cast<const SvxLanguageItem*>(pItem)->GetLanguage();
    }
    else    // Everything from rItemSet
    {
        pFontAttr = &static_cast<const SvxFontItem&>(rItemSet.Get( nFontId ));
        nFontHeight = static_cast<const SvxFontHeightItem&>(
                        rItemSet.Get( nHeightId )).GetHeight();
        eWeight = static_cast<const SvxWeightItem&>(
                        rItemSet.Get( nWeightId )).GetValue();
        eItalic = static_cast<const SvxPostureItem&>(
                        rItemSet.Get( nPostureId )).GetValue();
        eUnder = rItemSet.Get( ATTR_FONT_UNDERLINE ).GetValue();
        eOver = rItemSet.Get( ATTR_FONT_OVERLINE ).GetValue();
        bWordLine = rItemSet.Get( ATTR_FONT_WORDLINE ).GetValue();
        eStrike = rItemSet.Get( ATTR_FONT_CROSSEDOUT ).GetValue();
        bOutline = rItemSet.Get( ATTR_FONT_CONTOUR ).GetValue();
        bShadow = rItemSet.Get( ATTR_FONT_SHADOWED ).GetValue();
        eEmphasis = rItemSet.Get( ATTR_FONT_EMPHASISMARK ).GetEmphasisMark();
        eRelief = rItemSet.Get( ATTR_FONT_RELIEF ).GetValue();
        aColor = rItemSet.Get( ATTR_FONT_COLOR ).GetValue();
        // for graphite language features
        eLang = static_cast<const SvxLanguageItem&>(rItemSet.Get( nLangId )).GetLanguage();
    }
    OSL_ENSURE(pFontAttr,"Oops?");

    //  Evaluate

    //  FontItem:

    if (rFont.GetFamilyName() != pFontAttr->GetFamilyName())
        rFont.SetFamilyName( pFontAttr->GetFamilyName() );
    if (rFont.GetStyleName() != pFontAttr->GetStyleName())
        rFont.SetStyleName( pFontAttr->GetStyleName() );

    rFont.SetFamily( pFontAttr->GetFamily() );
    rFont.SetCharSet( pFontAttr->GetCharSet() );
    rFont.SetPitch( pFontAttr->GetPitch() );

    rFont.SetLanguage(eLang);

    //  Size

    if ( pOutDev != nullptr )
    {
        Size aEffSize;
        Fraction aFraction( 1,1 );
        if (pScale)
            aFraction = *pScale;
        Size aSize( 0, static_cast<tools::Long>(nFontHeight) );
        MapMode aDestMode = pOutDev->GetMapMode();
        MapMode aSrcMode( MapUnit::MapTwip, Point(), aFraction, aFraction );
        if (aDestMode.GetMapUnit() == MapUnit::MapPixel && pOutDev->GetDPIX() > 0)
            aEffSize = pOutDev->LogicToPixel( aSize, aSrcMode );
        else
        {
            Fraction aFractOne(1,1);
            aDestMode.SetScaleX( aFractOne );
            aDestMode.SetScaleY( aFractOne );
            aEffSize = OutputDevice::LogicToLogic( aSize, aSrcMode, aDestMode );
        }
        rFont.SetFontSize( aEffSize );
    }
    else /* if pOutDev != NULL */
    {
        rFont.SetFontSize( Size( 0, static_cast<tools::Long>(nFontHeight) ) );
    }

    //  determine effective font color

    if ( ( aColor == COL_AUTO && eAutoMode != SC_AUTOCOL_RAW ) ||
            eAutoMode == SC_AUTOCOL_IGNOREFONT || eAutoMode == SC_AUTOCOL_IGNOREALL )
    {
        if ( eAutoMode == SC_AUTOCOL_BLACK )
            aColor = COL_BLACK;
        else
        {
            //  get background color from conditional or own set
            Color aBackColor;
            if ( pCondSet )
            {
                const SfxPoolItem* pItem;
                if ( pCondSet->GetItemState( ATTR_BACKGROUND, true, &pItem ) != SfxItemState::SET )
                    pItem = &rItemSet.Get( ATTR_BACKGROUND );
                aBackColor = static_cast<const SvxBrushItem*>(pItem)->GetColor();
            }
            else
                aBackColor = rItemSet.Get( ATTR_BACKGROUND ).GetColor();

            //  if background color attribute is transparent, use window color for brightness comparisons
            if ( aBackColor == COL_TRANSPARENT ||
                    eAutoMode == SC_AUTOCOL_IGNOREBACK || eAutoMode == SC_AUTOCOL_IGNOREALL )
            {
                if ( eAutoMode == SC_AUTOCOL_PRINT )
                    aBackColor = COL_WHITE;
                else if ( pBackConfigColor )
                {
                    // pBackConfigColor can be used to avoid repeated lookup of the configured color
                    aBackColor = *pBackConfigColor;
                }
                else
                    aBackColor = SC_MOD()->GetColorConfig().GetColorValue(svtools::DOCCOLOR).nColor;
            }

            //  get system text color for comparison
            Color aSysTextColor;
            if ( eAutoMode == SC_AUTOCOL_PRINT )
                aSysTextColor = COL_BLACK;
            else if ( pTextConfigColor )
            {
                // pTextConfigColor can be used to avoid repeated lookup of the configured color
                aSysTextColor = *pTextConfigColor;
            }
            else
                aSysTextColor = SC_MOD()->GetColorConfig().GetColorValue(svtools::FONTCOLOR).nColor;

            //  select the resulting color
            if ( aBackColor.IsDark() && aSysTextColor.IsDark() )
            {
                //  use white instead of dark on dark
                aColor = COL_WHITE;
            }
            else if ( aBackColor.IsBright() && aSysTextColor.IsBright() )
            {
                //  use black instead of bright on bright
                aColor = COL_BLACK;
            }
            else
            {
                //  use aSysTextColor (black for SC_AUTOCOL_PRINT, from style settings otherwise)
                aColor = aSysTextColor;
            }
        }
    }

    //  set font effects
    rFont.SetWeight( eWeight );
    rFont.SetItalic( eItalic );
    rFont.SetUnderline( eUnder );
    rFont.SetOverline( eOver );
    rFont.SetWordLineMode( bWordLine );
    rFont.SetStrikeout( eStrike );
    rFont.SetOutline( bOutline );
    rFont.SetShadow( bShadow );
    rFont.SetEmphasisMark( eEmphasis );
    rFont.SetRelief( eRelief );
    rFont.SetColor( aColor );
    rFont.SetTransparent( true );
}

void ScPatternAttr::GetFont(
        vcl::Font& rFont, ScAutoFontColorMode eAutoMode,
        const OutputDevice* pOutDev, const Fraction* pScale,
        const SfxItemSet* pCondSet, SvtScriptType nScript,
        const Color* pBackConfigColor, const Color* pTextConfigColor ) const
{
    GetFont( rFont, GetItemSet(), eAutoMode, pOutDev, pScale, pCondSet, nScript, pBackConfigColor, pTextConfigColor );
}

ScDxfFont ScPatternAttr::GetDxfFont(const SfxItemSet& rItemSet, SvtScriptType nScript)
{
    sal_uInt16 nFontId, nHeightId, nWeightId, nPostureId, nLangId;
    getFontIDsByScriptType(nScript, nFontId, nHeightId, nWeightId, nPostureId, nLangId);
    const SfxPoolItem* pItem;

    ScDxfFont aReturn;

    if ( rItemSet.GetItemState( nFontId, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( nFontId );
        aReturn.pFontAttr = static_cast<const SvxFontItem*>(pItem);
    }

    if ( rItemSet.GetItemState( nHeightId, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( nHeightId );
        aReturn.nFontHeight = static_cast<const SvxFontHeightItem*>(pItem)->GetHeight();
    }

    if ( rItemSet.GetItemState( nWeightId, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( nWeightId );
        aReturn.eWeight = static_cast<const SvxWeightItem*>(pItem)->GetValue();
    }

    if ( rItemSet.GetItemState( nPostureId, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( nPostureId );
        aReturn.eItalic = static_cast<const SvxPostureItem*>(pItem)->GetValue();
    }

    if ( rItemSet.GetItemState( ATTR_FONT_UNDERLINE, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( ATTR_FONT_UNDERLINE );
        aReturn.eUnder = static_cast<const SvxUnderlineItem*>(pItem)->GetValue();
    }

    if ( rItemSet.GetItemState( ATTR_FONT_OVERLINE, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( ATTR_FONT_OVERLINE );
        aReturn.eOver = static_cast<const SvxOverlineItem*>(pItem)->GetValue();
    }

    if ( rItemSet.GetItemState( ATTR_FONT_WORDLINE, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( ATTR_FONT_WORDLINE );
        aReturn.bWordLine = static_cast<const SvxWordLineModeItem*>(pItem)->GetValue();
    }

    if ( rItemSet.GetItemState( ATTR_FONT_CROSSEDOUT, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( ATTR_FONT_CROSSEDOUT );
        aReturn.eStrike = static_cast<const SvxCrossedOutItem*>(pItem)->GetValue();
    }

    if ( rItemSet.GetItemState( ATTR_FONT_CONTOUR, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( ATTR_FONT_CONTOUR );
        aReturn.bOutline = static_cast<const SvxContourItem*>(pItem)->GetValue();
    }

    if ( rItemSet.GetItemState( ATTR_FONT_SHADOWED, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( ATTR_FONT_SHADOWED );
        aReturn.bShadow = static_cast<const SvxShadowedItem*>(pItem)->GetValue();
    }

    if ( rItemSet.GetItemState( ATTR_FONT_EMPHASISMARK, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( ATTR_FONT_EMPHASISMARK );
        aReturn.eEmphasis = static_cast<const SvxEmphasisMarkItem*>(pItem)->GetEmphasisMark();
    }

    if ( rItemSet.GetItemState( ATTR_FONT_RELIEF, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( ATTR_FONT_RELIEF );
        aReturn.eRelief = static_cast<const SvxCharReliefItem*>(pItem)->GetValue();
    }

    if ( rItemSet.GetItemState( ATTR_FONT_COLOR, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( ATTR_FONT_COLOR );
        aReturn.aColor = static_cast<const SvxColorItem*>(pItem)->GetValue();
    }

    if ( rItemSet.GetItemState( nLangId, true, &pItem ) == SfxItemState::SET )
    {
        pItem = &rItemSet.Get( nLangId );
        aReturn.eLang = static_cast<const SvxLanguageItem*>(pItem)->GetLanguage();
    }

    return aReturn;
}

void ScPatternAttr::FillToEditItemSet( SfxItemSet& rEditSet, const SfxItemSet& rSrcSet, const SfxItemSet* pCondSet )
{
    //  Read Items

    std::unique_ptr<SvxColorItem> aColorItem(std::make_unique<SvxColorItem>(EE_CHAR_COLOR));              // use item as-is
    std::unique_ptr<SvxFontItem> aFontItem(std::make_unique<SvxFontItem>(EE_CHAR_FONTINFO));            // use item as-is
    std::unique_ptr<SvxFontItem> aCjkFontItem(std::make_unique<SvxFontItem>(EE_CHAR_FONTINFO_CJK));            // use item as-is
    std::unique_ptr<SvxFontItem> aCtlFontItem(std::make_unique<SvxFontItem>(EE_CHAR_FONTINFO_CTL));            // use item as-is
    tools::Long            nTHeight, nCjkTHeight, nCtlTHeight;     // Twips
    FontWeight      eWeight, eCjkWeight, eCtlWeight;
    std::unique_ptr<SvxUnderlineItem> aUnderlineItem(std::make_unique<SvxUnderlineItem>(LINESTYLE_NONE, EE_CHAR_UNDERLINE));
    std::unique_ptr<SvxOverlineItem> aOverlineItem(std::make_unique<SvxOverlineItem>(LINESTYLE_NONE, EE_CHAR_OVERLINE));
    bool            bWordLine;
    FontStrikeout   eStrike;
    FontItalic      eItalic, eCjkItalic, eCtlItalic;
    bool            bOutline;
    bool            bShadow;
    bool            bForbidden;
    FontEmphasisMark eEmphasis;
    FontRelief      eRelief;
    LanguageType    eLang, eCjkLang, eCtlLang;
    bool            bHyphenate;
    SvxFrameDirection eDirection;

    //TODO: additional parameter to control if language is needed?

    if ( pCondSet )
    {
        const SfxPoolItem* pItem;

        if ( pCondSet->GetItemState( ATTR_FONT_COLOR, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_COLOR );
        aColorItem.reset(&pItem->Clone()->StaticWhichCast(ATTR_FONT_COLOR));

        if ( pCondSet->GetItemState( ATTR_FONT, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT );
        aFontItem.reset(&pItem->Clone()->StaticWhichCast(ATTR_FONT));

        if ( pCondSet->GetItemState( ATTR_CJK_FONT, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_CJK_FONT );
        aCjkFontItem.reset(&pItem->Clone()->StaticWhichCast(ATTR_CJK_FONT));

        if ( pCondSet->GetItemState( ATTR_CTL_FONT, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_CTL_FONT );
        aCtlFontItem.reset(&pItem->Clone()->StaticWhichCast(ATTR_CTL_FONT));

        if ( pCondSet->GetItemState( ATTR_FONT_HEIGHT, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_HEIGHT );
        nTHeight = static_cast<const SvxFontHeightItem*>(pItem)->GetHeight();
        if ( pCondSet->GetItemState( ATTR_CJK_FONT_HEIGHT, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_CJK_FONT_HEIGHT );
        nCjkTHeight = static_cast<const SvxFontHeightItem*>(pItem)->GetHeight();
        if ( pCondSet->GetItemState( ATTR_CTL_FONT_HEIGHT, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_CTL_FONT_HEIGHT );
        nCtlTHeight = static_cast<const SvxFontHeightItem*>(pItem)->GetHeight();

        if ( pCondSet->GetItemState( ATTR_FONT_WEIGHT, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_WEIGHT );
        eWeight = static_cast<const SvxWeightItem*>(pItem)->GetValue();
        if ( pCondSet->GetItemState( ATTR_CJK_FONT_WEIGHT, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_CJK_FONT_WEIGHT );
        eCjkWeight = static_cast<const SvxWeightItem*>(pItem)->GetValue();
        if ( pCondSet->GetItemState( ATTR_CTL_FONT_WEIGHT, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_CTL_FONT_WEIGHT );
        eCtlWeight = static_cast<const SvxWeightItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_POSTURE, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_POSTURE );
        eItalic = static_cast<const SvxPostureItem*>(pItem)->GetValue();
        if ( pCondSet->GetItemState( ATTR_CJK_FONT_POSTURE, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_CJK_FONT_POSTURE );
        eCjkItalic = static_cast<const SvxPostureItem*>(pItem)->GetValue();
        if ( pCondSet->GetItemState( ATTR_CTL_FONT_POSTURE, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_CTL_FONT_POSTURE );
        eCtlItalic = static_cast<const SvxPostureItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_UNDERLINE, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_UNDERLINE );
        aUnderlineItem.reset(&pItem->Clone()->StaticWhichCast(ATTR_FONT_UNDERLINE));

        if ( pCondSet->GetItemState( ATTR_FONT_OVERLINE, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_OVERLINE );
        aOverlineItem.reset(&pItem->Clone()->StaticWhichCast(ATTR_FONT_OVERLINE));

        if ( pCondSet->GetItemState( ATTR_FONT_WORDLINE, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_WORDLINE );
        bWordLine = static_cast<const SvxWordLineModeItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_CROSSEDOUT, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_CROSSEDOUT );
        eStrike = static_cast<const SvxCrossedOutItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_CONTOUR, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_CONTOUR );
        bOutline = static_cast<const SvxContourItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_SHADOWED, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_SHADOWED );
        bShadow = static_cast<const SvxShadowedItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FORBIDDEN_RULES, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FORBIDDEN_RULES );
        bForbidden = static_cast<const SvxForbiddenRuleItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_EMPHASISMARK, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_EMPHASISMARK );
        eEmphasis = static_cast<const SvxEmphasisMarkItem*>(pItem)->GetEmphasisMark();
        if ( pCondSet->GetItemState( ATTR_FONT_RELIEF, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_RELIEF );
        eRelief = static_cast<const SvxCharReliefItem*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_FONT_LANGUAGE, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_FONT_LANGUAGE );
        eLang = static_cast<const SvxLanguageItem*>(pItem)->GetLanguage();
        if ( pCondSet->GetItemState( ATTR_CJK_FONT_LANGUAGE, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_CJK_FONT_LANGUAGE );
        eCjkLang = static_cast<const SvxLanguageItem*>(pItem)->GetLanguage();
        if ( pCondSet->GetItemState( ATTR_CTL_FONT_LANGUAGE, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_CTL_FONT_LANGUAGE );
        eCtlLang = static_cast<const SvxLanguageItem*>(pItem)->GetLanguage();

        if ( pCondSet->GetItemState( ATTR_HYPHENATE, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_HYPHENATE );
        bHyphenate = static_cast<const ScHyphenateCell*>(pItem)->GetValue();

        if ( pCondSet->GetItemState( ATTR_WRITINGDIR, true, &pItem ) != SfxItemState::SET )
            pItem = &rSrcSet.Get( ATTR_WRITINGDIR );
        eDirection = static_cast<const SvxFrameDirectionItem*>(pItem)->GetValue();
    }
    else        // Everything directly from Pattern
    {
        aColorItem.reset(rSrcSet.Get(ATTR_FONT_COLOR).Clone());
        aFontItem.reset(rSrcSet.Get(ATTR_FONT).Clone());
        aCjkFontItem.reset(rSrcSet.Get(ATTR_CJK_FONT).Clone());
        aCtlFontItem.reset(rSrcSet.Get(ATTR_CTL_FONT).Clone());
        nTHeight = rSrcSet.Get( ATTR_FONT_HEIGHT ).GetHeight();
        nCjkTHeight = rSrcSet.Get( ATTR_CJK_FONT_HEIGHT ).GetHeight();
        nCtlTHeight = rSrcSet.Get( ATTR_CTL_FONT_HEIGHT ).GetHeight();
        eWeight = rSrcSet.Get( ATTR_FONT_WEIGHT ).GetValue();
        eCjkWeight = rSrcSet.Get( ATTR_CJK_FONT_WEIGHT ).GetValue();
        eCtlWeight = rSrcSet.Get( ATTR_CTL_FONT_WEIGHT ).GetValue();
        eItalic = rSrcSet.Get( ATTR_FONT_POSTURE ).GetValue();
        eCjkItalic = rSrcSet.Get( ATTR_CJK_FONT_POSTURE ).GetValue();
        eCtlItalic = rSrcSet.Get( ATTR_CTL_FONT_POSTURE ).GetValue();
        aUnderlineItem.reset(rSrcSet.Get(ATTR_FONT_UNDERLINE).Clone());
        aOverlineItem.reset(rSrcSet.Get(ATTR_FONT_OVERLINE).Clone());
        bWordLine = rSrcSet.Get( ATTR_FONT_WORDLINE ).GetValue();
        eStrike = rSrcSet.Get( ATTR_FONT_CROSSEDOUT ).GetValue();
        bOutline = rSrcSet.Get( ATTR_FONT_CONTOUR ).GetValue();
        bShadow = rSrcSet.Get( ATTR_FONT_SHADOWED ).GetValue();
        bForbidden = rSrcSet.Get( ATTR_FORBIDDEN_RULES ).GetValue();
        eEmphasis = rSrcSet.Get( ATTR_FONT_EMPHASISMARK ).GetEmphasisMark();
        eRelief = rSrcSet.Get( ATTR_FONT_RELIEF ).GetValue();
        eLang = rSrcSet.Get( ATTR_FONT_LANGUAGE ).GetLanguage();
        eCjkLang = rSrcSet.Get( ATTR_CJK_FONT_LANGUAGE ).GetLanguage();
        eCtlLang = rSrcSet.Get( ATTR_CTL_FONT_LANGUAGE ).GetLanguage();
        bHyphenate = rSrcSet.Get( ATTR_HYPHENATE ).GetValue();
        eDirection = rSrcSet.Get( ATTR_WRITINGDIR ).GetValue();
    }

    // Expect to be compatible to LogicToLogic, ie. 2540/1440 = 127/72, and round

    tools::Long nHeight = convertTwipToMm100(nTHeight);
    tools::Long nCjkHeight = convertTwipToMm100(nCjkTHeight);
    tools::Long nCtlHeight = convertTwipToMm100(nCtlTHeight);

    //  put items into EditEngine ItemSet

    if ( aColorItem->GetValue() == COL_AUTO )
    {
        //  When cell attributes are converted to EditEngine paragraph attributes,
        //  don't create a hard item for automatic color, because that would be converted
        //  to black when the item's Store method is used in CreateTransferable/WriteBin.
        //  COL_AUTO is the EditEngine's pool default, so ClearItem will result in automatic
        //  color, too, without having to store the item.
        rEditSet.ClearItem( EE_CHAR_COLOR );
    }
    else
    {
        // tdf#125054 adapt WhichID
        rEditSet.Put( *aColorItem, EE_CHAR_COLOR );
    }

    // tdf#125054 adapt WhichID
    rEditSet.Put( *aFontItem, EE_CHAR_FONTINFO );
    rEditSet.Put( *aCjkFontItem, EE_CHAR_FONTINFO_CJK );
    rEditSet.Put( *aCtlFontItem, EE_CHAR_FONTINFO_CTL );

    rEditSet.Put( SvxFontHeightItem( nHeight, 100, EE_CHAR_FONTHEIGHT ) );
    rEditSet.Put( SvxFontHeightItem( nCjkHeight, 100, EE_CHAR_FONTHEIGHT_CJK ) );
    rEditSet.Put( SvxFontHeightItem( nCtlHeight, 100, EE_CHAR_FONTHEIGHT_CTL ) );
    rEditSet.Put( SvxWeightItem ( eWeight,      EE_CHAR_WEIGHT ) );
    rEditSet.Put( SvxWeightItem ( eCjkWeight,   EE_CHAR_WEIGHT_CJK ) );
    rEditSet.Put( SvxWeightItem ( eCtlWeight,   EE_CHAR_WEIGHT_CTL ) );

    // tdf#125054 adapt WhichID
    rEditSet.Put( *aUnderlineItem, EE_CHAR_UNDERLINE );
    rEditSet.Put( *aOverlineItem, EE_CHAR_OVERLINE );

    rEditSet.Put( SvxWordLineModeItem( bWordLine,   EE_CHAR_WLM ) );
    rEditSet.Put( SvxCrossedOutItem( eStrike,       EE_CHAR_STRIKEOUT ) );
    rEditSet.Put( SvxPostureItem    ( eItalic,      EE_CHAR_ITALIC ) );
    rEditSet.Put( SvxPostureItem    ( eCjkItalic,   EE_CHAR_ITALIC_CJK ) );
    rEditSet.Put( SvxPostureItem    ( eCtlItalic,   EE_CHAR_ITALIC_CTL ) );
    rEditSet.Put( SvxContourItem    ( bOutline,     EE_CHAR_OUTLINE ) );
    rEditSet.Put( SvxShadowedItem   ( bShadow,      EE_CHAR_SHADOW ) );
    rEditSet.Put( SvxForbiddenRuleItem(bForbidden, EE_PARA_FORBIDDENRULES) );
    rEditSet.Put( SvxEmphasisMarkItem( eEmphasis,   EE_CHAR_EMPHASISMARK ) );
    rEditSet.Put( SvxCharReliefItem( eRelief,       EE_CHAR_RELIEF ) );
    rEditSet.Put( SvxLanguageItem   ( eLang,        EE_CHAR_LANGUAGE ) );
    rEditSet.Put( SvxLanguageItem   ( eCjkLang,     EE_CHAR_LANGUAGE_CJK ) );
    rEditSet.Put( SvxLanguageItem   ( eCtlLang,     EE_CHAR_LANGUAGE_CTL ) );
    rEditSet.Put( SfxBoolItem       ( EE_PARA_HYPHENATE, bHyphenate ) );
    rEditSet.Put( SvxFrameDirectionItem( eDirection, EE_PARA_WRITINGDIR ) );

    // Script spacing is always off.
    // The cell attribute isn't used here as long as there is no UI to set it
    // (don't evaluate attributes that can't be changed).
    // If a locale-dependent default is needed, it has to go into the cell
    // style, like the fonts.
    rEditSet.Put( SvxScriptSpaceItem( false, EE_PARA_ASIANCJKSPACING ) );
}

void ScPatternAttr::FillEditItemSet( SfxItemSet* pEditSet, const SfxItemSet* pCondSet ) const
{
    if( pEditSet )
        FillToEditItemSet( *pEditSet, GetItemSet(), pCondSet );
}

void ScPatternAttr::GetFromEditItemSet( SfxItemSet& rDestSet, const SfxItemSet& rEditSet )
{
    const SfxPoolItem* pItem;

    if (rEditSet.GetItemState(EE_CHAR_COLOR,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( *static_cast<const SvxColorItem*>(pItem), ATTR_FONT_COLOR );

    if (rEditSet.GetItemState(EE_CHAR_FONTINFO,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( *static_cast<const SvxFontItem*>(pItem), ATTR_FONT );
    if (rEditSet.GetItemState(EE_CHAR_FONTINFO_CJK,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( *static_cast<const SvxFontItem*>(pItem), ATTR_CJK_FONT );
    if (rEditSet.GetItemState(EE_CHAR_FONTINFO_CTL,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( *static_cast<const SvxFontItem*>(pItem), ATTR_CTL_FONT );

    if (rEditSet.GetItemState(EE_CHAR_FONTHEIGHT,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxFontHeightItem(o3tl::toTwips(static_cast<const SvxFontHeightItem*>(pItem)->GetHeight(), o3tl::Length::mm100),
                        100, ATTR_FONT_HEIGHT ) );
    if (rEditSet.GetItemState(EE_CHAR_FONTHEIGHT_CJK,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxFontHeightItem(o3tl::toTwips(static_cast<const SvxFontHeightItem*>(pItem)->GetHeight(), o3tl::Length::mm100),
                        100, ATTR_CJK_FONT_HEIGHT ) );
    if (rEditSet.GetItemState(EE_CHAR_FONTHEIGHT_CTL,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxFontHeightItem(o3tl::toTwips(static_cast<const SvxFontHeightItem*>(pItem)->GetHeight(), o3tl::Length::mm100),
                        100, ATTR_CTL_FONT_HEIGHT ) );

    if (rEditSet.GetItemState(EE_CHAR_WEIGHT,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxWeightItem( static_cast<const SvxWeightItem*>(pItem)->GetValue(),
                        ATTR_FONT_WEIGHT) );
    if (rEditSet.GetItemState(EE_CHAR_WEIGHT_CJK,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxWeightItem( static_cast<const SvxWeightItem*>(pItem)->GetValue(),
                        ATTR_CJK_FONT_WEIGHT) );
    if (rEditSet.GetItemState(EE_CHAR_WEIGHT_CTL,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxWeightItem( static_cast<const SvxWeightItem*>(pItem)->GetValue(),
                        ATTR_CTL_FONT_WEIGHT) );

    // SvxTextLineItem contains enum and color
    if (rEditSet.GetItemState(EE_CHAR_UNDERLINE,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( *static_cast<const SvxUnderlineItem*>(pItem), ATTR_FONT_UNDERLINE );
    if (rEditSet.GetItemState(EE_CHAR_OVERLINE,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( *static_cast<const SvxOverlineItem*>(pItem), ATTR_FONT_OVERLINE );
    if (rEditSet.GetItemState(EE_CHAR_WLM,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxWordLineModeItem( static_cast<const SvxWordLineModeItem*>(pItem)->GetValue(),
                        ATTR_FONT_WORDLINE) );

    if (rEditSet.GetItemState(EE_CHAR_STRIKEOUT,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxCrossedOutItem( static_cast<const SvxCrossedOutItem*>(pItem)->GetValue(),
                        ATTR_FONT_CROSSEDOUT) );

    if (rEditSet.GetItemState(EE_CHAR_ITALIC,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxPostureItem( static_cast<const SvxPostureItem*>(pItem)->GetValue(),
                        ATTR_FONT_POSTURE) );
    if (rEditSet.GetItemState(EE_CHAR_ITALIC_CJK,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxPostureItem( static_cast<const SvxPostureItem*>(pItem)->GetValue(),
                        ATTR_CJK_FONT_POSTURE) );
    if (rEditSet.GetItemState(EE_CHAR_ITALIC_CTL,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxPostureItem( static_cast<const SvxPostureItem*>(pItem)->GetValue(),
                        ATTR_CTL_FONT_POSTURE) );

    if (rEditSet.GetItemState(EE_CHAR_OUTLINE,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxContourItem( static_cast<const SvxContourItem*>(pItem)->GetValue(),
                        ATTR_FONT_CONTOUR) );
    if (rEditSet.GetItemState(EE_CHAR_SHADOW,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxShadowedItem( static_cast<const SvxShadowedItem*>(pItem)->GetValue(),
                        ATTR_FONT_SHADOWED) );
    if (rEditSet.GetItemState(EE_CHAR_EMPHASISMARK,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxEmphasisMarkItem( static_cast<const SvxEmphasisMarkItem*>(pItem)->GetEmphasisMark(),
                        ATTR_FONT_EMPHASISMARK) );
    if (rEditSet.GetItemState(EE_CHAR_RELIEF,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxCharReliefItem( static_cast<const SvxCharReliefItem*>(pItem)->GetValue(),
                        ATTR_FONT_RELIEF) );

    if (rEditSet.GetItemState(EE_CHAR_LANGUAGE,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxLanguageItem(static_cast<const SvxLanguageItem*>(pItem)->GetValue(), ATTR_FONT_LANGUAGE) );
    if (rEditSet.GetItemState(EE_CHAR_LANGUAGE_CJK,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxLanguageItem(static_cast<const SvxLanguageItem*>(pItem)->GetValue(), ATTR_CJK_FONT_LANGUAGE) );
    if (rEditSet.GetItemState(EE_CHAR_LANGUAGE_CTL,true,&pItem) == SfxItemState::SET)
        rDestSet.Put( SvxLanguageItem(static_cast<const SvxLanguageItem*>(pItem)->GetValue(), ATTR_CTL_FONT_LANGUAGE) );

    if (rEditSet.GetItemState(EE_PARA_JUST,true,&pItem) != SfxItemState::SET)
        return;

    SvxCellHorJustify eVal;
    switch ( static_cast<const SvxAdjustItem*>(pItem)->GetAdjust() )
    {
        case SvxAdjust::Left:
            // EditEngine Default is always set in the GetAttribs() ItemSet !
            // whether left or right, is decided in text / number
            eVal = SvxCellHorJustify::Standard;
            break;
        case SvxAdjust::Right:
            eVal = SvxCellHorJustify::Right;
            break;
        case SvxAdjust::Block:
            eVal = SvxCellHorJustify::Block;
            break;
        case SvxAdjust::Center:
            eVal = SvxCellHorJustify::Center;
            break;
        case SvxAdjust::BlockLine:
            eVal = SvxCellHorJustify::Block;
            break;
        case SvxAdjust::End:
            eVal = SvxCellHorJustify::Right;
            break;
        default:
            eVal = SvxCellHorJustify::Standard;
    }
    if ( eVal != SvxCellHorJustify::Standard )
        rDestSet.Put( SvxHorJustifyItem( eVal, ATTR_HOR_JUSTIFY) );
}

void ScPatternAttr::GetFromEditItemSet( const SfxItemSet* pEditSet )
{
    if( !pEditSet )
        return;
    GetFromEditItemSet( GetItemSet(), *pEditSet );
    mxHashCode.reset();
}

void ScPatternAttr::FillEditParaItems( SfxItemSet* pEditSet ) const
{
    //  already there in GetFromEditItemSet, but not in FillEditItemSet
    //  Default horizontal alignment is always implemented as left

    const SfxItemSet& rMySet = GetItemSet();

    SvxCellHorJustify eHorJust = rMySet.Get(ATTR_HOR_JUSTIFY).GetValue();

    SvxAdjust eSvxAdjust;
    switch (eHorJust)
    {
        case SvxCellHorJustify::Right:  eSvxAdjust = SvxAdjust::Right;  break;
        case SvxCellHorJustify::Center: eSvxAdjust = SvxAdjust::Center; break;
        case SvxCellHorJustify::Block:  eSvxAdjust = SvxAdjust::Block;  break;
        default:                     eSvxAdjust = SvxAdjust::Left;   break;
    }
    pEditSet->Put( SvxAdjustItem( eSvxAdjust, EE_PARA_JUST ) );
}

void ScPatternAttr::DeleteUnchanged( const ScPatternAttr* pOldAttrs )
{
    SfxItemSet& rThisSet = GetItemSet();
    const SfxItemSet& rOldSet = pOldAttrs->GetItemSet();

    const SfxPoolItem* pThisItem;
    const SfxPoolItem* pOldItem;

    for ( sal_uInt16 nSubWhich=ATTR_PATTERN_START; nSubWhich<=ATTR_PATTERN_END; nSubWhich++ )
    {
        //  only items that are set are interesting
        if ( rThisSet.GetItemState( nSubWhich, false, &pThisItem ) == SfxItemState::SET )
        {
            SfxItemState eOldState = rOldSet.GetItemState( nSubWhich, true, &pOldItem );
            if ( eOldState == SfxItemState::SET )
            {
                //  item is set in OldAttrs (or its parent) -> compare pointers
                if ( pThisItem == pOldItem )
                {
                    rThisSet.ClearItem( nSubWhich );
                    mxHashCode.reset();
                }
            }
            else if ( eOldState != SfxItemState::DONTCARE )
            {
                //  not set in OldAttrs -> compare item value to default item
                if ( *pThisItem == rThisSet.GetPool()->GetDefaultItem( nSubWhich ) )
                {
                    rThisSet.ClearItem( nSubWhich );
                    mxHashCode.reset();
                }
            }
        }
    }
}

bool ScPatternAttr::HasItemsSet( const sal_uInt16* pWhich ) const
{
    const SfxItemSet& rSet = GetItemSet();
    for (sal_uInt16 i=0; pWhich[i]; i++)
        if ( rSet.GetItemState( pWhich[i], false ) == SfxItemState::SET )
            return true;
    return false;
}

void ScPatternAttr::ClearItems( const sal_uInt16* pWhich )
{
    SfxItemSet& rSet = GetItemSet();
    for (sal_uInt16 i=0; pWhich[i]; i++)
        rSet.ClearItem(pWhich[i]);
    mxHashCode.reset();
}

static SfxStyleSheetBase* lcl_CopyStyleToPool
    (
        SfxStyleSheetBase*      pSrcStyle,
        SfxStyleSheetBasePool*  pSrcPool,
        SfxStyleSheetBasePool*  pDestPool,
        const SvNumberFormatterIndexTable*     pFormatExchangeList
    )
{
    if ( !pSrcStyle || !pDestPool || !pSrcPool )
    {
        OSL_FAIL( "CopyStyleToPool: Invalid Arguments :-/" );
        return nullptr;
    }

    const OUString       aStrSrcStyle = pSrcStyle->GetName();
    const SfxStyleFamily eFamily      = pSrcStyle->GetFamily();
    SfxStyleSheetBase*   pDestStyle   = pDestPool->Find( aStrSrcStyle, eFamily );

    if ( !pDestStyle )
    {
        const OUString   aStrParent = pSrcStyle->GetParent();
        const SfxItemSet& rSrcSet = pSrcStyle->GetItemSet();

        pDestStyle = &pDestPool->Make( aStrSrcStyle, eFamily, SfxStyleSearchBits::UserDefined );
        SfxItemSet& rDestSet = pDestStyle->GetItemSet();
        rDestSet.Put( rSrcSet );

        // number format exchange list has to be handled here, too
        // (only called for cell styles)

        const SfxPoolItem* pSrcItem;
        if ( pFormatExchangeList &&
             rSrcSet.GetItemState( ATTR_VALUE_FORMAT, false, &pSrcItem ) == SfxItemState::SET )
        {
            sal_uLong nOldFormat = static_cast<const SfxUInt32Item*>(pSrcItem)->GetValue();
            SvNumberFormatterIndexTable::const_iterator it = pFormatExchangeList->find(nOldFormat);
            if (it != pFormatExchangeList->end())
            {
                sal_uInt32 nNewFormat = it->second;
                rDestSet.Put( SfxUInt32Item( ATTR_VALUE_FORMAT, nNewFormat ) );
            }
        }

        // if necessary create derivative Styles, if not available:

        if ( ScResId(STR_STYLENAME_STANDARD) != aStrParent &&
             aStrSrcStyle != aStrParent &&
             !pDestPool->Find( aStrParent, eFamily ) )
        {
            lcl_CopyStyleToPool( pSrcPool->Find( aStrParent, eFamily ),
                                 pSrcPool, pDestPool, pFormatExchangeList );
        }

        pDestStyle->SetParent( aStrParent );
    }

    return pDestStyle;
}

ScPatternAttr* ScPatternAttr::PutInPool( ScDocument* pDestDoc, ScDocument* pSrcDoc ) const
{
    const SfxItemSet* pSrcSet = &GetItemSet();

    ScPatternAttr aDestPattern( pDestDoc->GetPool() );
    SfxItemSet* pDestSet = &aDestPattern.GetItemSet();

    // Copy cell pattern style to other document:

    if ( pDestDoc != pSrcDoc )
    {
        OSL_ENSURE( pStyle, "Missing Pattern-Style! :-/" );

        // if pattern in DestDoc is available, use this, otherwise copy
        // parent style to style or create if necessary and attach DestDoc

        SfxStyleSheetBase* pStyleCpy = lcl_CopyStyleToPool( pStyle,
                                                            pSrcDoc->GetStyleSheetPool(),
                                                            pDestDoc->GetStyleSheetPool(),
                                                            pDestDoc->GetFormatExchangeList() );

        aDestPattern.SetStyleSheet( static_cast<ScStyleSheet*>(pStyleCpy) );
    }

    for ( sal_uInt16 nAttrId = ATTR_PATTERN_START; nAttrId <= ATTR_PATTERN_END; nAttrId++ )
    {
        const SfxPoolItem* pSrcItem;
        SfxItemState eItemState = pSrcSet->GetItemState( nAttrId, false, &pSrcItem );
        if (eItemState==SfxItemState::SET)
        {
            std::unique_ptr<SfxPoolItem> pNewItem;

            if ( nAttrId == ATTR_VALIDDATA )
            {
                // Copy validity to the new document

                sal_uLong nNewIndex = 0;
                ScValidationDataList* pSrcList = pSrcDoc->GetValidationList();
                if ( pSrcList )
                {
                    sal_uLong nOldIndex = static_cast<const SfxUInt32Item*>(pSrcItem)->GetValue();
                    const ScValidationData* pOldData = pSrcList->GetData( nOldIndex );
                    if ( pOldData )
                        nNewIndex = pDestDoc->AddValidationEntry( *pOldData );
                }
                pNewItem.reset(new SfxUInt32Item( ATTR_VALIDDATA, nNewIndex ));
            }
            else if ( nAttrId == ATTR_VALUE_FORMAT && pDestDoc->GetFormatExchangeList() )
            {
                //  Number format to Exchange List

                sal_uLong nOldFormat = static_cast<const SfxUInt32Item*>(pSrcItem)->GetValue();
                SvNumberFormatterIndexTable::const_iterator it = pDestDoc->GetFormatExchangeList()->find(nOldFormat);
                if (it != pDestDoc->GetFormatExchangeList()->end())
                {
                    sal_uInt32 nNewFormat = it->second;
                    pNewItem.reset(new SfxUInt32Item( ATTR_VALUE_FORMAT, nNewFormat ));
                }
            }

            if ( pNewItem )
            {
                pDestSet->Put(*pNewItem);
            }
            else
                pDestSet->Put(*pSrcItem);
        }
    }

    ScPatternAttr* pPatternAttr = const_cast<ScPatternAttr*>( &pDestDoc->GetPool()->Put(aDestPattern) );
    return pPatternAttr;
}

bool ScPatternAttr::IsVisible() const
{
    const SfxItemSet& rSet = GetItemSet();

    const SfxPoolItem* pItem;
    SfxItemState eState;

    eState = rSet.GetItemState( ATTR_BACKGROUND, true, &pItem );
    if ( eState == SfxItemState::SET )
        if ( static_cast<const SvxBrushItem*>(pItem)->GetColor() != COL_TRANSPARENT )
            return true;

    eState = rSet.GetItemState( ATTR_BORDER, true, &pItem );
    if ( eState == SfxItemState::SET )
    {
        const SvxBoxItem* pBoxItem = static_cast<const SvxBoxItem*>(pItem);
        if ( pBoxItem->GetTop() || pBoxItem->GetBottom() ||
             pBoxItem->GetLeft() || pBoxItem->GetRight() )
            return true;
    }

    eState = rSet.GetItemState( ATTR_BORDER_TLBR, true, &pItem );
    if ( eState == SfxItemState::SET )
        if( static_cast< const SvxLineItem* >( pItem )->GetLine() )
            return true;

    eState = rSet.GetItemState( ATTR_BORDER_BLTR, true, &pItem );
    if ( eState == SfxItemState::SET )
        if( static_cast< const SvxLineItem* >( pItem )->GetLine() )
            return true;

    eState = rSet.GetItemState( ATTR_SHADOW, true, &pItem );
    if ( eState == SfxItemState::SET )
        if ( static_cast<const SvxShadowItem*>(pItem)->GetLocation() != SvxShadowLocation::NONE )
            return true;

    return false;
}

static bool OneEqual( const SfxItemSet& rSet1, const SfxItemSet& rSet2, sal_uInt16 nId )
{
    const SfxPoolItem* pItem1 = &rSet1.Get(nId);
    const SfxPoolItem* pItem2 = &rSet2.Get(nId);
    return ( pItem1 == pItem2 || *pItem1 == *pItem2 );
}

bool ScPatternAttr::IsVisibleEqual( const ScPatternAttr& rOther ) const
{
    const SfxItemSet& rThisSet = GetItemSet();
    const SfxItemSet& rOtherSet = rOther.GetItemSet();

    return OneEqual( rThisSet, rOtherSet, ATTR_BACKGROUND ) &&
            OneEqual( rThisSet, rOtherSet, ATTR_BORDER ) &&
            OneEqual( rThisSet, rOtherSet, ATTR_BORDER_TLBR ) &&
            OneEqual( rThisSet, rOtherSet, ATTR_BORDER_BLTR ) &&
            OneEqual( rThisSet, rOtherSet, ATTR_SHADOW );

    //TODO: also here only check really visible values !!!
}

const OUString* ScPatternAttr::GetStyleName() const
{
    return pName ? &*pName : ( pStyle ? &pStyle->GetName() : nullptr );
}

void ScPatternAttr::SetStyleSheet( ScStyleSheet* pNewStyle, bool bClearDirectFormat )
{
    if (pNewStyle)
    {
        SfxItemSet&       rPatternSet = GetItemSet();
        const SfxItemSet& rStyleSet = pNewStyle->GetItemSet();

        if (bClearDirectFormat)
        {
            for (sal_uInt16 i=ATTR_PATTERN_START; i<=ATTR_PATTERN_END; i++)
            {
                if (rStyleSet.GetItemState(i) == SfxItemState::SET)
                    rPatternSet.ClearItem(i);
            }
        }
        rPatternSet.SetParent(&pNewStyle->GetItemSet());
        pStyle = pNewStyle;
        pName.reset();
    }
    else
    {
        OSL_FAIL( "ScPatternAttr::SetStyleSheet( NULL ) :-|" );
        GetItemSet().SetParent(nullptr);
        pStyle = nullptr;
    }
}

void ScPatternAttr::UpdateStyleSheet(const ScDocument& rDoc)
{
    if (pName)
    {
        pStyle = static_cast<ScStyleSheet*>(rDoc.GetStyleSheetPool()->Find(*pName, SfxStyleFamily::Para));

        //  use Standard if Style is not found,
        //  to avoid empty display in Toolbox-Controller
        //  Assumes that "Standard" is always the 1st entry!
        if (!pStyle)
        {
            std::unique_ptr<SfxStyleSheetIterator> pIter = rDoc.GetStyleSheetPool()->CreateIterator(SfxStyleFamily::Para);
            pStyle = dynamic_cast< ScStyleSheet* >(pIter->First());
        }

        if (pStyle)
        {
            GetItemSet().SetParent(&pStyle->GetItemSet());
            pName.reset();
        }
    }
    else
        pStyle = nullptr;
}

void ScPatternAttr::StyleToName()
{
    // Style was deleted, remember name:

    if ( pStyle )
    {
        pName = pStyle->GetName();
        pStyle = nullptr;
        GetItemSet().SetParent( nullptr );
    }
}

bool ScPatternAttr::IsSymbolFont() const
{
    const SfxPoolItem* pItem;
    if( GetItemSet().GetItemState( ATTR_FONT, true, &pItem ) == SfxItemState::SET )
        return static_cast<const SvxFontItem*>(pItem)->GetCharSet() == RTL_TEXTENCODING_SYMBOL;
    else
        return false;
}

namespace {

sal_uInt32 getNumberFormatKey(const SfxItemSet& rSet)
{
    return rSet.Get(ATTR_VALUE_FORMAT).GetValue();
}

LanguageType getLanguageType(const SfxItemSet& rSet)
{
    return rSet.Get(ATTR_LANGUAGE_FORMAT).GetLanguage();
}

}

sal_uInt32 ScPatternAttr::GetNumberFormat( SvNumberFormatter* pFormatter ) const
{
    sal_uInt32 nFormat = getNumberFormatKey(GetItemSet());
    LanguageType eLang = getLanguageType(GetItemSet());
    if ( nFormat < SV_COUNTRY_LANGUAGE_OFFSET && eLang == LANGUAGE_SYSTEM )
        ;       // it remains as it is
    else if ( pFormatter )
        nFormat = pFormatter->GetFormatForLanguageIfBuiltIn( nFormat, eLang );
    return nFormat;
}

// the same if conditional formatting is in play:

sal_uInt32 ScPatternAttr::GetNumberFormat( SvNumberFormatter* pFormatter,
                                           const SfxItemSet* pCondSet ) const
{
    assert(pFormatter);
    if (!pCondSet)
        return GetNumberFormat(pFormatter);

    // Conditional format takes precedence over style and even hard format.

    const SfxPoolItem* pFormItem;
    sal_uInt32 nFormat;
    const SfxPoolItem* pLangItem;
    LanguageType eLang;
    if (pCondSet->GetItemState(ATTR_VALUE_FORMAT, true, &pFormItem) == SfxItemState::SET )
    {
        nFormat = getNumberFormatKey(*pCondSet);
        if (pCondSet->GetItemState(ATTR_LANGUAGE_FORMAT, true, &pLangItem) == SfxItemState::SET)
            eLang = getLanguageType(*pCondSet);
        else
            eLang = getLanguageType(GetItemSet());
    }
    else
    {
        nFormat = getNumberFormatKey(GetItemSet());
        eLang = getLanguageType(GetItemSet());
    }

    return pFormatter->GetFormatForLanguageIfBuiltIn(nFormat, eLang);
}

const SfxPoolItem& ScPatternAttr::GetItem( sal_uInt16 nWhich, const SfxItemSet& rItemSet, const SfxItemSet* pCondSet )
{
    const SfxPoolItem* pCondItem;
    if ( pCondSet && pCondSet->GetItemState( nWhich, true, &pCondItem ) == SfxItemState::SET )
        return *pCondItem;
    return rItemSet.Get(nWhich);
}

const SfxPoolItem& ScPatternAttr::GetItem( sal_uInt16 nSubWhich, const SfxItemSet* pCondSet ) const
{
    return GetItem( nSubWhich, GetItemSet(), pCondSet );
}

//  GetRotateVal is tested before ATTR_ORIENTATION

Degree100 ScPatternAttr::GetRotateVal( const SfxItemSet* pCondSet ) const
{
    Degree100 nAttrRotate(0);
    if ( GetCellOrientation() == SvxCellOrientation::Standard )
    {
        bool bRepeat = ( GetItem(ATTR_HOR_JUSTIFY, pCondSet).
                            GetValue() == SvxCellHorJustify::Repeat );
        // ignore orientation/rotation if "repeat" is active
        if ( !bRepeat )
            nAttrRotate = GetItem( ATTR_ROTATE_VALUE, pCondSet ).GetValue();
    }
    return nAttrRotate;
}

ScRotateDir ScPatternAttr::GetRotateDir( const SfxItemSet* pCondSet ) const
{
    ScRotateDir nRet = ScRotateDir::NONE;

    Degree100 nAttrRotate = GetRotateVal( pCondSet );
    if ( nAttrRotate )
    {
        SvxRotateMode eRotMode = GetItem(ATTR_ROTATE_MODE, pCondSet).GetValue();

        if ( eRotMode == SVX_ROTATE_MODE_STANDARD || nAttrRotate == 18000_deg100 )
            nRet = ScRotateDir::Standard;
        else if ( eRotMode == SVX_ROTATE_MODE_CENTER )
            nRet = ScRotateDir::Center;
        else if ( eRotMode == SVX_ROTATE_MODE_TOP || eRotMode == SVX_ROTATE_MODE_BOTTOM )
        {
            Degree100 nRot180 = nAttrRotate % 18000_deg100;     // 1/100 degrees
            if ( nRot180 == 9000_deg100 )
                nRet = ScRotateDir::Center;
            else if ( ( eRotMode == SVX_ROTATE_MODE_TOP && nRot180 < 9000_deg100 ) ||
                      ( eRotMode == SVX_ROTATE_MODE_BOTTOM && nRot180 > 9000_deg100 ) )
                nRet = ScRotateDir::Left;
            else
                nRet = ScRotateDir::Right;
        }
    }

    return nRet;
}

void ScPatternAttr::SetKey(sal_uInt64 nKey)
{
    mnKey = nKey;
}

sal_uInt64 ScPatternAttr::GetKey() const
{
    return mnKey;
}

void ScPatternAttr::CalcHashCode() const
{
    auto const & rSet = GetItemSet();
    mxHashCode = 1; // Set up seed so that an empty pattern does not have an (invalid) hash of 0.
    boost::hash_range(*mxHashCode, rSet.GetItems_Impl(), rSet.GetItems_Impl() + rSet.Count());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
