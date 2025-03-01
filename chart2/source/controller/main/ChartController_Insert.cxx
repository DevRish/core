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
#include <ChartController.hxx>

#include <dlg_InsertAxis_Grid.hxx>
#include <dlg_InsertDataLabel.hxx>
#include <dlg_InsertLegend.hxx>
#include <dlg_InsertErrorBars.hxx>
#include <dlg_InsertTitle.hxx>
#include <dlg_ObjectProperties.hxx>

#include <ChartModel.hxx>
#include <ChartModelHelper.hxx>
#include <Axis.hxx>
#include <AxisHelper.hxx>
#include <TitleHelper.hxx>
#include <DataSeries.hxx>
#include <DiagramHelper.hxx>
#include <Diagram.hxx>
#include <chartview/DrawModelWrapper.hxx>
#include <chartview/ChartSfxItemIds.hxx>
#include <NumberFormatterWrapper.hxx>
#include <ViewElementListProvider.hxx>
#include <MultipleChartConverters.hxx>
#include <ControllerLockGuard.hxx>
#include "UndoGuard.hxx"
#include <ResId.hxx>
#include <strings.hrc>
#include <ReferenceSizeProvider.hxx>
#include <ObjectIdentifier.hxx>
#include <RegressionCurveHelper.hxx>
#include <RegressionCurveItemConverter.hxx>
#include <StatisticsHelper.hxx>
#include <ErrorBarItemConverter.hxx>
#include <DataSeriesHelper.hxx>
#include <ObjectNameProvider.hxx>
#include <Legend.hxx>
#include <LegendHelper.hxx>
#include <RegressionCurveModel.hxx>

#include <com/sun/star/chart2/XRegressionCurve.hpp>
#include <com/sun/star/chart2/XRegressionCurveContainer.hpp>
#include <com/sun/star/chart2/XChartDocument.hpp>
#include <com/sun/star/chart2/XDataSeries.hpp>
#include <com/sun/star/chart/ErrorBarStyle.hpp>
#include <svx/ActionDescriptionProvider.hxx>

#include <tools/diagnose_ex.h>
#include <vcl/svapp.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::chart2;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;

namespace
{

void lcl_InsertMeanValueLine( const rtl::Reference< ::chart::DataSeries > & xSeries )
{
    if( xSeries.is())
    {
        ::chart::RegressionCurveHelper::addMeanValueLine(
            xSeries, xSeries);
    }
}

} // anonymous namespace

namespace chart
{

void ChartController::executeDispatch_InsertAxes()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_AXES )),
        m_xUndoManager );

    try
    {
        InsertAxisOrGridDialogData aDialogInput;
        rtl::Reference< Diagram > xDiagram = getFirstDiagram();
        AxisHelper::getAxisOrGridExistence( aDialogInput.aExistenceList, xDiagram );
        AxisHelper::getAxisOrGridPossibilities( aDialogInput.aPossibilityList, xDiagram );

        SolarMutexGuard aGuard;
        SchAxisDlg aDlg(GetChartFrame(), aDialogInput);
        if (aDlg.run() == RET_OK)
        {
            // lock controllers till end of block
            ControllerLockGuardUNO aCLGuard( getChartModel() );

            InsertAxisOrGridDialogData aDialogOutput;
            aDlg.getResult(aDialogOutput);
            std::unique_ptr< ReferenceSizeProvider > pRefSizeProvider(
                impl_createReferenceSizeProvider());
            bool bChanged = AxisHelper::changeVisibilityOfAxes( xDiagram
                , aDialogInput.aExistenceList, aDialogOutput.aExistenceList, m_xCC
                , pRefSizeProvider.get() );
            if( bChanged )
                aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_InsertGrid()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_GRIDS )),
        m_xUndoManager );

    try
    {
        InsertAxisOrGridDialogData aDialogInput;
        rtl::Reference< Diagram > xDiagram = getFirstDiagram();
        AxisHelper::getAxisOrGridExistence( aDialogInput.aExistenceList, xDiagram, false );
        AxisHelper::getAxisOrGridPossibilities( aDialogInput.aPossibilityList, xDiagram, false );

        SolarMutexGuard aGuard;
        SchGridDlg aDlg(GetChartFrame(), aDialogInput);//aItemSet, b3D, bNet, bSecondaryX, bSecondaryY );
        if (aDlg.run() == RET_OK)
        {
            // lock controllers till end of block
            ControllerLockGuardUNO aCLGuard( getChartModel() );
            InsertAxisOrGridDialogData aDialogOutput;
            aDlg.getResult( aDialogOutput );
            bool bChanged = AxisHelper::changeVisibilityOfGrids( xDiagram
                , aDialogInput.aExistenceList, aDialogOutput.aExistenceList );
            if( bChanged )
                aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_InsertTitles()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_TITLES )),
        m_xUndoManager );

    try
    {
        TitleDialogData aDialogInput;
        aDialogInput.readFromModel( getChartModel() );

        SolarMutexGuard aGuard;
        SchTitleDlg aDlg(GetChartFrame(), aDialogInput);
        if (aDlg.run() == RET_OK)
        {
            // lock controllers till end of block
            ControllerLockGuardUNO aCLGuard( getChartModel() );
            TitleDialogData aDialogOutput(impl_createReferenceSizeProvider());
            aDlg.getResult(aDialogOutput);
            bool bChanged = aDialogOutput.writeDifferenceToModel( getChartModel(), m_xCC, &aDialogInput );
            if( bChanged )
                aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_DeleteLegend()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Delete, SchResId( STR_OBJECT_LEGEND )),
        m_xUndoManager );

    LegendHelper::hideLegend(*getChartModel());
    aUndoGuard.commit();
}

void ChartController::executeDispatch_InsertLegend()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_LEGEND )),
        m_xUndoManager );

    LegendHelper::showLegend(*getChartModel(), m_xCC);
    aUndoGuard.commit();
}

void ChartController::executeDispatch_OpenLegendDialog()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_LEGEND )),
        m_xUndoManager );

    try
    {
        //prepare and open dialog
        SolarMutexGuard aGuard;
        SchLegendDlg aDlg(GetChartFrame(), m_xCC);
        aDlg.init( getChartModel() );
        if (aDlg.run() == RET_OK)
        {
            // lock controllers till end of block
            ControllerLockGuardUNO aCLGuard( getChartModel() );
            aDlg.writeToModel( getChartModel() );
            aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_InsertMenu_DataLabels()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_DATALABELS )),
        m_xUndoManager );

    //if a series is selected insert labels for that series only:
    rtl::Reference< DataSeries > xSeries =
        ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel());
    if( xSeries.is() )
    {
        // add labels
        DataSeriesHelper::insertDataLabelsToSeriesAndAllPoints( xSeries );

        OUString aChildParticle( ObjectIdentifier::getStringForType( OBJECTTYPE_DATA_LABELS ) + "=" );
        OUString aObjectCID = ObjectIdentifier::createClassifiedIdentifierForParticles(
            ObjectIdentifier::getSeriesParticleFromCID(m_aSelection.getSelectedCID()), aChildParticle );

        bool bSuccess = ChartController::executeDlg_ObjectProperties_withoutUndoGuard( aObjectCID, true );
        if( bSuccess )
            aUndoGuard.commit();
        return;
    }

    try
    {
        wrapper::AllDataLabelItemConverter aItemConverter(
            getChartModel(),
            m_pDrawModelWrapper->GetItemPool(),
            m_pDrawModelWrapper->getSdrModel(),
            getChartModel() );
        SfxItemSet aItemSet = aItemConverter.CreateEmptyItemSet();
        aItemConverter.FillItemSet( aItemSet );

        //prepare and open dialog
        SolarMutexGuard aGuard;

        //get number formatter
        NumberFormatterWrapper aNumberFormatterWrapper( getChartModel() );
        SvNumberFormatter* pNumberFormatter = aNumberFormatterWrapper.getSvNumberFormatter();

        DataLabelsDialog aDlg(GetChartFrame(), aItemSet, pNumberFormatter);

        if (aDlg.run() == RET_OK)
        {
            SfxItemSet aOutItemSet = aItemConverter.CreateEmptyItemSet();
            aDlg.FillItemSet(aOutItemSet);
            // lock controllers till end of block
            ControllerLockGuardUNO aCLGuard( getChartModel() );
            bool bChanged = aItemConverter.ApplyItemSet( aOutItemSet );//model should be changed now
            if( bChanged )
                aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_InsertMeanValue()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_AVERAGE_LINE )),
        m_xUndoManager );
    lcl_InsertMeanValueLine( ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(),
                                                                    getChartModel() ) );
    aUndoGuard.commit();
}

void ChartController::executeDispatch_InsertMenu_MeanValues()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_AVERAGE_LINE )),
        m_xUndoManager );

    rtl::Reference< DataSeries > xSeries =
        ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );
    if( xSeries.is() )
    {
        //if a series is selected insert mean value only for that series:
        lcl_InsertMeanValueLine( xSeries );
    }
    else
    {
        std::vector< rtl::Reference< DataSeries > > aSeries =
            DiagramHelper::getDataSeriesFromDiagram( getFirstDiagram());

        for( const auto& xSrs : aSeries )
            lcl_InsertMeanValueLine( xSrs );
    }
    aUndoGuard.commit();
}

void ChartController::executeDispatch_InsertMenu_Trendlines()
{
    OUString aCID = m_aSelection.getSelectedCID();

    rtl::Reference< DataSeries > xSeries =
        ObjectIdentifier::getDataSeriesForCID( aCID, getChartModel() );

    if( !xSeries.is() )
        return;

    executeDispatch_InsertTrendline();
}

void ChartController::executeDispatch_InsertTrendline()
{
    rtl::Reference< DataSeries > xRegressionCurveContainer =
        ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel());

    if( !xRegressionCurveContainer.is() )
        return;

    UndoLiveUpdateGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_CURVE )),
        m_xUndoManager );

    rtl::Reference< RegressionCurveModel > xCurve =
        RegressionCurveHelper::addRegressionCurve(
            SvxChartRegress::Linear,
            xRegressionCurveContainer );

    if( !xCurve.is())
        return;

    wrapper::RegressionCurveItemConverter aItemConverter(
        xCurve, xRegressionCurveContainer, m_pDrawModelWrapper->getSdrModel().GetItemPool(),
        m_pDrawModelWrapper->getSdrModel(),
        getChartModel() );

    // open dialog
    SfxItemSet aItemSet = aItemConverter.CreateEmptyItemSet();
    aItemConverter.FillItemSet( aItemSet );
    ObjectPropertiesDialogParameter aDialogParameter(
        ObjectIdentifier::createDataCurveCID(
            ObjectIdentifier::getSeriesParticleFromCID( m_aSelection.getSelectedCID()),
            RegressionCurveHelper::getRegressionCurveIndex( xRegressionCurveContainer, xCurve ), false ));
    aDialogParameter.init( getChartModel() );
    ViewElementListProvider aViewElementListProvider( m_pDrawModelWrapper.get());
    SolarMutexGuard aGuard;
    SchAttribTabDlg aDialog(
        GetChartFrame(), &aItemSet, &aDialogParameter,
        &aViewElementListProvider,
        getChartModel() );

    // note: when a user pressed "OK" but didn't change any settings in the
    // dialog, the SfxTabDialog returns "Cancel"
    if( aDialog.run() == RET_OK || aDialog.DialogWasClosedWithOK())
    {
        const SfxItemSet* pOutItemSet = aDialog.GetOutputItemSet();
        if( pOutItemSet )
        {
            ControllerLockGuardUNO aCLGuard( getChartModel() );
            aItemConverter.ApplyItemSet( *pOutItemSet );
        }
        aUndoGuard.commit();
    }
}

void ChartController::executeDispatch_InsertErrorBars( bool bYError )
{
    ObjectType objType = bYError ? OBJECTTYPE_DATA_ERRORS_Y : OBJECTTYPE_DATA_ERRORS_X;

    //if a series is selected insert error bars for that series only:
    rtl::Reference< DataSeries > xSeries =
        ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );

    if( xSeries.is())
    {
        UndoLiveUpdateGuard aUndoGuard(
            ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::ActionType::Insert,
                SchResId( bYError ? STR_OBJECT_ERROR_BARS_Y : STR_OBJECT_ERROR_BARS_X )),
            m_xUndoManager );

        // add error bars with standard deviation
        uno::Reference< beans::XPropertySet > xErrorBarProp(
            StatisticsHelper::addErrorBars( xSeries,
                                            css::chart::ErrorBarStyle::STANDARD_DEVIATION,
                                            bYError));

        // get an appropriate item converter
        wrapper::ErrorBarItemConverter aItemConverter(
            getChartModel(), xErrorBarProp, m_pDrawModelWrapper->getSdrModel().GetItemPool(),
            m_pDrawModelWrapper->getSdrModel(),
            getChartModel() );

        // open dialog
        SfxItemSet aItemSet = aItemConverter.CreateEmptyItemSet();
        aItemSet.Put(SfxBoolItem(SCHATTR_STAT_ERRORBAR_TYPE,bYError));
        aItemConverter.FillItemSet( aItemSet );
        ObjectPropertiesDialogParameter aDialogParameter(
            ObjectIdentifier::createClassifiedIdentifierWithParent(
                objType, u"", m_aSelection.getSelectedCID()));
        aDialogParameter.init( getChartModel() );
        ViewElementListProvider aViewElementListProvider( m_pDrawModelWrapper.get());
        SolarMutexGuard aGuard;
        SchAttribTabDlg aDlg(
                GetChartFrame(), &aItemSet, &aDialogParameter,
                &aViewElementListProvider,
                getChartModel() );
        aDlg.SetAxisMinorStepWidthForErrorBarDecimals(
            InsertErrorBarsDialog::getAxisMinorStepWidthForErrorBarDecimals( getChartModel(),
                                                                             m_xChartView, m_aSelection.getSelectedCID()));

        // note: when a user pressed "OK" but didn't change any settings in the
        // dialog, the SfxTabDialog returns "Cancel"
        if (aDlg.run() == RET_OK || aDlg.DialogWasClosedWithOK())
        {
            const SfxItemSet* pOutItemSet = aDlg.GetOutputItemSet();
            if( pOutItemSet )
            {
                ControllerLockGuardUNO aCLGuard( getChartModel() );
                aItemConverter.ApplyItemSet( *pOutItemSet );
            }
            aUndoGuard.commit();
        }
    }
    else
    {
        //if no series is selected insert error bars for all series
        UndoGuard aUndoGuard(
            ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::ActionType::Insert,
                ObjectNameProvider::getName_ObjectForAllSeries( objType ) ),
            m_xUndoManager );

        try
        {
            wrapper::AllSeriesStatisticsConverter aItemConverter(
                getChartModel(), m_pDrawModelWrapper->GetItemPool() );
            SfxItemSet aItemSet = aItemConverter.CreateEmptyItemSet();
            aItemConverter.FillItemSet( aItemSet );

            //prepare and open dialog
            SolarMutexGuard aGuard;
            InsertErrorBarsDialog aDlg(
                GetChartFrame(), aItemSet,
                getChartModel(),
                bYError ? ErrorBarResources::ERROR_BAR_Y : ErrorBarResources::ERROR_BAR_X);

            aDlg.SetAxisMinorStepWidthForErrorBarDecimals(
                InsertErrorBarsDialog::getAxisMinorStepWidthForErrorBarDecimals( getChartModel(), m_xChartView, OUString() ) );

            if (aDlg.run() == RET_OK)
            {
                SfxItemSet aOutItemSet = aItemConverter.CreateEmptyItemSet();
                aDlg.FillItemSet( aOutItemSet );

                // lock controllers till end of block
                ControllerLockGuardUNO aCLGuard( getChartModel() );
                bool bChanged = aItemConverter.ApplyItemSet( aOutItemSet );//model should be changed now
                if( bChanged )
                    aUndoGuard.commit();
            }
        }
        catch(const uno::RuntimeException&)
        {
            TOOLS_WARN_EXCEPTION("chart2", "" );
        }
    }
}

void ChartController::executeDispatch_InsertTrendlineEquation( bool bInsertR2 )
{
    uno::Reference< chart2::XRegressionCurve > xRegCurve(
        ObjectIdentifier::getObjectPropertySet( m_aSelection.getSelectedCID(), getChartModel() ), uno::UNO_QUERY );
    if( !xRegCurve.is() )
    {
        rtl::Reference< DataSeries > xRegCurveCnt =
            ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );
        xRegCurve.set( RegressionCurveHelper::getFirstCurveNotMeanValueLine( xRegCurveCnt ) );
    }
    if( !xRegCurve.is())
        return;

    uno::Reference< beans::XPropertySet > xEqProp( xRegCurve->getEquationProperties());
    if( xEqProp.is())
    {
        UndoGuard aUndoGuard(
            ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_CURVE_EQUATION )),
            m_xUndoManager );
        xEqProp->setPropertyValue( "ShowEquation", uno::Any( true ));
        xEqProp->setPropertyValue( "XName", uno::Any( OUString("x") ));
        xEqProp->setPropertyValue( "YName", uno::Any( OUString("f(x)") ));
        xEqProp->setPropertyValue( "ShowCorrelationCoefficient", uno::Any( bInsertR2 ));
        aUndoGuard.commit();
    }
}

void ChartController::executeDispatch_InsertR2Value()
{
    uno::Reference< beans::XPropertySet > xEqProp =
        ObjectIdentifier::getObjectPropertySet( m_aSelection.getSelectedCID(), getChartModel() );
    if( xEqProp.is())
    {
        UndoGuard aUndoGuard(
            ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_CURVE_EQUATION )),
            m_xUndoManager );
        xEqProp->setPropertyValue( "ShowCorrelationCoefficient", uno::Any( true ));
        aUndoGuard.commit();
    }
}

void ChartController::executeDispatch_DeleteR2Value()
{
    uno::Reference< beans::XPropertySet > xEqProp =
        ObjectIdentifier::getObjectPropertySet( m_aSelection.getSelectedCID(), getChartModel() );
    if( xEqProp.is())
    {
        UndoGuard aUndoGuard(
            ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_CURVE_EQUATION )),
            m_xUndoManager );
        xEqProp->setPropertyValue( "ShowCorrelationCoefficient", uno::Any( false ));
        aUndoGuard.commit();
    }
}

void ChartController::executeDispatch_DeleteMeanValue()
{
    rtl::Reference< DataSeries > xRegCurveCnt =
        ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );
    if( xRegCurveCnt.is())
    {
        UndoGuard aUndoGuard(
            ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::ActionType::Delete, SchResId( STR_OBJECT_AVERAGE_LINE )),
            m_xUndoManager );
        RegressionCurveHelper::removeMeanValueLine( xRegCurveCnt );
        aUndoGuard.commit();
    }
}

void ChartController::executeDispatch_DeleteTrendline()
{
    rtl::Reference< DataSeries > xRegCurveCnt =
        ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );
    if( xRegCurveCnt.is())
    {
        UndoGuard aUndoGuard(
            ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::ActionType::Delete, SchResId( STR_OBJECT_CURVE )),
            m_xUndoManager );
        RegressionCurveHelper::removeAllExceptMeanValueLine( xRegCurveCnt );
        aUndoGuard.commit();
    }
}

void ChartController::executeDispatch_DeleteTrendlineEquation()
{
    rtl::Reference< DataSeries > xRegCurveCnt =
        ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );
    if( xRegCurveCnt.is())
    {
        UndoGuard aUndoGuard(
            ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::ActionType::Delete, SchResId( STR_OBJECT_CURVE_EQUATION )),
            m_xUndoManager );
        RegressionCurveHelper::removeEquations( xRegCurveCnt );
        aUndoGuard.commit();
    }
}

void ChartController::executeDispatch_DeleteErrorBars( bool bYError )
{
    rtl::Reference< DataSeries > xDataSeries =
        ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );
    if( xDataSeries.is())
    {
        UndoGuard aUndoGuard(
            ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::ActionType::Delete, SchResId( STR_OBJECT_CURVE )),
            m_xUndoManager );
        StatisticsHelper::removeErrorBars( xDataSeries, bYError );
        aUndoGuard.commit();
    }
}

void ChartController::executeDispatch_InsertDataLabels()
{
    rtl::Reference< DataSeries > xSeries =
        ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );
    if( xSeries.is() )
    {
        UndoGuard aUndoGuard( ActionDescriptionProvider::createDescription( ActionDescriptionProvider::ActionType::Insert,
            SchResId( STR_OBJECT_DATALABELS )),
            m_xUndoManager );
        DataSeriesHelper::insertDataLabelsToSeriesAndAllPoints( xSeries );
        aUndoGuard.commit();
    }
}

void ChartController::executeDispatch_InsertDataLabel()
{
    UndoGuard aUndoGuard( ActionDescriptionProvider::createDescription( ActionDescriptionProvider::ActionType::Insert,
        SchResId( STR_OBJECT_LABEL )),
        m_xUndoManager );
    DataSeriesHelper::insertDataLabelToPoint( ObjectIdentifier::getObjectPropertySet( m_aSelection.getSelectedCID(), getChartModel() ) );
    aUndoGuard.commit();
}

void ChartController::executeDispatch_DeleteDataLabels()
{
    rtl::Reference< DataSeries > xSeries =
        ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );
    if( xSeries.is() )
    {
        UndoGuard aUndoGuard( ActionDescriptionProvider::createDescription( ActionDescriptionProvider::ActionType::Delete,
            SchResId( STR_OBJECT_DATALABELS )),
            m_xUndoManager );
        DataSeriesHelper::deleteDataLabelsFromSeriesAndAllPoints( xSeries );
        aUndoGuard.commit();
    }
}

void ChartController::executeDispatch_DeleteDataLabel()
{
    UndoGuard aUndoGuard( ActionDescriptionProvider::createDescription( ActionDescriptionProvider::ActionType::Delete,
        SchResId( STR_OBJECT_LABEL )),
        m_xUndoManager );
    DataSeriesHelper::deleteDataLabelsFromPoint( ObjectIdentifier::getObjectPropertySet( m_aSelection.getSelectedCID(), getChartModel() ) );
    aUndoGuard.commit();
}

void ChartController::executeDispatch_ResetAllDataPoints()
{
    UndoGuard aUndoGuard( ActionDescriptionProvider::createDescription( ActionDescriptionProvider::ActionType::Format,
        SchResId( STR_OBJECT_DATAPOINTS )),
        m_xUndoManager );
    rtl::Reference< DataSeries > xSeries = ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );
    if( xSeries.is() )
        xSeries->resetAllDataPoints();
    aUndoGuard.commit();
}
void ChartController::executeDispatch_ResetDataPoint()
{
    UndoGuard aUndoGuard( ActionDescriptionProvider::createDescription( ActionDescriptionProvider::ActionType::Format,
        SchResId( STR_OBJECT_DATAPOINT )),
        m_xUndoManager );
    rtl::Reference< DataSeries > xSeries = ObjectIdentifier::getDataSeriesForCID( m_aSelection.getSelectedCID(), getChartModel() );
    if( xSeries.is() )
    {
        sal_Int32 nPointIndex = ObjectIdentifier::getIndexFromParticleOrCID( m_aSelection.getSelectedCID() );
        xSeries->resetDataPoint( nPointIndex );
    }
    aUndoGuard.commit();
}

void ChartController::executeDispatch_InsertAxisTitle()
{
    try
    {
        uno::Reference< XTitle > xTitle;
        {
            UndoGuard aUndoGuard(
            ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_TITLE )),
            m_xUndoManager );

            rtl::Reference< Axis > xAxis = ObjectIdentifier::getAxisForCID( m_aSelection.getSelectedCID(), getChartModel() );
            sal_Int32 nDimensionIndex = -1;
            sal_Int32 nCooSysIndex = -1;
            sal_Int32 nAxisIndex = -1;
            AxisHelper::getIndicesForAxis( xAxis, getFirstDiagram(), nCooSysIndex, nDimensionIndex, nAxisIndex );

            TitleHelper::eTitleType eTitleType = TitleHelper::X_AXIS_TITLE;
            if( nDimensionIndex==0 )
                eTitleType = nAxisIndex==0 ? TitleHelper::X_AXIS_TITLE : TitleHelper::SECONDARY_X_AXIS_TITLE;
            else if( nDimensionIndex==1 )
                eTitleType = nAxisIndex==0 ? TitleHelper::Y_AXIS_TITLE : TitleHelper::SECONDARY_Y_AXIS_TITLE;
            else
                eTitleType = TitleHelper::Z_AXIS_TITLE;

            std::unique_ptr< ReferenceSizeProvider > apRefSizeProvider( impl_createReferenceSizeProvider());
            xTitle = TitleHelper::createTitle( eTitleType, ObjectNameProvider::getTitleNameByType(eTitleType), getChartModel(), m_xCC, apRefSizeProvider.get() );
            aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_InsertAxis()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_AXIS )),
        m_xUndoManager );

    try
    {
        rtl::Reference< Axis > xAxis = ObjectIdentifier::getAxisForCID( m_aSelection.getSelectedCID(), getChartModel() );
        if( xAxis.is() )
        {
            AxisHelper::makeAxisVisible( xAxis );
            aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_DeleteAxis()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Delete, SchResId( STR_OBJECT_AXIS )),
        m_xUndoManager );

    try
    {
        rtl::Reference< Axis > xAxis = ObjectIdentifier::getAxisForCID( m_aSelection.getSelectedCID(), getChartModel() );
        if( xAxis.is() )
        {
            AxisHelper::makeAxisInvisible( xAxis );
            aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_InsertMajorGrid()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_GRID )),
        m_xUndoManager );

    try
    {
        rtl::Reference< Axis > xAxis = ObjectIdentifier::getAxisForCID( m_aSelection.getSelectedCID(), getChartModel() );
        if( xAxis.is() )
        {
            AxisHelper::makeGridVisible( xAxis->getGridProperties() );
            aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_DeleteMajorGrid()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Delete, SchResId( STR_OBJECT_GRID )),
        m_xUndoManager );

    try
    {
        rtl::Reference< Axis > xAxis = ObjectIdentifier::getAxisForCID( m_aSelection.getSelectedCID(), getChartModel() );
        if( xAxis.is() )
        {
            AxisHelper::makeGridInvisible( xAxis->getGridProperties() );
            aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_InsertMinorGrid()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Insert, SchResId( STR_OBJECT_GRID )),
        m_xUndoManager );

    try
    {
        rtl::Reference< Axis > xAxis = ObjectIdentifier::getAxisForCID( m_aSelection.getSelectedCID(), getChartModel() );
        if( xAxis.is() )
        {
            const Sequence< Reference< beans::XPropertySet > > aSubGrids( xAxis->getSubGridProperties() );
            for( Reference< beans::XPropertySet > const & props : aSubGrids)
                AxisHelper::makeGridVisible( props );
            aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

void ChartController::executeDispatch_DeleteMinorGrid()
{
    UndoGuard aUndoGuard(
        ActionDescriptionProvider::createDescription(
            ActionDescriptionProvider::ActionType::Delete, SchResId(STR_OBJECT_GRID)),
        m_xUndoManager );

    try
    {
        rtl::Reference< Axis > xAxis = ObjectIdentifier::getAxisForCID( m_aSelection.getSelectedCID(), getChartModel() );
        if( xAxis.is() )
        {
            const Sequence< Reference< beans::XPropertySet > > aSubGrids( xAxis->getSubGridProperties() );
            for( Reference< beans::XPropertySet > const & props : aSubGrids)
                AxisHelper::makeGridInvisible( props );
            aUndoGuard.commit();
        }
    }
    catch(const uno::RuntimeException&)
    {
        TOOLS_WARN_EXCEPTION("chart2", "" );
    }
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
