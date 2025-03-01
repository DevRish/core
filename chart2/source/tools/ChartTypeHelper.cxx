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

#include <ChartTypeHelper.hxx>
#include <ChartType.hxx>
#include <BaseCoordinateSystem.hxx>
#include <DiagramHelper.hxx>
#include <servicenames_charttypes.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/chart/DataLabelPlacement.hpp>
#include <com/sun/star/chart/MissingValueTreatment.hpp>
#include <com/sun/star/chart2/AxisType.hpp>
#include <com/sun/star/chart2/StackingDirection.hpp>
#include <com/sun/star/chart2/XChartType.hpp>
#include <com/sun/star/chart2/XDataSeries.hpp>
#include <tools/diagnose_ex.h>

using namespace ::com::sun::star;
using namespace ::com::sun::star::chart2;

namespace chart
{

bool ChartTypeHelper::isSupportingAxisSideBySide(
    const rtl::Reference< ::chart::ChartType >& xChartType, sal_Int32 nDimensionCount )
{
    bool bResult = false;

    if( xChartType.is() &&
        nDimensionCount < 3 )
    {
        bool bFound=false;
        bool bAmbiguous=false;
        StackMode eStackMode = DiagramHelper::getStackModeFromChartType( xChartType, bFound, bAmbiguous, nullptr );
        if( eStackMode == StackMode::NONE && !bAmbiguous )
        {
            OUString aChartTypeName = xChartType->getChartType();
            bResult = ( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_COLUMN) ||
                        aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BAR) );
        }
    }

    return bResult;
}

bool ChartTypeHelper::isSupportingGeometryProperties( const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount )
{
    //form tab only for 3D-bar and 3D-column charts.

    //@todo ask charttype itself --> need model change first
    if(xChartType.is())
    {
        if(nDimensionCount==3)
        {
            OUString aChartTypeName = xChartType->getChartType();
            if( aChartTypeName == CHART2_SERVICE_NAME_CHARTTYPE_BAR )
                return true;
            if( aChartTypeName == CHART2_SERVICE_NAME_CHARTTYPE_COLUMN )
                return true;
        }
    }
    return false;
}

bool ChartTypeHelper::isSupportingStatisticProperties( const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount )
{
    //3D charts, pie, net and stock do not support statistic properties

    //@todo ask charttype itself (and series? --> stock chart?)  --> need model change first
    if(xChartType.is())
    {
        if(nDimensionCount==3)
            return false;

        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_PIE) )
            return false;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_NET) )
            return false;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_FILLED_NET) )
            return false;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_CANDLESTICK) )
            return false;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BUBBLE) ) //todo: BubbleChart support error bars and trend lines
            return false;
    }
    return true;
}

bool ChartTypeHelper::isSupportingRegressionProperties( const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount )
{
    // note: old chart: only scatter chart
    return isSupportingStatisticProperties( xChartType, nDimensionCount );
}

bool ChartTypeHelper::isSupportingAreaProperties( const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount )
{
    //2D line charts, net and stock do not support area properties

    //@todo ask charttype itself --> need model change first
    if(xChartType.is())
    {
        if(nDimensionCount==2)
        {
            OUString aChartTypeName = xChartType->getChartType();
            if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_LINE) )
                return false;
            if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_SCATTER) )
                return false;
            if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_NET) )
                return false;
            if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_CANDLESTICK) )
                return false;
        }
    }
    return true;
}

bool ChartTypeHelper::isSupportingSymbolProperties( const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount )
{
    //2D line charts, 2D scatter charts and 2D net charts do support symbols

    //@todo ask charttype itself --> need model change first
    if(xChartType.is())
    {
        if(nDimensionCount==3)
            return false;

        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_LINE) )
            return true;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_SCATTER) )
            return true;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_NET) )
            return true;
    }
    return false;
}

bool ChartTypeHelper::isSupportingMainAxis( const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount, sal_Int32 nDimensionIndex )
{
    //pie charts do not support axis at all
    //no 3rd axis for 2D charts

    //@todo ask charttype itself --> need model change first
    if(xChartType.is())
    {
        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_PIE) )
            return false;

        if( nDimensionIndex == 2 )
            return nDimensionCount == 3;
    }
    return true;
}

bool ChartTypeHelper::isSupportingSecondaryAxis( const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount )
{
    //3D, pie and net charts do not support a secondary axis at all

    //@todo ask charttype itself --> need model change first
    if(xChartType.is())
    {
        if(nDimensionCount==3)
            return false;

        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_PIE) )
            return false;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_NET) )
            return false;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_FILLED_NET) )
            return false;
    }
    return true;
}

bool ChartTypeHelper::isSupportingOverlapAndGapWidthProperties(
        const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount )
{
    //2D bar charts do support a this special properties

    //@todo ask charttype itself --> need model change first
    if(xChartType.is())
    {
        if(nDimensionCount==3)
            return false;

        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_COLUMN) )
            return true;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BAR) )
            return true;
    }
    return false;
}

bool ChartTypeHelper::isSupportingBarConnectors(
    const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount )
{
    //2D bar charts with stacked series support this

    //@todo ask charttype itself --> need model change first
    if(xChartType.is())
    {
        if(nDimensionCount==3)
            return false;

        bool bFound=false;
        bool bAmbiguous=false;
        StackMode eStackMode = DiagramHelper::getStackModeFromChartType( xChartType, bFound, bAmbiguous, nullptr );
        if( eStackMode != StackMode::YStacked || bAmbiguous )
            return false;

        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_COLUMN) )
            return true;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BAR) )
            return true;  // note: old chart was false here
    }
    return false;
}

uno::Sequence < sal_Int32 > ChartTypeHelper::getSupportedLabelPlacements( const rtl::Reference< ChartType >& xChartType
                                                                         , bool bSwapXAndY
                                                                         , const uno::Reference< chart2::XDataSeries >& xSeries )
{
    uno::Sequence < sal_Int32 > aRet;
    if( !xChartType.is() )
        return aRet;

    OUString aChartTypeName = xChartType->getChartType();
    if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_PIE) )
    {
        bool bDonut = false;
        xChartType->getPropertyValue( "UseRings") >>= bDonut;

        if(!bDonut)
        {
            aRet.realloc(5);
            sal_Int32* pSeq = aRet.getArray();
            *pSeq++ = css::chart::DataLabelPlacement::AVOID_OVERLAP;
            *pSeq++ = css::chart::DataLabelPlacement::OUTSIDE;
            *pSeq++ = css::chart::DataLabelPlacement::INSIDE;
            *pSeq++ = css::chart::DataLabelPlacement::CENTER;
            *pSeq++ = css::chart::DataLabelPlacement::CUSTOM;
        }
        else
        {
            aRet.realloc(1);
            sal_Int32* pSeq = aRet.getArray();
            *pSeq++ = css::chart::DataLabelPlacement::CENTER;
        }
    }
    else if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_SCATTER)
        || aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_LINE)
        || aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BUBBLE)
        )
    {
        aRet.realloc(5);
        sal_Int32* pSeq = aRet.getArray();
        *pSeq++ = css::chart::DataLabelPlacement::TOP;
        *pSeq++ = css::chart::DataLabelPlacement::BOTTOM;
        *pSeq++ = css::chart::DataLabelPlacement::LEFT;
        *pSeq++ = css::chart::DataLabelPlacement::RIGHT;
        *pSeq++ = css::chart::DataLabelPlacement::CENTER;
    }
    else if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_COLUMN)
        || aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BAR) )
    {

        bool bStacked = false;
        {
            uno::Reference< beans::XPropertySet > xSeriesProp( xSeries, uno::UNO_QUERY );
            chart2::StackingDirection eStacking = chart2::StackingDirection_NO_STACKING;
            xSeriesProp->getPropertyValue( "StackingDirection" ) >>= eStacking;
            bStacked = (eStacking == chart2::StackingDirection_Y_STACKING);
        }

        aRet.realloc( bStacked ? 3 : 6 );
        sal_Int32* pSeq = aRet.getArray();
        if(!bStacked)
        {
            if(bSwapXAndY)
            {
                *pSeq++ = css::chart::DataLabelPlacement::RIGHT;
                *pSeq++ = css::chart::DataLabelPlacement::LEFT;
            }
            else
            {
                *pSeq++ = css::chart::DataLabelPlacement::TOP;
                *pSeq++ = css::chart::DataLabelPlacement::BOTTOM;
            }
        }
        *pSeq++ = css::chart::DataLabelPlacement::CENTER;
        if(!bStacked)
            *pSeq++ = css::chart::DataLabelPlacement::OUTSIDE;
        *pSeq++ = css::chart::DataLabelPlacement::INSIDE;
        *pSeq++ = css::chart::DataLabelPlacement::NEAR_ORIGIN;
    }
    else if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_AREA) )
    {
        bool bStacked = false;
        {
            uno::Reference<beans::XPropertySet> xSeriesProp(xSeries, uno::UNO_QUERY);
            chart2::StackingDirection eStacking = chart2::StackingDirection_NO_STACKING;
            xSeriesProp->getPropertyValue("StackingDirection") >>= eStacking;
            bStacked = (eStacking == chart2::StackingDirection_Y_STACKING);
        }

        aRet.realloc(2);
        sal_Int32* pSeq = aRet.getArray();
        if (bStacked)
        {
            *pSeq++ = css::chart::DataLabelPlacement::CENTER;
            *pSeq++ = css::chart::DataLabelPlacement::TOP;
        }
        else
        {
            *pSeq++ = css::chart::DataLabelPlacement::TOP;
            *pSeq++ = css::chart::DataLabelPlacement::CENTER;
        }
    }
    else if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_NET) )
    {
        aRet.realloc(6);
        sal_Int32* pSeq = aRet.getArray();
        *pSeq++ = css::chart::DataLabelPlacement::OUTSIDE;
        *pSeq++ = css::chart::DataLabelPlacement::TOP;
        *pSeq++ = css::chart::DataLabelPlacement::BOTTOM;
        *pSeq++ = css::chart::DataLabelPlacement::LEFT;
        *pSeq++ = css::chart::DataLabelPlacement::RIGHT;
        *pSeq++ = css::chart::DataLabelPlacement::CENTER;
    }
    else if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_FILLED_NET) )
    {
        aRet.realloc(1);
        sal_Int32* pSeq = aRet.getArray();
        *pSeq++ = css::chart::DataLabelPlacement::OUTSIDE;
    }
    else if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_CANDLESTICK) )
    {
        aRet.realloc( 1 );
        sal_Int32* pSeq = aRet.getArray();
        *pSeq++ = css::chart::DataLabelPlacement::OUTSIDE;
    }
    else
    {
        OSL_FAIL( "unknown charttype" );
    }

    return aRet;
}

bool ChartTypeHelper::isSupportingRightAngledAxes( const rtl::Reference< ChartType >& xChartType )
{
    if(xChartType.is())
    {
        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_PIE) )
            return false;
    }
    return true;
}

bool ChartTypeHelper::isSupportingStartingAngle( const rtl::Reference< ChartType >& xChartType )
{
    if(xChartType.is())
    {
        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_PIE) )
            return true;
    }
    return false;
}
bool ChartTypeHelper::isSupportingBaseValue( const rtl::Reference< ChartType >& xChartType )
{
    if(xChartType.is())
    {
        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_COLUMN)
            || aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BAR)
            || aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_AREA)
            )
            return true;
    }
    return false;
}

bool ChartTypeHelper::isSupportingAxisPositioning( const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount, sal_Int32 nDimensionIndex )
{
    if(xChartType.is())
    {
        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_NET) )
            return false;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_FILLED_NET) )
            return false;
    }
    if( nDimensionCount==3 )
        return nDimensionIndex<2;
    return true;
}

bool ChartTypeHelper::isSupportingDateAxis( const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionIndex )
{
    if( nDimensionIndex!=0 )
        return false;
    if( xChartType.is() )
    {
        sal_Int32 nType = ChartTypeHelper::getAxisType( xChartType, nDimensionIndex );
        if( nType != AxisType::CATEGORY )
            return false;
        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_PIE) )
            return false;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_NET) )
            return false;
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_FILLED_NET) )
            return false;
    }
    return true;
}

bool ChartTypeHelper::isSupportingComplexCategory( const rtl::Reference< ChartType >& xChartType )
{
    if( xChartType.is() )
    {
        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_PIE) )
            return false;
    }
    return true;
}

bool ChartTypeHelper::isSupportingCategoryPositioning( const rtl::Reference< ChartType >& xChartType, sal_Int32 nDimensionCount )
{
    if( xChartType.is() )
    {
        OUString aChartTypeName = xChartType->getChartType();
        if (aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_AREA) ||
            aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_LINE) ||
            aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_CANDLESTICK))
            return true;
        else if (nDimensionCount == 2 &&
                (aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_COLUMN) || aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BAR)))
            return true;
    }
    return false;
}

bool ChartTypeHelper::shiftCategoryPosAtXAxisPerDefault( const rtl::Reference< ChartType >& xChartType )
{
    if(xChartType.is())
    {
        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_COLUMN)
            || aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BAR)
            || aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_CANDLESTICK) )
            return true;
    }
    return false;
}

bool ChartTypeHelper::noBordersForSimpleScheme( const rtl::Reference< ChartType >& xChartType )
{
    if(xChartType.is())
    {
        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_PIE) )
            return true;
    }
    return false;
}

sal_Int32 ChartTypeHelper::getDefaultDirectLightColor( bool bSimple, const rtl::Reference< ChartType >& xChartType )
{
    sal_Int32 nRet = static_cast< sal_Int32 >( 0x808080 ); // grey
    if( xChartType .is() )
    {
        OUString aChartType = xChartType->getChartType();
        if( aChartType == CHART2_SERVICE_NAME_CHARTTYPE_PIE )
        {
            if( bSimple )
                nRet = static_cast< sal_Int32 >( 0x333333 ); // grey80
            else
                nRet = static_cast< sal_Int32 >( 0xb3b3b3 ); // grey30
        }
        else if( aChartType == CHART2_SERVICE_NAME_CHARTTYPE_LINE
            || aChartType == CHART2_SERVICE_NAME_CHARTTYPE_SCATTER )
            nRet = static_cast< sal_Int32 >( 0x666666 ); // grey60
    }
    return nRet;
}

sal_Int32 ChartTypeHelper::getDefaultAmbientLightColor( bool bSimple, const rtl::Reference< ChartType >& xChartType )
{
    sal_Int32 nRet = static_cast< sal_Int32 >( 0x999999 ); // grey40
    if( xChartType .is() )
    {
        OUString aChartType = xChartType->getChartType();
        if( aChartType == CHART2_SERVICE_NAME_CHARTTYPE_PIE )
        {
            if( bSimple )
                nRet = static_cast< sal_Int32 >( 0xcccccc ); // grey20
            else
                nRet = static_cast< sal_Int32 >( 0x666666 ); // grey60
        }
    }
    return nRet;
}

drawing::Direction3D ChartTypeHelper::getDefaultSimpleLightDirection( const rtl::Reference< ChartType >& xChartType )
{
    drawing::Direction3D aRet(0.0, 0.0, 1.0);
    if( xChartType .is() )
    {
        OUString aChartType = xChartType->getChartType();
        if( aChartType == CHART2_SERVICE_NAME_CHARTTYPE_PIE )
            aRet = drawing::Direction3D(0.0, 0.8, 0.5);
        else if( aChartType == CHART2_SERVICE_NAME_CHARTTYPE_LINE
            || aChartType == CHART2_SERVICE_NAME_CHARTTYPE_SCATTER )
            aRet = drawing::Direction3D(0.9, 0.5, 0.05);
    }
    return aRet;
}

drawing::Direction3D ChartTypeHelper::getDefaultRealisticLightDirection( const rtl::Reference< ChartType >& xChartType )
{
    drawing::Direction3D aRet(0.0, 0.0, 1.0);
    if( xChartType .is() )
    {
        OUString aChartType = xChartType->getChartType();
        if( aChartType == CHART2_SERVICE_NAME_CHARTTYPE_PIE )
            aRet = drawing::Direction3D(0.6, 0.6, 0.6);
        else if( aChartType == CHART2_SERVICE_NAME_CHARTTYPE_LINE
            || aChartType == CHART2_SERVICE_NAME_CHARTTYPE_SCATTER )
            aRet = drawing::Direction3D(0.9, 0.5, 0.05);
    }
    return aRet;
}

sal_Int32 ChartTypeHelper::getAxisType( const rtl::Reference<
            ChartType >& xChartType, sal_Int32 nDimensionIndex )
{
    //returned is a constant from constant group css::chart2::AxisType

    //@todo ask charttype itself --> need model change first
    if(!xChartType.is())
        return AxisType::CATEGORY;

    OUString aChartTypeName = xChartType->getChartType();
    if(nDimensionIndex==2)//z-axis
        return AxisType::SERIES;
    if(nDimensionIndex==1)//y-axis
        return AxisType::REALNUMBER;
    if(nDimensionIndex==0)//x-axis
    {
        if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_SCATTER)
         || aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BUBBLE) )
            return AxisType::REALNUMBER;
        return AxisType::CATEGORY;
    }
    return AxisType::CATEGORY;
}

sal_Int32 ChartTypeHelper::getNumberOfDisplayedSeries(
    const rtl::Reference< ChartType >& xChartType,
    sal_Int32 nNumberOfSeries )
{
    if( xChartType.is() )
    {
        try
        {
            OUString aChartTypeName = xChartType->getChartType();
            if( aChartTypeName == CHART2_SERVICE_NAME_CHARTTYPE_PIE )
            {
                bool bDonut = false;
                if( (xChartType->getPropertyValue( "UseRings") >>= bDonut)
                    && !bDonut )
                {
                    return nNumberOfSeries>0 ? 1 : 0;
                }
            }
        }
        catch( const uno::Exception & )
        {
            DBG_UNHANDLED_EXCEPTION("chart2");
        }
    }
    return nNumberOfSeries;
}

uno::Sequence < sal_Int32 > ChartTypeHelper::getSupportedMissingValueTreatments( const rtl::Reference< ChartType >& xChartType )
{
    uno::Sequence < sal_Int32 > aRet;
    if( !xChartType.is() )
        return aRet;

    bool bFound=false;
    bool bAmbiguous=false;
    StackMode eStackMode = DiagramHelper::getStackModeFromChartType( xChartType, bFound, bAmbiguous, nullptr );
    bool bStacked = bFound && (eStackMode == StackMode::YStacked);

    OUString aChartTypeName = xChartType->getChartType();
    if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_COLUMN) ||
        aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BAR) ||
        aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BUBBLE) )
    {
        aRet.realloc( 2 );
        sal_Int32* pSeq = aRet.getArray();
        *pSeq++ = css::chart::MissingValueTreatment::LEAVE_GAP;
        *pSeq++ = css::chart::MissingValueTreatment::USE_ZERO;
    }
    else if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_AREA) )
    {
        aRet.realloc( bStacked ? 1 : 2 );
        sal_Int32* pSeq = aRet.getArray();
        *pSeq++ = css::chart::MissingValueTreatment::USE_ZERO;
        if( !bStacked )
            *pSeq++ = css::chart::MissingValueTreatment::CONTINUE;
    }
    else if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_LINE) ||
        aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_NET) ||
        aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_FILLED_NET) )
    {
        aRet.realloc( bStacked ? 2 : 3 );
        sal_Int32* pSeq = aRet.getArray();
        *pSeq++ = css::chart::MissingValueTreatment::LEAVE_GAP;
        *pSeq++ = css::chart::MissingValueTreatment::USE_ZERO;
        if( !bStacked )
            *pSeq++ = css::chart::MissingValueTreatment::CONTINUE;
    }
    else if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_SCATTER) )
    {
        aRet.realloc( 3 );
        sal_Int32* pSeq = aRet.getArray();
        *pSeq++ = css::chart::MissingValueTreatment::CONTINUE;
        *pSeq++ = css::chart::MissingValueTreatment::LEAVE_GAP;
        *pSeq++ = css::chart::MissingValueTreatment::USE_ZERO;
    }
    else if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_PIE) ||
        aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_CANDLESTICK) )
    {
        aRet.realloc( 0 );
    }
    else
    {
        OSL_FAIL( "unknown charttype" );
    }

    return aRet;
}

bool ChartTypeHelper::isSeriesInFrontOfAxisLine( const rtl::Reference< ChartType >& xChartType )
{
    if( xChartType.is() )
    {
        OUString aChartTypeName = xChartType->getChartType();
        if( aChartTypeName.match( CHART2_SERVICE_NAME_CHARTTYPE_FILLED_NET ) )
            return false;
    }
    return true;
}

OUString ChartTypeHelper::getRoleOfSequenceForYAxisNumberFormatDetection( const rtl::Reference< ChartType >& xChartType )
{
    OUString aRet( "values-y" );
    if( !xChartType.is() )
        return aRet;
    OUString aChartTypeName = xChartType->getChartType();
    if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_CANDLESTICK) )
        aRet = xChartType->getRoleOfSequenceForSeriesLabel();
    return aRet;
}

OUString ChartTypeHelper::getRoleOfSequenceForDataLabelNumberFormatDetection( const rtl::Reference< ChartType >& xChartType )
{
    OUString aRet( "values-y" );
    if( !xChartType.is() )
        return aRet;
    OUString aChartTypeName = xChartType->getChartType();
    if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_CANDLESTICK)
        || aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_BUBBLE) )
        aRet = xChartType->getRoleOfSequenceForSeriesLabel();
    return aRet;
}

bool ChartTypeHelper::isSupportingOnlyDeepStackingFor3D( const rtl::Reference< ChartType >& xChartType )
{
    bool bRet = false;
    if( !xChartType.is() )
        return bRet;

    OUString aChartTypeName = xChartType->getChartType();
    if( aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_LINE) ||
        aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_SCATTER) ||
        aChartTypeName.match(CHART2_SERVICE_NAME_CHARTTYPE_AREA) )
    {
        bRet = true;
    }
    return bRet;
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
