/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "charttest.hxx"
#include <com/sun/star/chart2/CurveStyle.hpp>
#include <com/sun/star/chart2/DataPointLabel.hpp>
#include <com/sun/star/chart2/XDataPointCustomLabelField.hpp>
#include <com/sun/star/chart2/DataPointCustomLabelFieldType.hpp>
#include <com/sun/star/chart/ErrorBarStyle.hpp>
#include <com/sun/star/chart2/XRegressionCurveContainer.hpp>
#include <com/sun/star/chart2/XChartDocument.hpp>
#include <com/sun/star/chart/XChartDocument.hpp>
#include <com/sun/star/chart2/XInternalDataProvider.hpp>
#include <com/sun/star/chart/XChartDataArray.hpp>
#include <com/sun/star/drawing/FillStyle.hpp>
#include <com/sun/star/drawing/LineStyle.hpp>
#include <com/sun/star/chart/XAxisXSupplier.hpp>
#include <com/sun/star/chart/XAxisYSupplier.hpp>
#include <com/sun/star/chart/MissingValueTreatment.hpp>
#include <com/sun/star/chart2/TickmarkStyle.hpp>
#include <com/sun/star/chart2/SymbolStyle.hpp>
#include <com/sun/star/chart2/Symbol.hpp>
#include <com/sun/star/chart2/data/XTextualDataSequence.hpp>
#include <com/sun/star/chart/DataLabelPlacement.hpp>
#include <com/sun/star/text/XTextRange.hpp>
#include <iterator>

#include <com/sun/star/util/Color.hpp>
#include <com/sun/star/awt/Gradient.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

class Chart2ImportTest : public ChartTest
{
public:
    void Fdo60083();
    void testSteppedLines();
    void testErrorBarRange();
    void testErrorBarFormatting();
    void testODSChartSeries();
    void testXLSXChartSeries();
    void testXLSChartSeries();
    void testODTChartSeries();
    void testDOCChartSeries();
    void testDOCXChartSeries();
    void testDOCXChartEmptySeries();
    void testTdf81396();
    void testDOCXChartValuesSize();
    void testPPTXChartSeries();
    void testPPTXSparseChartSeries();
    /**
     * Original data contains 3 series but 2 of them are hidden. For now, we
     * detect and skip those hidden series on import (since we don't support
     * hidden columns for internal data table yet).
     */
    void testPPTXHiddenDataSeries();
    void testPPTXPercentageNumberFormats();
    void testPieChartLabelsNumFormat();
    void testPPTXStackedNonStackedYAxis();
    void testPPTChartSeries();
    void testODPChartSeries();
    void testBnc864396();
    void testBnc882383();
    void testTransparencyGradientValue();
    void testBnc889755();
    void testSimpleStrictXLSX();
    void testDelayedCellImport(); // chart range referencing content on later sheets
    void testFlatODSStackedColumnChart();
    void testFdo78080();
    void testFdo54361();
    void testFdo54361_1();
    void testTdf127811();
    void testTdf86624(); // manually placed legends
    void testTdf105517();
    void testTdf106217();
    void testTdf108021();
    void testTdf100084();
    void testTdf124817();
    void testTdf126033();
    void testAutoBackgroundXLSX();
    void testAutoChartAreaBorderPropXLSX();
    void testChartAreaStyleBackgroundXLSX();
    void testChartHatchFillXLSX();
    void testAxisTextRotationXLSX();
    // void testTextCanOverlapXLSX(); // TODO : temporarily disabled.
    void testTextBreakXLSX();
    void testNumberFormatsXLSX();
    void testNumberFormatsDOCX();
    void testPercentageNumberFormatsDOCX();

    void testTransparentBackground(OUString const & filename);

    // below are OOXML default value tests for cases
    // where we fixed the handling of MSO 2007 vs OOXML
    void testAutoTitleDelDefaultValue2007XLSX();
    void testAutoTitleDelDefaultValue2013XLSX();
    void testDispBlanksAsDefaultValue2007XLSX();
    void testDispBlanksAsDefaultValue2013XLSX();

    void testSmoothDefaultValue2007XLSX();
    void testSmoothDefaultValue2013XLSX();
    void testTrendlineDefaultValue2007XLSX();
    void testTrendlineDefaultValue2013XLSX();
    void testVaryColorDefaultValues2007XLSX();
    void testVaryColorDefaultValues2013XLSX();
    void testPlotVisOnlyDefaultValue2013XLSX();
    void testRAngAxDefaultValue2013XLSX();
    void testMajorTickMarksDefaultValue2013XLSX();
    void testMinorTickMarksDefaultValue2013XLSX();

    void testAxisTitleDefaultRotationXLSX();
    void testSecondaryAxisTitleDefaultRotationXLSX();
    void testAxisTitleRotationXLSX();
    void testAxisTitlePositionDOCX();
    void testCombinedChartAttachedAxisXLSX();
    void testTdf140489MultiSeriesChartAxisXLSX();

    void testTdf90510(); // Pie chart label placement settings(XLS)
    void testTdf109858(); // Pie chart label placement settings(XLSX)
    void testTdf130105();
    void testTdf111173();
    void testTdf122226();
    void testInternalDataProvider();
    void testTdf115107(); // import complex data point labels
    void testTdf115107_2(); // import complex data point labels in cobo charts with multiple data series
    void testTdf116163();
    void testTdf48041();
    void testTdf121205();
    void testFixedSizeBarChartVeryLongLabel();
    void testAutomaticSizeBarChartVeryLongLabel();

    CPPUNIT_TEST_SUITE(Chart2ImportTest);
    CPPUNIT_TEST(Fdo60083);
    CPPUNIT_TEST(testSteppedLines);
    CPPUNIT_TEST(testErrorBarRange);
    CPPUNIT_TEST(testErrorBarFormatting);
    CPPUNIT_TEST(testODSChartSeries);
    CPPUNIT_TEST(testXLSXChartSeries);
    CPPUNIT_TEST(testXLSChartSeries);
    CPPUNIT_TEST(testODTChartSeries);
    CPPUNIT_TEST(testDOCChartSeries);
    CPPUNIT_TEST(testDOCXChartSeries);
    CPPUNIT_TEST(testDOCXChartEmptySeries);
    CPPUNIT_TEST(testTdf81396);
    CPPUNIT_TEST(testDOCXChartValuesSize);
    CPPUNIT_TEST(testPPTChartSeries);
    CPPUNIT_TEST(testPPTXChartSeries);
    CPPUNIT_TEST(testPPTXSparseChartSeries);
    CPPUNIT_TEST(testPPTXHiddenDataSeries);
    CPPUNIT_TEST(testPPTXPercentageNumberFormats);
    CPPUNIT_TEST(testPieChartLabelsNumFormat);
    CPPUNIT_TEST(testPPTXStackedNonStackedYAxis);
    CPPUNIT_TEST(testODPChartSeries);
    CPPUNIT_TEST(testBnc864396);
    CPPUNIT_TEST(testBnc882383);
    CPPUNIT_TEST(testTransparencyGradientValue);
    CPPUNIT_TEST(testBnc889755);
    CPPUNIT_TEST(testSimpleStrictXLSX);
    CPPUNIT_TEST(testDelayedCellImport);
    CPPUNIT_TEST(testFlatODSStackedColumnChart);
    CPPUNIT_TEST(testFdo78080);
    CPPUNIT_TEST(testFdo54361);
    CPPUNIT_TEST(testFdo54361_1);
    CPPUNIT_TEST(testTdf127811);
    CPPUNIT_TEST(testTdf86624);
    CPPUNIT_TEST(testTdf105517);
    CPPUNIT_TEST(testTdf106217);
    CPPUNIT_TEST(testTdf108021);
    CPPUNIT_TEST(testTdf100084);
    CPPUNIT_TEST(testTdf124817);
    CPPUNIT_TEST(testTdf126033);
    CPPUNIT_TEST(testAutoBackgroundXLSX);
    CPPUNIT_TEST(testAutoChartAreaBorderPropXLSX);
    CPPUNIT_TEST(testChartAreaStyleBackgroundXLSX);
    CPPUNIT_TEST(testChartHatchFillXLSX);
    CPPUNIT_TEST(testAxisTextRotationXLSX);
    // CPPUNIT_TEST(testTextCanOverlapXLSX); // TODO : temporarily disabled.
    CPPUNIT_TEST(testTextBreakXLSX);
    CPPUNIT_TEST(testNumberFormatsXLSX);
    CPPUNIT_TEST(testNumberFormatsDOCX);
    CPPUNIT_TEST(testPercentageNumberFormatsDOCX);
    CPPUNIT_TEST(testAutoTitleDelDefaultValue2007XLSX);
    CPPUNIT_TEST(testAutoTitleDelDefaultValue2013XLSX);
    CPPUNIT_TEST(testDispBlanksAsDefaultValue2007XLSX);
    CPPUNIT_TEST(testDispBlanksAsDefaultValue2013XLSX);
    CPPUNIT_TEST(testSmoothDefaultValue2007XLSX);
    CPPUNIT_TEST(testSmoothDefaultValue2013XLSX);
    CPPUNIT_TEST(testTrendlineDefaultValue2007XLSX);
    CPPUNIT_TEST(testTrendlineDefaultValue2013XLSX);
    CPPUNIT_TEST(testVaryColorDefaultValues2007XLSX);
    CPPUNIT_TEST(testVaryColorDefaultValues2013XLSX);
    CPPUNIT_TEST(testPlotVisOnlyDefaultValue2013XLSX);
    CPPUNIT_TEST(testRAngAxDefaultValue2013XLSX);
    CPPUNIT_TEST(testMajorTickMarksDefaultValue2013XLSX);
    CPPUNIT_TEST(testMinorTickMarksDefaultValue2013XLSX);
    CPPUNIT_TEST(testAxisTitleDefaultRotationXLSX);
    CPPUNIT_TEST(testSecondaryAxisTitleDefaultRotationXLSX);
    CPPUNIT_TEST(testAxisTitleRotationXLSX);
    CPPUNIT_TEST(testAxisTitlePositionDOCX);
    CPPUNIT_TEST(testCombinedChartAttachedAxisXLSX);
    CPPUNIT_TEST(testTdf140489MultiSeriesChartAxisXLSX);
    CPPUNIT_TEST(testTdf90510);
    CPPUNIT_TEST(testTdf109858);
    CPPUNIT_TEST(testTdf130105);
    CPPUNIT_TEST(testTdf111173);
    CPPUNIT_TEST(testTdf122226);
    CPPUNIT_TEST(testInternalDataProvider);
    CPPUNIT_TEST(testTdf115107);
    CPPUNIT_TEST(testTdf115107_2);
    CPPUNIT_TEST(testTdf116163);
    CPPUNIT_TEST(testTdf48041);
    CPPUNIT_TEST(testTdf121205);
    CPPUNIT_TEST(testFixedSizeBarChartVeryLongLabel);
    CPPUNIT_TEST(testAutomaticSizeBarChartVeryLongLabel);

    CPPUNIT_TEST_SUITE_END();
};

// error bar import
// split method up into smaller chunks for more detailed tests
void Chart2ImportTest::Fdo60083()
{
    load(u"/chart2/qa/extras/data/ods/", "fdo60083.ods");
    uno::Reference< chart2::XChartDocument > xChartDoc = getChartDocFromSheet( 0, mxComponent );
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference< chart2::XDataSeries > xDataSeries = getDataSeriesFromDoc( xChartDoc, 0 );
    CPPUNIT_ASSERT( xDataSeries.is() );

    Reference< beans::XPropertySet > xPropSet( xDataSeries, UNO_QUERY_THROW );

    // test that y error bars are there
    Reference< beans::XPropertySet > xErrorBarYProps;
    xPropSet->getPropertyValue(CHART_UNONAME_ERRORBAR_Y) >>= xErrorBarYProps;
    CPPUNIT_ASSERT(xErrorBarYProps.is());
    {
        sal_Int32 nErrorBarStyle;
        CPPUNIT_ASSERT(
            xErrorBarYProps->getPropertyValue("ErrorBarStyle")
            >>= nErrorBarStyle);
        CPPUNIT_ASSERT_EQUAL(
            chart::ErrorBarStyle::RELATIVE,
            nErrorBarStyle);

        double nVal = 0.0;
        CPPUNIT_ASSERT(
            xErrorBarYProps->getPropertyValue("PositiveError") >>= nVal);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, nVal, 1e-8);

        CPPUNIT_ASSERT(
            xErrorBarYProps->getPropertyValue("NegativeError") >>= nVal);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, nVal, 1e-8);

        bool bVal;
        CPPUNIT_ASSERT(
            xErrorBarYProps->getPropertyValue("ShowPositiveError") >>= bVal);
        CPPUNIT_ASSERT_EQUAL(true, bVal);

        CPPUNIT_ASSERT(
            xErrorBarYProps->getPropertyValue("ShowNegativeError") >>= bVal);
        CPPUNIT_ASSERT_EQUAL(true, bVal);
    }

    // test that x error bars are not imported
    Reference< beans::XPropertySet > xErrorBarXProps;
    xPropSet->getPropertyValue(CHART_UNONAME_ERRORBAR_X) >>= xErrorBarXProps;
    CPPUNIT_ASSERT(!xErrorBarXProps.is());
}

void Chart2ImportTest::testErrorBarRange()
{
    load(u"/chart2/qa/extras/data/ods/", "error_bar_range.ods");
    uno::Reference< chart2::XChartDocument > xChartDoc = getChartDocFromSheet( 0, mxComponent );
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference< chart2::XDataSeries > xDataSeries = getDataSeriesFromDoc( xChartDoc, 0 );
    CPPUNIT_ASSERT( xDataSeries.is() );

    Reference< beans::XPropertySet > xPropSet( xDataSeries, UNO_QUERY_THROW );

    // test that y error bars are there
    Reference< beans::XPropertySet > xErrorBarYProps;
    xPropSet->getPropertyValue(CHART_UNONAME_ERRORBAR_Y) >>= xErrorBarYProps;
    CPPUNIT_ASSERT(xErrorBarYProps.is());

    sal_Int32 nErrorBarStyle;
    CPPUNIT_ASSERT(
            xErrorBarYProps->getPropertyValue("ErrorBarStyle")
            >>= nErrorBarStyle);
    CPPUNIT_ASSERT_EQUAL(
            chart::ErrorBarStyle::FROM_DATA,
            nErrorBarStyle);

    OUString aRangePos;
    CPPUNIT_ASSERT(xErrorBarYProps->getPropertyValue("ErrorBarRangePositive") >>= aRangePos);
    CPPUNIT_ASSERT_EQUAL(OUString("$Sheet1.$C$2:$C$4"), aRangePos);
}

void Chart2ImportTest::testErrorBarFormatting()
{
    load(u"/chart2/qa/extras/data/ods/", "error_bar_properties.ods");
    uno::Reference< chart2::XChartDocument > xChartDoc = getChartDocFromSheet( 0, mxComponent );
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference< chart2::XDataSeries > xDataSeries = getDataSeriesFromDoc( xChartDoc, 0 );
    CPPUNIT_ASSERT( xDataSeries.is() );

    Reference< beans::XPropertySet > xPropSet( xDataSeries, UNO_QUERY_THROW );

    // test that y error bars are there
    Reference< beans::XPropertySet > xErrorBarYProps;
    xPropSet->getPropertyValue(CHART_UNONAME_ERRORBAR_Y) >>= xErrorBarYProps;
    CPPUNIT_ASSERT(xErrorBarYProps.is());

    util::Color aColor(0);
    xErrorBarYProps->getPropertyValue("LineColor") >>= aColor;
    sal_uInt32 nColorValue = aColor;
    CPPUNIT_ASSERT_EQUAL(sal_uInt32(0xff3333), nColorValue);
}

// stepped line interpolation
void Chart2ImportTest::testSteppedLines()
{
    const sal_Int32 MAXSHEET = 14;
    chart2::CurveStyle const curveStyle[] = {
        chart2::CurveStyle_LINES,
        chart2::CurveStyle_CUBIC_SPLINES,
        chart2::CurveStyle_B_SPLINES,
        chart2::CurveStyle_STEP_START,
        chart2::CurveStyle_STEP_END,
        chart2::CurveStyle_STEP_CENTER_X,
        chart2::CurveStyle_STEP_CENTER_Y,
        chart2::CurveStyle_LINES,
        chart2::CurveStyle_CUBIC_SPLINES,
        chart2::CurveStyle_B_SPLINES,
        chart2::CurveStyle_STEP_START,
        chart2::CurveStyle_STEP_END,
        chart2::CurveStyle_STEP_CENTER_X,
        chart2::CurveStyle_STEP_CENTER_Y
    };

    load(u"/chart2/qa/extras/data/ods/", "stepped_lines.ods");
    for(sal_Int32 nSheet = 0; nSheet < MAXSHEET; ++nSheet)
    {
        uno::Reference< chart2::XChartDocument > xChart2Doc = getChartDocFromSheet( nSheet, mxComponent );
        CPPUNIT_ASSERT(xChart2Doc.is());

        Reference< chart2::XChartType > xChartType = getChartTypeFromDoc( xChart2Doc, 0 );
        CPPUNIT_ASSERT(xChartType.is());

        Reference< beans::XPropertySet > xPropSet( xChartType, UNO_QUERY );
        CPPUNIT_ASSERT(xPropSet.is());

        chart2::CurveStyle eCurveStyle;
        xPropSet->getPropertyValue("CurveStyle") >>= eCurveStyle;
        CPPUNIT_ASSERT_EQUAL(eCurveStyle, curveStyle[nSheet]);
    }
}

static uno::Sequence < OUString > getChartColumnDescriptions( uno::Reference< chart::XChartDocument > const & xChart1Doc)
{
    CPPUNIT_ASSERT(xChart1Doc.is());
    uno::Reference< chart::XChartDataArray > xChartData ( xChart1Doc->getData(), UNO_QUERY_THROW);
    uno::Sequence < OUString > seriesList = xChartData->getColumnDescriptions();
    return seriesList;
}

void Chart2ImportTest::testODSChartSeries()
{
    load(u"/chart2/qa/extras/data/ods/", "chart.ods");
    uno::Reference< chart::XChartDocument > xChart1Doc ( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW);
    uno::Sequence < OUString > seriesList = getChartColumnDescriptions( xChart1Doc);
    CPPUNIT_ASSERT_EQUAL(OUString("Col 1"), seriesList[0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Col2"), seriesList[1]);
    CPPUNIT_ASSERT_EQUAL(OUString("Col 33"), seriesList[2]);

}

void Chart2ImportTest::testXLSXChartSeries()
{
    load(u"/chart2/qa/extras/data/xlsx/", "chart.xlsx");
    uno::Reference< chart::XChartDocument > xChart1Doc ( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW);
    uno::Sequence < OUString > seriesList = getChartColumnDescriptions(xChart1Doc );
    CPPUNIT_ASSERT_EQUAL(OUString("Col 1"), seriesList[0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Col2"), seriesList[1]);
    CPPUNIT_ASSERT_EQUAL(OUString("Col 33"), seriesList[2]);

}

void Chart2ImportTest::testXLSChartSeries()
{
    load(u"/chart2/qa/extras/data/xls/", "chart.xls");
    uno::Reference< chart::XChartDocument > xChart1Doc ( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW);
    uno::Sequence < OUString > seriesList = getChartColumnDescriptions(xChart1Doc );
    CPPUNIT_ASSERT_EQUAL(OUString("Col 1"), seriesList[0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Col 2"), seriesList[1]);
    CPPUNIT_ASSERT_EQUAL(OUString("Col 3"), seriesList[2]);

}

void Chart2ImportTest::testODTChartSeries()
{
    load(u"/chart2/qa/extras/data/odt/", "chart.odt");
    uno::Sequence< OUString > seriesList = getWriterChartColumnDescriptions(mxComponent);
    CPPUNIT_ASSERT_EQUAL(OUString("Column 1"), seriesList[0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Column 2"), seriesList[1]);
    CPPUNIT_ASSERT_EQUAL(OUString("Column 3"), seriesList[2]);

}

void Chart2ImportTest::testDOCChartSeries()
{
    load(u"/chart2/qa/extras/data/doc/", "chart.doc");
    uno::Sequence< OUString > seriesList = getWriterChartColumnDescriptions(mxComponent);
    CPPUNIT_ASSERT_EQUAL(OUString("Column 1"), seriesList[0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Column 2"), seriesList[1]);
    CPPUNIT_ASSERT_EQUAL(OUString("Column 3"), seriesList[2]);
}

void Chart2ImportTest::testDOCXChartSeries()
{
    load(u"/chart2/qa/extras/data/docx/", "chart.docx");
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromWriter(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XChartType> xCT = getChartTypeFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xCT.is());

    std::vector<uno::Sequence<uno::Any> > aLabels = getDataSeriesLabelsFromChartType(xCT);
    CPPUNIT_ASSERT_EQUAL(size_t(3), aLabels.size());
    CPPUNIT_ASSERT_EQUAL(OUString("Series 1"), aLabels[0][0].get<OUString>());
    CPPUNIT_ASSERT_EQUAL(OUString("Series 2"), aLabels[1][0].get<OUString>());
    CPPUNIT_ASSERT_EQUAL(OUString("Series 3"), aLabels[2][0].get<OUString>());
}

void Chart2ImportTest::testDOCXChartEmptySeries()
{
    load(u"/chart2/qa/extras/data/docx/", "tdf125337.docx");
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromWriter(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XChartType> xCT = getChartTypeFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xCT.is());

    std::vector<uno::Sequence<uno::Any> > aLabels = getDataSeriesLabelsFromChartType(xCT);
    CPPUNIT_ASSERT_EQUAL(size_t(3), aLabels.size());
    CPPUNIT_ASSERT_EQUAL(OUString("1. dataseries"), aLabels[0][0].get<OUString>());
    CPPUNIT_ASSERT_EQUAL(OUString("2. dataseries"), aLabels[1][0].get<OUString>());
    CPPUNIT_ASSERT_EQUAL(OUString("Column 3"), aLabels[2][0].get<OUString>());

    //test chart series sparse data for docx
    std::vector<std::vector<double> > aValues = getDataSeriesYValuesFromChartType(xCT);
    CPPUNIT_ASSERT_EQUAL(size_t(3), aValues.size());
    //test the second series values
    CPPUNIT_ASSERT_EQUAL(2.4, aValues[1][0]);
    CPPUNIT_ASSERT_EQUAL(4.4, aValues[1][1]);
    //test the third series (empty) values
    CPPUNIT_ASSERT(std::isnan(aValues[2][0]));
    CPPUNIT_ASSERT(std::isnan(aValues[2][1]));
}

void Chart2ImportTest::testTdf81396()
{
    load(u"/chart2/qa/extras/data/xlsx/", "tdf81396.xlsx");
    Reference<chart::XChartDocument> xChartDoc(getChartDocFromSheet(0, mxComponent),
                                               UNO_QUERY_THROW);

    Reference<chart2::XChartDocument> xChartDoc2(xChartDoc, UNO_QUERY_THROW);
    Reference<chart2::XChartType> xChartType(getChartTypeFromDoc(xChartDoc2, 0), UNO_SET_THROW);
    std::vector aDataSeriesYValues = getDataSeriesYValuesFromChartType(xChartType);
    CPPUNIT_ASSERT_EQUAL(size_t(1), aDataSeriesYValues.size());

    // Without the fix in place, this test would have failed with
    // - Expected: 105.210801910481
    // - Actual  : nan
    CPPUNIT_ASSERT_EQUAL(105.210801910481, aDataSeriesYValues[0][0]);
}

void Chart2ImportTest::testDOCXChartValuesSize()
{
    load( u"/chart2/qa/extras/data/docx/", "bubblechart.docx" );
    Reference<chart2::XChartDocument> xChartDoc( getChartDocFromWriter(0), uno::UNO_QUERY );
    CPPUNIT_ASSERT( xChartDoc.is() );

    uno::Reference< chart::XChartDataArray > xDataArray( xChartDoc->getDataProvider(), UNO_QUERY_THROW );
    Sequence<OUString> aColumnDesc = xDataArray->getColumnDescriptions();
    // Number of columns = 3 (Y-values, X-values and bubble sizes).
    // Without the fix there would only be 2 columns (no bubble sizes).
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "There must be 3 columns and descriptions", static_cast<sal_Int32>(3), aColumnDesc.getLength() );
    Sequence<Sequence<double>> aData = xDataArray->getData();
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "There must be exactly 3 data points", static_cast<sal_Int32>(3), aData.getLength() );

    std::vector<std::vector<double>> aExpected = { { 2.7, 0.7, 10.0 }, { 3.2, 1.8, 4.0 }, { 0.8, 2.6, 8.0 } };

    for ( sal_Int32 nRowIdx = 0; nRowIdx < 3; ++nRowIdx )
        for( sal_Int32 nColIdx = 0; nColIdx < 3; ++nColIdx )
            CPPUNIT_ASSERT_DOUBLES_EQUAL( aExpected[nRowIdx][nColIdx], aData[nRowIdx][nColIdx], 1e-1 );
}

void Chart2ImportTest::testPPTChartSeries()
{
    //test chart series names for ppt
    uno::Sequence < OUString > seriesList = getImpressChartColumnDescriptions(u"/chart2/qa/extras/data/ppt/", "chart.ppt");

    CPPUNIT_ASSERT_EQUAL(OUString("Column 1"), seriesList[0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Column 2"), seriesList[1]);
    CPPUNIT_ASSERT_EQUAL(OUString("Column 3"), seriesList[2]);

}

void Chart2ImportTest::testPPTXChartSeries()
{
    //test chart series names for pptx
    load(u"/chart2/qa/extras/data/pptx/", "chart.pptx");
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XChartType> xCT = getChartTypeFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xCT.is());

    std::vector<uno::Sequence<uno::Any> > aLabels = getDataSeriesLabelsFromChartType(xCT);
    CPPUNIT_ASSERT_EQUAL(size_t(3), aLabels.size());
    CPPUNIT_ASSERT_EQUAL(OUString("Column 1"), aLabels[0][0].get<OUString>());
    CPPUNIT_ASSERT_EQUAL(OUString("Column 2"), aLabels[1][0].get<OUString>());
    CPPUNIT_ASSERT_EQUAL(OUString("Column 3"), aLabels[2][0].get<OUString>());
}

void Chart2ImportTest::testPPTXSparseChartSeries()
{
    //test chart series sparse data for pptx
    load(u"/chart2/qa/extras/data/pptx/", "sparse-chart.pptx");
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XChartType> xCT = getChartTypeFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xCT.is());

    std::vector<std::vector<double> > aValues = getDataSeriesYValuesFromChartType(xCT);
    CPPUNIT_ASSERT_EQUAL(size_t(2), aValues.size());
    CPPUNIT_ASSERT( std::isnan( aValues[0][0] ) );
    CPPUNIT_ASSERT_EQUAL(2.5,  aValues[0][1]);
    CPPUNIT_ASSERT_EQUAL(3.5,  aValues[0][2]);
    CPPUNIT_ASSERT( std::isnan( aValues[0][3] ) );
    CPPUNIT_ASSERT_EQUAL(-2.4, aValues[1][0]);
    CPPUNIT_ASSERT( std::isnan( aValues[1][1] ) );
    CPPUNIT_ASSERT( std::isnan( aValues[1][2] ) );
    CPPUNIT_ASSERT_EQUAL(-2.8, aValues[1][3]);
}

void Chart2ImportTest::testPPTXHiddenDataSeries()
{
    load(u"/chart2/qa/extras/data/pptx/", "stacked-bar-chart-hidden-series.pptx");
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    // "Automatic" chart background fill in pptx should be loaded as no fill.
    Reference<beans::XPropertySet> xPropSet = xChartDoc->getPageBackground();
    CPPUNIT_ASSERT(xPropSet.is());
    drawing::FillStyle eStyle = xPropSet->getPropertyValue("FillStyle").get<drawing::FillStyle>();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("'Automatic' chart background fill in pptx should be loaded as no fill (transparent).",
        drawing::FillStyle_NONE, eStyle);

    Reference<chart2::XChartType> xCT = getChartTypeFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xCT.is());

    // There should be only one data series present.
    std::vector<uno::Sequence<uno::Any> > aLabels = getDataSeriesLabelsFromChartType(xCT);
    CPPUNIT_ASSERT_EQUAL(size_t(1), aLabels.size());
    CPPUNIT_ASSERT_EQUAL(OUString("Series 3"), aLabels[0][0].get<OUString>());

    // Test the internal data.
    CPPUNIT_ASSERT(xChartDoc->hasInternalDataProvider());

    Reference<chart2::XInternalDataProvider> xInternalProvider(xChartDoc->getDataProvider(), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xInternalProvider.is());

    Reference<chart::XComplexDescriptionAccess> xDescAccess(xInternalProvider, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xDescAccess.is());

    // Get the category labels.
    Sequence<Sequence<OUString> > aCategories = xDescAccess->getComplexRowDescriptions();
    CPPUNIT_ASSERT_EQUAL(sal_Int32(4), aCategories.getLength());
    CPPUNIT_ASSERT_EQUAL(OUString("Category 1"), aCategories[0][0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Category 2"), aCategories[1][0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Category 3"), aCategories[2][0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Category 4"), aCategories[3][0]);
}

void Chart2ImportTest::testPPTXPercentageNumberFormats()
{
    load(u"/chart2/qa/extras/data/pptx/", "percentage-number-formats.pptx");

    // 1st chart
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    uno::Reference<chart2::XDataSeries> xDataSeries(getDataSeriesFromDoc(xChartDoc, 0));
    CPPUNIT_ASSERT(xDataSeries.is());
    uno::Reference<beans::XPropertySet> xPropertySet;
    chart2::DataPointLabel aLabel;
    sal_Int32 nNumberFormat;
    const sal_Int32 nPercentFormatSimple = getNumberFormat(xChartDoc, "0%");
    const sal_Int32 nPercentFormatDecimal = getNumberFormat(xChartDoc, "0.00%");

    xPropertySet.set(xDataSeries->getDataPointByIndex(0), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("Label") >>= aLabel;
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumber);
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumberInPercent);
    xPropertySet->getPropertyValue("PercentageNumberFormat") >>= nNumberFormat;
    CPPUNIT_ASSERT_EQUAL(nPercentFormatSimple, nNumberFormat);

    xPropertySet.set(xDataSeries->getDataPointByIndex(1), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("Label") >>= aLabel;
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumber);
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumberInPercent);
    xPropertySet->getPropertyValue("PercentageNumberFormat") >>= nNumberFormat;
    CPPUNIT_ASSERT_EQUAL(nPercentFormatDecimal, nNumberFormat);

    xPropertySet.set(xDataSeries->getDataPointByIndex(2), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("Label") >>= aLabel;
    CPPUNIT_ASSERT_EQUAL(sal_False, aLabel.ShowNumber);
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumberInPercent);
    xPropertySet->getPropertyValue("PercentageNumberFormat") >>= nNumberFormat;
    CPPUNIT_ASSERT_EQUAL(nPercentFormatSimple, nNumberFormat);

    xPropertySet.set(xDataSeries->getDataPointByIndex(3), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("Label") >>= aLabel;
    CPPUNIT_ASSERT_EQUAL(sal_False, aLabel.ShowNumber);
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumberInPercent);
    xPropertySet->getPropertyValue("PercentageNumberFormat") >>= nNumberFormat;
    CPPUNIT_ASSERT_EQUAL(nPercentFormatDecimal, nNumberFormat);

    // 2nd chart
    xChartDoc.set(getChartDocFromDrawImpress(1, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XAxis> xYAxis = getAxisFromDoc(xChartDoc, 0, 1, 0);
    CPPUNIT_ASSERT(xYAxis.is());

    Reference<beans::XPropertySet> xPS(xYAxis, uno::UNO_QUERY_THROW);
    bool bLinkNumberFormatToSource = true;
    bool bSuccess = xPS->getPropertyValue(CHART_UNONAME_LINK_TO_SRC_NUMFMT) >>= bLinkNumberFormatToSource;
    CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to false.", bSuccess);
    CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to false.", !bLinkNumberFormatToSource);

    // FIXME: This should be in fact "0.00%".
    // see TODO in oox/source/drawingml/chart/modelbase.cxx
    const sal_Int32 nPercentFormatDecimalShort = getNumberFormat(xChartDoc, "0.0%");
    nNumberFormat = getNumberFormatFromAxis(xYAxis);
    CPPUNIT_ASSERT_EQUAL(nPercentFormatDecimalShort, nNumberFormat);
    sal_Int16 nType = getNumberFormatType(xChartDoc, nNumberFormat);
    CPPUNIT_ASSERT_MESSAGE("Y axis should be a percent format.", (nType & util::NumberFormat::PERCENT));
}

void Chart2ImportTest::testPieChartLabelsNumFormat()
{
    load(u"/chart2/qa/extras/data/xlsx/", "tdfPieNumFormat.xlsx");
    uno::Reference< chart::XChartDocument > xChartDoc(getChartCompFromSheet(0, mxComponent), UNO_QUERY_THROW);
    CPPUNIT_ASSERT(xChartDoc.is());
    // test data point labels format
    Reference<beans::XPropertySet> xDataPointPropSet(xChartDoc->getDiagram()->getDataPointProperties(0, 0), uno::UNO_SET_THROW);
    chart2::DataPointLabel aLabel;
    xDataPointPropSet->getPropertyValue("Label") >>= aLabel;
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumber);
}

void Chart2ImportTest::testPPTXStackedNonStackedYAxis()
{
    load(u"/chart2/qa/extras/data/pptx/", "stacked-non-stacked-mix-y-axis.pptx");

    // 1st chart is a normal stacked column.
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XTitled> xTitled(xChartDoc, uno::UNO_QUERY_THROW);
    OUString aTitle = getTitleString(xTitled);
    CPPUNIT_ASSERT_EQUAL(OUString("Stacked"), aTitle);

    // Get the Y-axis.
    Reference<chart2::XAxis> xYAxis = getAxisFromDoc(xChartDoc, 0, 1, 0);
    CPPUNIT_ASSERT(xYAxis.is());

    sal_Int32 nNumberFormat = getNumberFormatFromAxis(xYAxis);
    sal_Int16 nType = getNumberFormatType(xChartDoc, nNumberFormat);
    CPPUNIT_ASSERT_MESSAGE("Y axis should be a normal number format.", (nType & util::NumberFormat::NUMBER));
    CPPUNIT_ASSERT_MESSAGE("Y axis should NOT be a percent format.", !(nType & util::NumberFormat::PERCENT));

    // 2nd chart is a percent-stacked column.
    xChartDoc.set(getChartDocFromDrawImpress(1, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    xTitled.set(xChartDoc, uno::UNO_QUERY_THROW);
    aTitle = getTitleString(xTitled);
    CPPUNIT_ASSERT_EQUAL(OUString("100% Stacked"), aTitle);

    // Get the Y-axis.
    xYAxis = getAxisFromDoc(xChartDoc, 0, 1, 0);
    CPPUNIT_ASSERT(xYAxis.is());

    // Get the number format of the Y-axis.
    nNumberFormat = getNumberFormatFromAxis(xYAxis);
    nType = getNumberFormatType(xChartDoc, nNumberFormat);
    CPPUNIT_ASSERT_MESSAGE("Y axis should be a percent format.", (nType & util::NumberFormat::PERCENT));

    // 3rd chart is a mixture of normal-stacked column with a percent-stacked
    // area chart series.  Excel in this case sets the Y-axis to be
    // non-percent axis and we should do the same for interoperability.
    xChartDoc.set(getChartDocFromDrawImpress(2, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    xTitled.set(xChartDoc, uno::UNO_QUERY_THROW);
    aTitle = getTitleString(xTitled);
    CPPUNIT_ASSERT_EQUAL(OUString("Stacked column mixed with 100% stacked area"), aTitle);

    // Get the Y-axis.
    xYAxis = getAxisFromDoc(xChartDoc, 0, 1, 0);
    CPPUNIT_ASSERT(xYAxis.is());

    // Get the number format of the Y-axis.
    nNumberFormat = getNumberFormatFromAxis(xYAxis);
    nType = getNumberFormatType(xChartDoc, nNumberFormat);
    CPPUNIT_ASSERT_MESSAGE("Y axis should be a normal number format.", (nType & util::NumberFormat::NUMBER));
    CPPUNIT_ASSERT_MESSAGE("Y axis should NOT be a percent format.", !(nType & util::NumberFormat::PERCENT));
}

void Chart2ImportTest::testODPChartSeries()
{
    //test chart series names for odp
    uno::Sequence < OUString > seriesList = getImpressChartColumnDescriptions(u"/chart2/qa/extras/data/odp/", "chart.odp");
    CPPUNIT_ASSERT_EQUAL(OUString("Column 1"), seriesList[0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Column 2"), seriesList[1]);
    CPPUNIT_ASSERT_EQUAL(OUString("Column 3"), seriesList[2]);

}

void Chart2ImportTest::testBnc864396()
{
    uno::Reference< chart2::XChartDocument > xChartDoc(getChartDocFromImpress(u"/chart2/qa/extras/data/pptx/", "bnc864396.pptx"), uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT(xChartDoc->hasInternalDataProvider());

    uno::Reference< chart2::XInternalDataProvider > xDataProvider( xChartDoc->getDataProvider(), uno::UNO_QUERY_THROW );
    uno::Reference< chart::XChartDataArray > xChartDataArray(xDataProvider, uno::UNO_QUERY_THROW);
    uno::Sequence< OUString > aRowLabels = xChartDataArray->getRowDescriptions();
    for(sal_Int32 i = 0; i < aRowLabels.getLength(); ++i)
    {
        OUString aExpected = "cat" + OUString::number(i+1);
        CPPUNIT_ASSERT_EQUAL(aExpected, aRowLabels[i]);
    }
}

void Chart2ImportTest::testBnc889755()
{
    load(u"/chart2/qa/extras/data/pptx/", "bnc889755.pptx");
    uno::Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 5), uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT(xChartDoc->hasInternalDataProvider());

    constexpr sal_Int32 nNumCategories = 16;
    Sequence<OUString> aDateSeq = getFormattedDateCategories(xChartDoc);

    CPPUNIT_ASSERT_EQUAL(nNumCategories, aDateSeq.getLength());

    const OUString aExpectedDateCategories[nNumCategories] = {
        "Oct-12", "Nov-12", "Dec-12", "Jan-13",
        "Feb-13", "Mar-13", "Apr-13", "May-13",
        "Jun-13", "Jul-13", "Aug-13", "Sep-13",
        "Oct-13", "Nov-13", "Dec-13", "Jan-14",
    };

    for (size_t nIdx = 0; nIdx < nNumCategories; ++nIdx)
        CPPUNIT_ASSERT_EQUAL(aExpectedDateCategories[nIdx], aDateSeq[nIdx]);

    //tdf#139940 - the title's gradient was lost and was filled with solid blue, instead of a "blue underline".
    uno::Reference<drawing::XDrawPagesSupplier> xDoc(mxComponent, uno::UNO_QUERY_THROW);
    uno::Reference<drawing::XDrawPage> xPage(xDoc->getDrawPages()->getByIndex(0), uno::UNO_QUERY_THROW);
    // Shape "Title 3"
    uno::Reference<beans::XPropertySet> xShapeProps(xPage->getByIndex(4), uno::UNO_QUERY_THROW);
    awt::Gradient aTransparence;
    xShapeProps->getPropertyValue("FillTransparenceGradient") >>= aTransparence;
    CPPUNIT_ASSERT(aTransparence.StartColor != aTransparence.EndColor);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0xffffff), aTransparence.StartColor);
}

void Chart2ImportTest::testBnc882383()
{
    load(u"/chart2/qa/extras/data/pptx/", "bnc882383.pptx");
    uno::Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY_THROW);
    uno::Reference<chart2::XDataSeries> xDataSeries(getDataSeriesFromDoc(xChartDoc, 0));
    CPPUNIT_ASSERT(xDataSeries.is());

    uno::Reference<beans::XPropertySet> xPropertySet(xDataSeries->getDataPointByIndex(0), uno::UNO_SET_THROW);
    OUString sGradientName;
    xPropertySet->getPropertyValue("GradientName") >>= sGradientName;
    CPPUNIT_ASSERT(!sGradientName.isEmpty());
}

void Chart2ImportTest::testTransparencyGradientValue()
{
    load(u"/chart2/qa/extras/data/xlsx/", "tdf128732.xlsx");
    uno::Reference< chart2::XChartDocument > xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT(xChartDoc.is());
    uno::Reference<chart2::XDataSeries> xDataSeries(getDataSeriesFromDoc(xChartDoc, 0));
    CPPUNIT_ASSERT(xDataSeries.is());

    uno::Reference<beans::XPropertySet> xPropertySet(xDataSeries->getDataPointByIndex(0), uno::UNO_SET_THROW);
    OUString sTranspGradientName;
    xPropertySet->getPropertyValue("FillTransparenceGradientName") >>= sTranspGradientName;
    CPPUNIT_ASSERT(!sTranspGradientName.isEmpty());

    awt::Gradient aTransparenceGradient;
    uno::Reference< lang::XMultiServiceFactory > xFact(xChartDoc, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xFact.is());
    uno::Reference< container::XNameAccess > xTransparenceGradient(xFact->createInstance("com.sun.star.drawing.TransparencyGradientTable"), uno::UNO_QUERY);
    uno::Any rTransparenceValue = xTransparenceGradient->getByName(sTranspGradientName);
    CPPUNIT_ASSERT(rTransparenceValue >>= aTransparenceGradient);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(3355443), aTransparenceGradient.EndColor);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(5000268), aTransparenceGradient.StartColor);
}

void Chart2ImportTest::testSimpleStrictXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "strict_chart.xlsx");
    uno::Reference< chart2::XChartDocument > xChartDoc = getChartDocFromSheet( 0, mxComponent );
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference< chart2::XDataSeries > xDataSeries = getDataSeriesFromDoc( xChartDoc, 0 );
    CPPUNIT_ASSERT(xDataSeries.is());

}

void Chart2ImportTest::testDelayedCellImport()
{
    load(u"/chart2/qa/extras/data/xlsx/", "fdo70609.xlsx");
    uno::Reference< chart2::XChartDocument > xChartDoc = getChartDocFromSheet( 0, mxComponent );
    Reference< chart2::data::XDataSequence > xDataSeq =
        getDataSequenceFromDocByRole(xChartDoc, u"values-x");

    OUString aRange = xDataSeq->getSourceRangeRepresentation();
    CPPUNIT_ASSERT_EQUAL(OUString("$Sheet2.$C$5:$C$9"), aRange);
}

void Chart2ImportTest::testFlatODSStackedColumnChart()
{
    load(u"/chart2/qa/extras/data/fods/", "stacked-column-chart.fods");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XChartType> xChartType = getChartTypeFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xChartType.is());

    Reference<chart2::XDataSeriesContainer> xDSCont(xChartType, UNO_QUERY);
    CPPUNIT_ASSERT(xDSCont.is());
    Sequence<Reference<chart2::XDataSeries> > aSeriesSeq = xDSCont->getDataSeries();

    // The stacked column chart should consist of 5 data series.
    CPPUNIT_ASSERT_EQUAL(sal_Int32(5), aSeriesSeq.getLength());
}

void Chart2ImportTest::testFdo78080()
{
    load(u"/chart2/qa/extras/data/xlsx/", "fdo78080.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XTitled> xTitled(xChartDoc, uno::UNO_QUERY_THROW);
    Reference<chart2::XTitle> xTitle = xTitled->getTitleObject();
    CPPUNIT_ASSERT(!xTitle.is());
}

void Chart2ImportTest::testTdf127811()
{
    load(u"/chart2/qa/extras/data/pptx/", "tdf127811.pptx");
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XChartType> xCT = getChartTypeFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xCT.is());

    std::vector<uno::Sequence<uno::Any> > aLabels = getDataSeriesLabelsFromChartType(xCT);
    CPPUNIT_ASSERT_EQUAL(size_t(2), aLabels.size());

    // Without the fix in place, this test would have failed with
    // - Expected: 1. first
    // - Actual  : 2. second
    CPPUNIT_ASSERT_EQUAL(OUString("1. first"), aLabels[0][0].get<OUString>());
    CPPUNIT_ASSERT_EQUAL(OUString("2. second"), aLabels[1][0].get<OUString>());
}

void Chart2ImportTest::testTdf86624()
{
    load(u"/chart2/qa/extras/data/ods/", "tdf86624.ods");
    uno::Reference< chart2::XChartDocument > xChart2Doc = getChartDocFromSheet(0, mxComponent);
    uno::Reference< chart::XChartDocument > xChartDoc (xChart2Doc, uno::UNO_QUERY);
    uno::Reference<drawing::XShape> xLegend = xChartDoc->getLegend();
    awt::Point aPos = xLegend->getPosition();
    CPPUNIT_ASSERT(aPos.X > 5000); // real value for me is above 8000 but before bug fix is below 1000
    CPPUNIT_ASSERT(aPos.Y > 4000); // real value for ms is above 7000
}

void Chart2ImportTest::testTdf105517()
{
    load(u"/chart2/qa/extras/data/pptx/", "tdf105517.pptx");
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XCoordinateSystemContainer> xCoordContainer(xChartDoc->getFirstDiagram(), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xCoordContainer.is());
    Reference<chart2::XChartTypeContainer> xChartTypeContainer(xCoordContainer->getCoordinateSystems()[0], uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartTypeContainer.is());
    Reference<chart2::XDataSeriesContainer> xDSContainer(xChartTypeContainer->getChartTypes()[0], uno::UNO_QUERY);
    CPPUNIT_ASSERT(xDSContainer.is());
    Reference<beans::XPropertySet> xPropSet1(xDSContainer->getDataSeries()[0], uno::UNO_QUERY);
    CPPUNIT_ASSERT(xPropSet1.is());

    tools::Long lineColor;
    xPropSet1->getPropertyValue("Color") >>= lineColor;
    // incorrect line color was 0x4a7ebb due to not handling themeOverride
    CPPUNIT_ASSERT_EQUAL(tools::Long(0xeaa700), lineColor);

    Reference<beans::XPropertySet> xPropSet2(xDSContainer->getDataSeries()[1], uno::UNO_QUERY);
    CPPUNIT_ASSERT(xPropSet2.is());

    xPropSet2->getPropertyValue("Color") >>= lineColor;
    // incorrect line color was 0x98b855
    CPPUNIT_ASSERT_EQUAL(tools::Long(0x1e69a8), lineColor);
}

void Chart2ImportTest::testTdf106217()
{
    load(u"/chart2/qa/extras/data/pptx/", "tdf106217.pptx");
    uno::Reference< chart::XChartDocument > xChartDoc = getChartDocFromDrawImpress(0, 0);
    CPPUNIT_ASSERT(xChartDoc.is());

    uno::Reference<drawing::XDrawPageSupplier> xDrawPageSupplier(xChartDoc, uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage = xDrawPageSupplier->getDrawPage();
    uno::Reference<drawing::XShape> xCircle(xDrawPage->getByIndex(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xCircle.is());

    uno::Reference<container::XNamed> xNamedShape(xCircle, uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Oval 1"), xNamedShape->getName());

    awt::Point aPosition = xCircle->getPosition();
    CPPUNIT_ASSERT_EQUAL(sal_Int32(6870), aPosition.X);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(7261), aPosition.Y);
    awt::Size aSize = xCircle->getSize();
    CPPUNIT_ASSERT_EQUAL(sal_Int32(2701), aSize.Width);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(2700), aSize.Height);
}

void Chart2ImportTest::testTdf108021()
{
    // Tdf108021 : To check TextBreak value is true.
    load(u"/chart2/qa/extras/data/ods/", "tdf108021.ods");
    uno::Reference< chart::XDiagram > mxDiagram;
    uno::Reference< beans::XPropertySet > xAxisProp;
    bool bTextBreak = false;
    uno::Reference< chart::XChartDocument > xChartDoc ( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW);
    mxDiagram.set(xChartDoc->getDiagram());
    CPPUNIT_ASSERT(mxDiagram.is());
    uno::Reference< chart::XAxisXSupplier > xAxisXSupp( mxDiagram, uno::UNO_QUERY );
    CPPUNIT_ASSERT(xAxisXSupp.is());
    xAxisProp = xAxisXSupp->getXAxis();
    xAxisProp->getPropertyValue("TextBreak") >>= bTextBreak;
    // Expected value of 'TextBreak' is true
    CPPUNIT_ASSERT(bTextBreak);
}

void Chart2ImportTest::testTdf100084()
{
    // The test file was created with IBM Cognos, make sure there is a diagram.
    load(u"/chart2/qa/extras/data/xlsx/", "tdf100084.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    Reference<beans::XPropertySet> xDiagram(xChartDoc->getFirstDiagram(), UNO_QUERY);
    CPPUNIT_ASSERT_MESSAGE("There should be a Diagram.", xDiagram.is());
}

void Chart2ImportTest::testTdf124817()
{
    load(u"/chart2/qa/extras/data/xlsx/", "tdf124817.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    uno::Reference<chart2::XDataSeries> xDataSeries;
    chart2::Symbol aSymblProp;

    // Check the symbol of data series 1 (marker style none)
    xDataSeries = getDataSeriesFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xDataSeries.is());
    uno::Reference<beans::XPropertySet> xPropSet_0(xDataSeries, uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT((xPropSet_0->getPropertyValue("Symbol") >>= aSymblProp));
    CPPUNIT_ASSERT_EQUAL(chart2::SymbolStyle_NONE, aSymblProp.Style);

    // Check the symbol of data series 2 (marker style square)
    xDataSeries = getDataSeriesFromDoc(xChartDoc, 1);
    CPPUNIT_ASSERT(xDataSeries.is());
    uno::Reference<beans::XPropertySet> xPropSet_1(xDataSeries, uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT((xPropSet_1->getPropertyValue("Symbol") >>= aSymblProp));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0xED7D31), aSymblProp.FillColor);

    // Check the symbol of data series 3 (marker style diagonal cross)
    xDataSeries = getDataSeriesFromDoc(xChartDoc, 2);
    CPPUNIT_ASSERT(xDataSeries.is());
    uno::Reference<beans::XPropertySet> xPropSet_2(xDataSeries, uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT((xPropSet_2->getPropertyValue("Symbol") >>= aSymblProp));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0xFF0000), aSymblProp.BorderColor);
}

void Chart2ImportTest::testTdf126033()
{
    load(u"/chart2/qa/extras/data/xlsx/", "tdf126033.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    // Check symbol style and size of data points
    chart2::Symbol aSymblProp;
    uno::Reference<chart2::XDataSeries> xDataSeries(getDataSeriesFromDoc(xChartDoc, 0));
    CPPUNIT_ASSERT(xDataSeries.is());
    uno::Reference<beans::XPropertySet> xPropertySet(xDataSeries->getDataPointByIndex(0), uno::UNO_SET_THROW);
    CPPUNIT_ASSERT(xPropertySet->getPropertyValue("Symbol") >>= aSymblProp);
    CPPUNIT_ASSERT_EQUAL(chart2::SymbolStyle_NONE, aSymblProp.Style);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(176), aSymblProp.Size.Width);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(176), aSymblProp.Size.Height);
}

void Chart2ImportTest::testTransparentBackground(OUString const & filename)
{
    load(u"/chart2/qa/extras/data/xlsx/", filename);
    uno::Reference< chart2::XChartDocument > xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    uno::Reference< chart::XChartDocument > xChart2Doc (xChartDoc, uno::UNO_QUERY);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChart2Doc.is());

    Reference< beans::XPropertySet > xPropSet = xChart2Doc->getArea();
    CPPUNIT_ASSERT_MESSAGE("failed to get Area", xPropSet.is());

    css::drawing::FillStyle aStyle;
    xPropSet -> getPropertyValue("FillStyle") >>= aStyle;

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Background needs to be with solid fill style", css::drawing::FillStyle_SOLID, aStyle);
}
// 2 test methods here so that tearDown() can dispose the document
void Chart2ImportTest::testFdo54361()
{
    testTransparentBackground("fdo54361.xlsx");
}
void Chart2ImportTest::testFdo54361_1()
{
    testTransparentBackground("fdo54361-1.xlsx");
}

void Chart2ImportTest::testAutoBackgroundXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "chart-auto-background.xlsx");
    uno::Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    // "Automatic" chart background fill in xlsx should be loaded as solid white.
    Reference<beans::XPropertySet> xPropSet = xChartDoc->getPageBackground();
    CPPUNIT_ASSERT(xPropSet.is());
    drawing::FillStyle eStyle = xPropSet->getPropertyValue("FillStyle").get<drawing::FillStyle>();
    sal_Int32 nColor = xPropSet->getPropertyValue("FillColor").get<sal_Int32>();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("'Automatic' chart background fill in xlsx should be loaded as solid fill.",
        drawing::FillStyle_SOLID, eStyle);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("'Automatic' chart background fill in xlsx should be loaded as solid white.",
        sal_Int32(0x00FFFFFF), sal_Int32(nColor & 0x00FFFFFF)); // highest 2 bytes are transparency which we ignore here.
}

void Chart2ImportTest::testAutoChartAreaBorderPropXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "chart-area-style-border.xlsx");
    uno::Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    // Test "Automatic" chartarea border style/color/width.
    Reference<beans::XPropertySet> xPropSet = xChartDoc->getPageBackground();
    CPPUNIT_ASSERT(xPropSet.is());
    drawing::LineStyle eStyle = xPropSet->getPropertyValue("LineStyle").get<drawing::LineStyle>();
    sal_Int32 nColor = xPropSet->getPropertyValue("LineColor").get<sal_Int32>();
    sal_Int32 nWidth = xPropSet->getPropertyValue("LineWidth").get<sal_Int32>();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("'Automatic' chartarea border should be loaded as solid style.",
        drawing::LineStyle_SOLID, eStyle);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("'Automatic' chartarea border color should be loaded as light gray.",
        sal_Int32(0xD9D9D9), nColor);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("'Automatic' chartarea border width should be loaded as 0.75 pt (~0.026 cm)",
        sal_Int32(26), nWidth);
}

void Chart2ImportTest::testChartAreaStyleBackgroundXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "chart-area-style-background.xlsx");
    uno::Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    // "Automatic" chart background fill in xlsx should be loaded as solid white.
    Reference<beans::XPropertySet> xPropSet = xChartDoc->getPageBackground();
    CPPUNIT_ASSERT(xPropSet.is());
    drawing::FillStyle eStyle = xPropSet->getPropertyValue("FillStyle").get<drawing::FillStyle>();
    sal_Int32 nColor = xPropSet->getPropertyValue("FillColor").get<sal_Int32>();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("'Automatic' chart background fill in xlsx should be loaded as solid fill.",
        drawing::FillStyle_SOLID, eStyle);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("'Automatic' chart background fill in xlsx should be loaded as solid white.",
        sal_Int32(0), nColor);
}

void Chart2ImportTest::testChartHatchFillXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "chart-hatch-fill.xlsx");
    uno::Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    // Check the chart background FillStyle is HATCH
    Reference<beans::XPropertySet> xPropSet = xChartDoc->getPageBackground();
    CPPUNIT_ASSERT(xPropSet.is());
    drawing::FillStyle eStyle = xPropSet->getPropertyValue("FillStyle").get<drawing::FillStyle>();

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Chart background fill in this xlsx should be loaded as hatch fill.",
        drawing::FillStyle_HATCH, eStyle);

    // Check the FillBackground of chart background
    bool bBackgroundFill = false;
    xPropSet->getPropertyValue("FillBackground") >>= bBackgroundFill;
    CPPUNIT_ASSERT(bBackgroundFill);

    Color nBackgroundColor;
    xPropSet->getPropertyValue("FillColor") >>= nBackgroundColor;
    CPPUNIT_ASSERT_EQUAL(COL_WHITE, nBackgroundColor);

    // Check the datapoint has HatchName value
    uno::Reference<chart2::XDataSeries> xDataSeries(getDataSeriesFromDoc(xChartDoc, 0));
    CPPUNIT_ASSERT(xDataSeries.is());

    uno::Reference<beans::XPropertySet> xPropertySet(xDataSeries->getDataPointByIndex(1), uno::UNO_SET_THROW);
    OUString sHatchName;
    xPropertySet->getPropertyValue("HatchName") >>= sHatchName;
    CPPUNIT_ASSERT(!sHatchName.isEmpty());

    // Check the FillBackground of datapoint
    bool bBackgroundFillofDatapoint = false;
    xPropertySet->getPropertyValue("FillBackground") >>= bBackgroundFillofDatapoint;
    CPPUNIT_ASSERT(bBackgroundFillofDatapoint);

    sal_Int32 nBackgroundColorofDatapoint;
    xPropertySet->getPropertyValue("FillColor") >>= nBackgroundColorofDatapoint;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0x00B050), nBackgroundColorofDatapoint);
}

void Chart2ImportTest::testAxisTextRotationXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "axis-label-rotation.xlsx");
    uno::Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    Reference<chart2::XAxis> xYAxis = getAxisFromDoc(xChartDoc, 0, 0, 0);
    CPPUNIT_ASSERT(xYAxis.is());

    Reference<beans::XPropertySet> xPS(xYAxis, uno::UNO_QUERY_THROW);
    double nRotation = 0;
    bool bSuccess = xPS->getPropertyValue("TextRotation") >>= nRotation;

    CPPUNIT_ASSERT(bSuccess);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(90, nRotation, 1e-10);
}

/* TODO : temporarily disabled.
void Chart2ImportTest::testTextCanOverlapXLSX()
{
    // fdo#84647 : To check textoverlap value is imported correctly.
    load("/chart2/qa/extras/data/xlsx/", "chart-text-can-overlap.xlsx");
    uno::Reference< chart::XDiagram > mxDiagram;
    uno::Reference< beans::XPropertySet > xAxisProp;
    bool bTextCanOverlap = false;
    uno::Reference< chart::XChartDocument > xChartDoc ( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW);
    mxDiagram.set(xChartDoc->getDiagram());
    CPPUNIT_ASSERT(mxDiagram.is());
    uno::Reference< chart::XAxisXSupplier > xAxisXSupp( mxDiagram, uno::UNO_QUERY );
    CPPUNIT_ASSERT(xAxisXSupp.is());
    xAxisProp = xAxisXSupp->getXAxis();
    xAxisProp->getPropertyValue("TextCanOverlap") >>= bTextCanOverlap;
    // Expected value of 'TextCanOverlap' is true
    CPPUNIT_ASSERT(bTextCanOverlap);
}
*/

void Chart2ImportTest::testTextBreakXLSX()
{
    // tdf#122091: To check textbreak value is true in case of 0° degree of Axis label rotation.
    load(u"/chart2/qa/extras/data/xlsx/", "chart_label_text_break.xlsx");
    uno::Reference< chart::XDiagram > mxDiagram;
    uno::Reference< beans::XPropertySet > xAxisProp;
    bool textBreak = false;
    uno::Reference< chart::XChartDocument > xChartDoc ( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW);
    CPPUNIT_ASSERT(xChartDoc.is());
    mxDiagram.set(xChartDoc->getDiagram());
    CPPUNIT_ASSERT(mxDiagram.is());
    uno::Reference< chart::XAxisXSupplier > xAxisXSupp( mxDiagram, uno::UNO_QUERY );
    CPPUNIT_ASSERT(xAxisXSupp.is());
    xAxisProp = xAxisXSupp->getXAxis();
    xAxisProp->getPropertyValue("TextBreak") >>= textBreak;
    // Expected value of 'TextBreak' is true
    CPPUNIT_ASSERT(textBreak);
}

void Chart2ImportTest::testNumberFormatsXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "number-formats.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    uno::Reference<chart2::XDataSeries> xDataSeries(getDataSeriesFromDoc(xChartDoc, 0));
    CPPUNIT_ASSERT(xDataSeries.is());
    uno::Reference<beans::XPropertySet> xPropertySet;
    chart2::DataPointLabel aLabel;
    sal_Int32 nNumberFormat;
    bool bLinkNumberFormatToSource = false;
    const sal_Int32 nChartDataNumberFormat = getNumberFormat(
            xChartDoc, "_(\"$\"* #,##0_);_(\"$\"* \\(#,##0\\);_(\"$\"* \"-\"??_);_(@_)");

    xPropertySet.set(xDataSeries->getDataPointByIndex(0), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("Label") >>= aLabel;
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumber);
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumberInPercent);
    xPropertySet->getPropertyValue(CHART_UNONAME_NUMFMT) >>= nNumberFormat;
    CPPUNIT_ASSERT_EQUAL(nChartDataNumberFormat, nNumberFormat);
    bool bSuccess = xPropertySet->getPropertyValue("PercentageNumberFormat") >>= nNumberFormat;
    CPPUNIT_ASSERT_EQUAL(false, bSuccess);
    bSuccess = xPropertySet->getPropertyValue(CHART_UNONAME_LINK_TO_SRC_NUMFMT) >>= bLinkNumberFormatToSource;
    CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to true.", bSuccess);
    CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to true.", bLinkNumberFormatToSource);

    xPropertySet.set(xDataSeries->getDataPointByIndex(1), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("Label") >>= aLabel;
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumber);
    CPPUNIT_ASSERT_EQUAL(sal_False, aLabel.ShowNumberInPercent);
    xPropertySet->getPropertyValue(CHART_UNONAME_NUMFMT) >>= nNumberFormat;
    CPPUNIT_ASSERT_EQUAL(nChartDataNumberFormat, nNumberFormat);
    bSuccess = xPropertySet->getPropertyValue("PercentageNumberFormat") >>= nNumberFormat;
    CPPUNIT_ASSERT_EQUAL(false, bSuccess);
    bSuccess = xPropertySet->getPropertyValue(CHART_UNONAME_LINK_TO_SRC_NUMFMT) >>= bLinkNumberFormatToSource;
    CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to true.", bSuccess);
    CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to true.", bLinkNumberFormatToSource);

    xPropertySet.set(xDataSeries->getDataPointByIndex(2), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("Label") >>= aLabel;
    CPPUNIT_ASSERT_EQUAL(sal_False, aLabel.ShowNumber);
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumberInPercent);
    xPropertySet->getPropertyValue(CHART_UNONAME_NUMFMT) >>= nNumberFormat;
    CPPUNIT_ASSERT_EQUAL(nChartDataNumberFormat, nNumberFormat);
    bSuccess = xPropertySet->getPropertyValue("PercentageNumberFormat") >>= nNumberFormat;
    CPPUNIT_ASSERT_EQUAL(false, bSuccess);
    bSuccess = xPropertySet->getPropertyValue(CHART_UNONAME_LINK_TO_SRC_NUMFMT) >>= bLinkNumberFormatToSource;
    CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to true.", bSuccess);
    CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to true.", bLinkNumberFormatToSource);
}

void Chart2ImportTest::testNumberFormatsDOCX()
{
    load(u"/chart2/qa/extras/data/docx/", "tdf132174.docx");
    {
        uno::Reference< chart2::XChartDocument > xChartDoc(getChartDocFromWriter(0), uno::UNO_QUERY);
        CPPUNIT_ASSERT(xChartDoc.is());

        css::uno::Reference<chart2::XDiagram> xDiagram(xChartDoc->getFirstDiagram(), UNO_SET_THROW);
        Reference<chart2::XDataSeries> xDataSeries = getDataSeriesFromDoc(xChartDoc, 0);
        uno::Reference<beans::XPropertySet> xPropertySet(xDataSeries, uno::UNO_QUERY_THROW);
        CPPUNIT_ASSERT(xPropertySet.is());

        sal_Int32 nNumberFormat;
        bool bLinkNumberFormatToSource = true;
        const sal_Int32 nChartDataNumberFormat = getNumberFormat(xChartDoc, "0%");
        xPropertySet->getPropertyValue(CHART_UNONAME_NUMFMT) >>= nNumberFormat;
        CPPUNIT_ASSERT_EQUAL(nChartDataNumberFormat, nNumberFormat);
        xPropertySet->getPropertyValue(CHART_UNONAME_LINK_TO_SRC_NUMFMT) >>= bLinkNumberFormatToSource;
        // LinkNumberFormatToSource should be set to false even if the original OOXML contain a true value,
        // because the inner data table of charts have no own number format!
        CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to false.", !bLinkNumberFormatToSource);
    }

    load(u"/chart2/qa/extras/data/docx/", "tdf136650.docx");
    {
        uno::Reference< chart2::XChartDocument > xChartDoc(getChartDocFromWriter(0), uno::UNO_QUERY);
        CPPUNIT_ASSERT(xChartDoc.is());

        css::uno::Reference<chart2::XDiagram> xDiagram(xChartDoc->getFirstDiagram(), UNO_SET_THROW);
        Reference<chart2::XDataSeries> xDataSeries = getDataSeriesFromDoc(xChartDoc, 0);
        CPPUNIT_ASSERT(xDataSeries.is());
        Reference<beans::XPropertySet> xPropertySet(xDataSeries->getDataPointByIndex(1), uno::UNO_SET_THROW);

        sal_Int32 nNumberFormat;
        bool bLinkNumberFormatToSource = true;
        const sal_Int32 nChartDataNumberFormat = getNumberFormat(xChartDoc, "0%");
        xPropertySet->getPropertyValue(CHART_UNONAME_NUMFMT) >>= nNumberFormat;
        CPPUNIT_ASSERT_EQUAL(nChartDataNumberFormat, nNumberFormat);
        xPropertySet->getPropertyValue(CHART_UNONAME_LINK_TO_SRC_NUMFMT) >>= bLinkNumberFormatToSource;
        // LinkNumberFormatToSource should be set to false even if the original OOXML file contain a true value,
        // because the inner data table of charts have no own number format!
        CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to false.", !bLinkNumberFormatToSource);
    }
}

void Chart2ImportTest::testPercentageNumberFormatsDOCX()
{
    load(u"/chart2/qa/extras/data/docx/", "tdf133632.docx");
    uno::Reference< chart2::XChartDocument > xChartDoc(getChartDocFromWriter(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    css::uno::Reference<chart2::XDiagram> xDiagram(xChartDoc->getFirstDiagram(), UNO_SET_THROW);
    Reference<chart2::XDataSeries> xDataSeries = getDataSeriesFromDoc(xChartDoc, 0);
    uno::Reference<beans::XPropertySet> xPropertySet(xDataSeries, uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT(xPropertySet.is());

    bool bLinkNumberFormatToSource = false;
    chart2::DataPointLabel aLabel;
    xPropertySet->getPropertyValue("Label") >>= aLabel;
    CPPUNIT_ASSERT_EQUAL(sal_False, aLabel.ShowNumber);
    CPPUNIT_ASSERT_EQUAL(sal_True, aLabel.ShowNumberInPercent);
    bool bSuccess = xPropertySet->getPropertyValue(CHART_UNONAME_LINK_TO_SRC_NUMFMT) >>= bLinkNumberFormatToSource;
    CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to true.", bSuccess);
    CPPUNIT_ASSERT_MESSAGE("\"LinkNumberFormatToSource\" should be set to true.", bLinkNumberFormatToSource);
}

void Chart2ImportTest::testAutoTitleDelDefaultValue2007XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "autotitledel_2007.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    Reference<chart2::XTitled> xTitled(xChartDoc, uno::UNO_QUERY_THROW);
    OUString aTitle = getTitleString(xTitled);
    CPPUNIT_ASSERT_MESSAGE("autoTitleDel default value is false in MSO 2007 documents",
            !aTitle.isEmpty());
}

void Chart2ImportTest::testAutoTitleDelDefaultValue2013XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "autotitledel_2013.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    Reference<chart2::XTitled> xTitled(xChartDoc, uno::UNO_QUERY_THROW);
    uno::Reference<chart2::XTitle> xTitle = xTitled->getTitleObject();
    CPPUNIT_ASSERT_MESSAGE("autoTitleDel default value is true in the OOXML spec",
            !xTitle.is());
}

void Chart2ImportTest::testDispBlanksAsDefaultValue2007XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "dispBlanksAs_2007.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    Reference<beans::XPropertySet> xDiagram(xChartDoc->getFirstDiagram(), UNO_QUERY);
    CPPUNIT_ASSERT(xDiagram.is());
    uno::Any aAny = xDiagram->getPropertyValue("MissingValueTreatment");
    sal_Int32 nMissingValueTreatment = -2;
    CPPUNIT_ASSERT(aAny >>= nMissingValueTreatment);
    CPPUNIT_ASSERT_EQUAL(chart::MissingValueTreatment::LEAVE_GAP, nMissingValueTreatment);
}

void Chart2ImportTest::testDispBlanksAsDefaultValue2013XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "dispBlanksAs_2013.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());

    Reference<beans::XPropertySet> xDiagram(xChartDoc->getFirstDiagram(), UNO_QUERY);
    CPPUNIT_ASSERT(xDiagram.is());
    uno::Any aAny = xDiagram->getPropertyValue("MissingValueTreatment");
    sal_Int32 nMissingValueTreatment = -2;
    CPPUNIT_ASSERT(aAny >>= nMissingValueTreatment);
    CPPUNIT_ASSERT_EQUAL(chart::MissingValueTreatment::USE_ZERO, nMissingValueTreatment);
}

void Chart2ImportTest::testSmoothDefaultValue2007XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "smoothed_series2007.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference< chart2::XChartType > xChartType = getChartTypeFromDoc( xChartDoc, 0 );
    CPPUNIT_ASSERT(xChartType.is());

    Reference< beans::XPropertySet > xPropSet( xChartType, UNO_QUERY );
    CPPUNIT_ASSERT(xPropSet.is());

    chart2::CurveStyle eCurveStyle;
    xPropSet->getPropertyValue("CurveStyle") >>= eCurveStyle;
    CPPUNIT_ASSERT_EQUAL(chart2::CurveStyle_LINES, eCurveStyle);
}

void Chart2ImportTest::testSmoothDefaultValue2013XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "smoothed_series.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference< chart2::XChartType > xChartType = getChartTypeFromDoc( xChartDoc, 0 );
    CPPUNIT_ASSERT(xChartType.is());

    Reference< beans::XPropertySet > xPropSet( xChartType, UNO_QUERY );
    CPPUNIT_ASSERT(xPropSet.is());

    chart2::CurveStyle eCurveStyle;
    xPropSet->getPropertyValue("CurveStyle") >>= eCurveStyle;
    CPPUNIT_ASSERT(eCurveStyle != chart2::CurveStyle_LINES);
}

void Chart2ImportTest::testTrendlineDefaultValue2007XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "trendline2007.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    CPPUNIT_ASSERT(xChartDoc.is());
    Reference<chart2::XDataSeries> xDataSeries = getDataSeriesFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xDataSeries.is());
    Reference<chart2::XRegressionCurveContainer> xRegressionCurveContainer(xDataSeries, UNO_QUERY_THROW);
    Sequence< Reference<chart2::XRegressionCurve> > xRegressionCurveSequence = xRegressionCurveContainer->getRegressionCurves();
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), xRegressionCurveSequence.getLength());

    Reference<chart2::XRegressionCurve> xCurve = xRegressionCurveSequence[0];

    Reference<beans::XPropertySet> xPropSet(xCurve->getEquationProperties(), uno::UNO_SET_THROW);
    uno::Any aAny = xPropSet->getPropertyValue("ShowEquation");
    bool bShowEquation = true;
    CPPUNIT_ASSERT(aAny >>= bShowEquation);
    CPPUNIT_ASSERT(!bShowEquation);

    aAny = xPropSet->getPropertyValue("ShowCorrelationCoefficient");
    bool bShowCorrelation = true;
    CPPUNIT_ASSERT(aAny >>= bShowCorrelation);
    CPPUNIT_ASSERT(!bShowCorrelation);
}

void Chart2ImportTest::testTrendlineDefaultValue2013XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "trendline.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    CPPUNIT_ASSERT(xChartDoc.is());
    Reference<chart2::XDataSeries> xDataSeries = getDataSeriesFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xDataSeries.is());
    Reference<chart2::XRegressionCurveContainer> xRegressionCurveContainer(xDataSeries, UNO_QUERY_THROW);
    Sequence< Reference<chart2::XRegressionCurve> > xRegressionCurveSequence = xRegressionCurveContainer->getRegressionCurves();
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), xRegressionCurveSequence.getLength());

    Reference<chart2::XRegressionCurve> xCurve = xRegressionCurveSequence[0];

    Reference<beans::XPropertySet> xPropSet(xCurve->getEquationProperties(), uno::UNO_SET_THROW);
    uno::Any aAny = xPropSet->getPropertyValue("ShowEquation");
    bool bShowEquation = false;
    CPPUNIT_ASSERT(aAny >>= bShowEquation);
    CPPUNIT_ASSERT(bShowEquation);

    aAny = xPropSet->getPropertyValue("ShowCorrelationCoefficient");
    bool bShowCorrelation = false;
    CPPUNIT_ASSERT(aAny >>= bShowCorrelation);
    CPPUNIT_ASSERT(bShowCorrelation);
}

void Chart2ImportTest::testVaryColorDefaultValues2007XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "vary_color2007.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    CPPUNIT_ASSERT(xChartDoc.is());
    Reference<chart2::XDataSeries> xDataSeries = getDataSeriesFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xDataSeries.is());
    Reference<beans::XPropertySet> xPropSet(xDataSeries, uno::UNO_QUERY_THROW);
    uno::Any aAny = xPropSet->getPropertyValue("VaryColorsByPoint");
    bool bVaryColor = true;
    CPPUNIT_ASSERT(aAny >>= bVaryColor);
    CPPUNIT_ASSERT(!bVaryColor);
}

void Chart2ImportTest::testVaryColorDefaultValues2013XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "vary_color.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    CPPUNIT_ASSERT(xChartDoc.is());
    Reference<chart2::XDataSeries> xDataSeries = getDataSeriesFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xDataSeries.is());
    Reference<beans::XPropertySet> xPropSet(xDataSeries, uno::UNO_QUERY_THROW);
    uno::Any aAny = xPropSet->getPropertyValue("VaryColorsByPoint");
    bool bVaryColor = false;
    CPPUNIT_ASSERT(aAny >>= bVaryColor);
    CPPUNIT_ASSERT(!bVaryColor);
}

void Chart2ImportTest::testPlotVisOnlyDefaultValue2013XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "plotVisOnly.xlsx");
    uno::Reference< chart::XChartDocument > xChart1Doc ( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW);
    Reference<beans::XPropertySet> xPropSet(xChart1Doc->getDiagram(), uno::UNO_QUERY_THROW);
    uno::Any aAny = xPropSet->getPropertyValue("IncludeHiddenCells");
    CPPUNIT_ASSERT(aAny.hasValue());
    bool bShowHiddenValues = true;
    CPPUNIT_ASSERT(aAny >>= bShowHiddenValues);
    CPPUNIT_ASSERT(!bShowHiddenValues);
}

void Chart2ImportTest::testRAngAxDefaultValue2013XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "rAngAx.xlsx");
    uno::Reference< chart::XChartDocument > xChart1Doc ( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW);
    Reference<beans::XPropertySet> xPropSet(xChart1Doc->getDiagram(), uno::UNO_QUERY_THROW);
    uno::Any aAny = xPropSet->getPropertyValue("RightAngledAxes");
    CPPUNIT_ASSERT(aAny.hasValue());
    bool bRightAngleAxes = false;
    CPPUNIT_ASSERT(aAny >>= bRightAngleAxes);
    CPPUNIT_ASSERT(bRightAngleAxes);
}

void Chart2ImportTest::testMajorTickMarksDefaultValue2013XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "majorTickMark.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    Reference<chart2::XAxis> xXAxis = getAxisFromDoc(xChartDoc, 0, 0, 0);
    CPPUNIT_ASSERT(xXAxis.is());
    Reference<beans::XPropertySet> xPropSet(xXAxis, uno::UNO_QUERY_THROW);
    uno::Any aAny = xPropSet->getPropertyValue("MajorTickmarks");
    sal_Int32 nMajorTickmarks = chart2::TickmarkStyle::NONE;
    CPPUNIT_ASSERT(aAny.hasValue());
    CPPUNIT_ASSERT(aAny >>= nMajorTickmarks);
    CPPUNIT_ASSERT_EQUAL(chart2::TickmarkStyle::INNER | chart2::TickmarkStyle::OUTER, nMajorTickmarks);
}

void Chart2ImportTest::testMinorTickMarksDefaultValue2013XLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "minorTickMark.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    Reference<chart2::XAxis> xXAxis = getAxisFromDoc(xChartDoc, 0, 0, 0);
    CPPUNIT_ASSERT(xXAxis.is());
    Reference<beans::XPropertySet> xPropSet(xXAxis, uno::UNO_QUERY_THROW);
    uno::Any aAny = xPropSet->getPropertyValue("MinorTickmarks");
    sal_Int32 nMajorTickmarks = chart2::TickmarkStyle::NONE;
    CPPUNIT_ASSERT(aAny.hasValue());
    CPPUNIT_ASSERT(aAny >>= nMajorTickmarks);
    CPPUNIT_ASSERT_EQUAL(chart2::TickmarkStyle::INNER | chart2::TickmarkStyle::OUTER, nMajorTickmarks);
}

void Chart2ImportTest::testAxisTitleDefaultRotationXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "axis_title_default_rotation.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    Reference<chart2::XAxis> xYAxis = getAxisFromDoc(xChartDoc, 0, 1, 0);
    CPPUNIT_ASSERT(xYAxis.is());
    Reference<chart2::XTitled> xTitled(xYAxis, uno::UNO_QUERY_THROW);
    Reference<chart2::XTitle> xTitle = xTitled->getTitleObject();
    CPPUNIT_ASSERT(xTitle.is());
    Reference<beans::XPropertySet> xPropSet(xTitle, uno::UNO_QUERY_THROW);
    uno::Any aAny = xPropSet->getPropertyValue("TextRotation");
    double nRotation = 0;
    CPPUNIT_ASSERT(aAny >>= nRotation);
    CPPUNIT_ASSERT_EQUAL(90.0, nRotation);
}

void Chart2ImportTest::testSecondaryAxisTitleDefaultRotationXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "secondary_axis_title_default_rotation.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    Reference<chart2::XAxis> xYAxis = getAxisFromDoc(xChartDoc, 0, 1, 1);
    CPPUNIT_ASSERT(xYAxis.is());
    Reference<chart2::XTitled> xTitled(xYAxis, uno::UNO_QUERY_THROW);
    Reference<chart2::XTitle> xTitle = xTitled->getTitleObject();
    CPPUNIT_ASSERT(xTitle.is());
    Reference<beans::XPropertySet> xPropSet(xTitle, uno::UNO_QUERY_THROW);
    uno::Any aAny = xPropSet->getPropertyValue("TextRotation");
    double nRotation = 0;
    CPPUNIT_ASSERT(aAny >>= nRotation);
    CPPUNIT_ASSERT_EQUAL(90.0, nRotation);
}

void Chart2ImportTest::testAxisTitleRotationXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "axis_title_rotated.xlsx");
    Reference<chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    {
        Reference<chart2::XAxis> xYAxis = getAxisFromDoc(xChartDoc, 0, 1, 0);
        CPPUNIT_ASSERT(xYAxis.is());
        Reference<chart2::XTitled> xTitled(xYAxis, uno::UNO_QUERY_THROW);
        Reference<chart2::XTitle> xTitle = xTitled->getTitleObject();
        CPPUNIT_ASSERT(xTitle.is());
        Reference<beans::XPropertySet> xPropSet(xTitle, uno::UNO_QUERY_THROW);
        uno::Any aAny = xPropSet->getPropertyValue("TextRotation");
        double nRotation = 0;
        CPPUNIT_ASSERT(aAny >>= nRotation);
        CPPUNIT_ASSERT_EQUAL(340.0, nRotation);
    }
    {
        Reference<chart2::XAxis> xYAxis = getAxisFromDoc(xChartDoc, 0, 1, 1);
        CPPUNIT_ASSERT(xYAxis.is());
        Reference<chart2::XTitled> xTitled(xYAxis, uno::UNO_QUERY_THROW);
        Reference<chart2::XTitle> xTitle = xTitled->getTitleObject();
        CPPUNIT_ASSERT(xTitle.is());
        Reference<beans::XPropertySet> xPropSet(xTitle, uno::UNO_QUERY_THROW);
        uno::Any aAny = xPropSet->getPropertyValue("TextRotation");
        double nRotation = 0;
        CPPUNIT_ASSERT(aAny >>= nRotation);
        CPPUNIT_ASSERT_EQUAL(270.0, nRotation);
    }

}

void Chart2ImportTest::testAxisTitlePositionDOCX()
{
    load(u"/chart2/qa/extras/data/docx/", "testAxisTitlePosition.docx");
    uno::Reference< chart::XDiagram > mxDiagram;
    uno::Reference< drawing::XShape > xAxisTitle;
    uno::Reference< chart::XChartDocument > xChartDoc = getChartDocFromWriter(0);
    CPPUNIT_ASSERT_MESSAGE("failed to load chart", xChartDoc.is());
    mxDiagram.set(xChartDoc->getDiagram());
    CPPUNIT_ASSERT(mxDiagram.is());
    // test X Axis title position
    uno::Reference< chart::XAxisXSupplier > xAxisXSupp(mxDiagram, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xAxisXSupp.is());

    xAxisTitle = xAxisXSupp->getXAxisTitle();
    CPPUNIT_ASSERT(xAxisTitle.is());

    awt::Point aPos = xAxisTitle->getPosition();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10640, aPos.X, 2);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7157, aPos.Y, 2);

    // test Y Axis title position
    uno::Reference< chart::XAxisYSupplier > xAxisYSupp(mxDiagram, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xAxisYSupp.is());

    xAxisTitle = xAxisYSupp->getYAxisTitle();
    CPPUNIT_ASSERT(xAxisTitle.is());

    aPos = xAxisTitle->getPosition();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(387, aPos.X, 2);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(6378, aPos.Y, 300);
}

void Chart2ImportTest::testCombinedChartAttachedAxisXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "testCombinedChartAxis.xlsx");
    Reference< chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    // First series
    Reference<chart2::XDataSeries> xSeries = getDataSeriesFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xSeries.is());

    Reference<beans::XPropertySet> xPropSet(xSeries, uno::UNO_QUERY_THROW);
    sal_Int32 nAxisIndex = -1;
    // First series (column chart) should be attached to secondary axis!
    uno::Any aAny = xPropSet->getPropertyValue("AttachedAxisIndex");
    CPPUNIT_ASSERT(aAny >>= nAxisIndex);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), nAxisIndex);

    // Second series
    xSeries = getDataSeriesFromDoc(xChartDoc, 0, 1);
    CPPUNIT_ASSERT(xSeries.is());

    xPropSet.set(xSeries, uno::UNO_QUERY_THROW);
    // Second series (line chart) should be attached to primary axis!
    aAny = xPropSet->getPropertyValue("AttachedAxisIndex");
    CPPUNIT_ASSERT(aAny >>= nAxisIndex);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), nAxisIndex);
}

void Chart2ImportTest::testTdf140489MultiSeriesChartAxisXLSX()
{
    load(u"/chart2/qa/extras/data/xlsx/", "tdf140489.xlsx");
    Reference< chart2::XChartDocument> xChartDoc = getChartDocFromSheet(0, mxComponent);
    // First series
    Reference<chart2::XDataSeries> xSeries = getDataSeriesFromDoc(xChartDoc, 0);
    CPPUNIT_ASSERT(xSeries.is());

    Reference<beans::XPropertySet> xPropSet(xSeries, uno::UNO_QUERY_THROW);
    sal_Int32 nAxisIndex = -1;
    uno::Any aAny = xPropSet->getPropertyValue("AttachedAxisIndex");
    CPPUNIT_ASSERT(aAny >>= nAxisIndex);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), nAxisIndex);

    // Second series
    xSeries = getDataSeriesFromDoc(xChartDoc, 0, 1);
    CPPUNIT_ASSERT(xSeries.is());

    xPropSet.set(xSeries, uno::UNO_QUERY_THROW);
    aAny = xPropSet->getPropertyValue("AttachedAxisIndex");
    CPPUNIT_ASSERT(aAny >>= nAxisIndex);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), nAxisIndex);

    // Third series
    xSeries = getDataSeriesFromDoc(xChartDoc, 0, 2);
    CPPUNIT_ASSERT(xSeries.is());

    xPropSet.set(xSeries, uno::UNO_QUERY_THROW);
    aAny = xPropSet->getPropertyValue("AttachedAxisIndex");
    CPPUNIT_ASSERT(aAny >>= nAxisIndex);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), nAxisIndex);
}

void Chart2ImportTest::testInternalDataProvider() {
    uno::Reference< chart2::XChartDocument > xChartDoc(getChartDocFromImpress(u"/chart2/qa/extras/data/odp/", "chart.odp"), uno::UNO_QUERY_THROW);
    const uno::Reference< chart2::data::XDataProvider >& rxDataProvider = xChartDoc->getDataProvider();

    // Parse 42 array
    Reference<chart2::data::XDataSequence> xDataSeq = rxDataProvider->createDataSequenceByValueArray("values-y", "{42;42;42;42}", "");
    Sequence<Any> xSequence = xDataSeq->getData();
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(42)), xSequence[0]);
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(42)), xSequence[1]);
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(42)), xSequence[2]);
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(42)), xSequence[3]);

    // Parse empty first and last
    xDataSeq = rxDataProvider->createDataSequenceByValueArray("values-y", "{\"\";42;42;\"\"}", "");
    xSequence = xDataSeq->getData();
    CPPUNIT_ASSERT( std::isnan( *static_cast<const double*>(xSequence[0].getValue())));
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(42)), xSequence[1]);
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(42)), xSequence[2]);
    CPPUNIT_ASSERT( std::isnan( *static_cast<const double*>(xSequence[3].getValue())));

    // Parse empty middle
    xDataSeq = rxDataProvider->createDataSequenceByValueArray("values-y", "{42;\"\";\"\";42}", "");
    xSequence = xDataSeq->getData();
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(42)), xSequence[0]);
    CPPUNIT_ASSERT( std::isnan( *static_cast<const double*>(xSequence[1].getValue())) );
    CPPUNIT_ASSERT( std::isnan( *static_cast<const double*>(xSequence[2].getValue())) );
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(42)), xSequence[3]);

    // Parse mixed types, numeric only role
    xDataSeq = rxDataProvider->createDataSequenceByValueArray("values-y", "{42;\"hello\";0;\"world\"}", "");
    xSequence = xDataSeq->getData();
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(42)), xSequence[0]);
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(0)),  xSequence[1]);
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(0)),  xSequence[2]);
    CPPUNIT_ASSERT_EQUAL(uno::Any(sal_Int32(0)),  xSequence[3]);

    // Parse mixed types, mixed role
    xDataSeq = rxDataProvider->createDataSequenceByValueArray("categories", "{42;\"hello\";0;\"world\"}", "");
    xSequence = xDataSeq->getData();
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Row 1 42")), xSequence[0]);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Row 2 hello")), xSequence[1]);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Row 3 0")), xSequence[2]);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Row 4 world")), xSequence[3]);
}

void Chart2ImportTest::testTdf90510()
{
    load(u"/chart2/qa/extras/data/xls/", "piechart_outside.xls");
    uno::Reference< chart::XChartDocument > xChart1Doc( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW );
    Reference<beans::XPropertySet> xPropSet( xChart1Doc->getDiagram()->getDataPointProperties( 0, 0 ), uno::UNO_SET_THROW );
    uno::Any aAny = xPropSet->getPropertyValue( "LabelPlacement" );
    CPPUNIT_ASSERT( aAny.hasValue() );
    sal_Int32 nLabelPlacement = 0;
    CPPUNIT_ASSERT( aAny >>= nLabelPlacement );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Data labels should be placed outside", chart::DataLabelPlacement::OUTSIDE, nLabelPlacement );
}

void Chart2ImportTest::testTdf109858()
{
    load(u"/chart2/qa/extras/data/xlsx/", "piechart_outside.xlsx");
    uno::Reference< chart::XChartDocument > xChart1Doc( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW );

    // test data point labels position
    Reference<beans::XPropertySet> xDataPointPropSet( xChart1Doc->getDiagram()->getDataPointProperties( 0, 0 ), uno::UNO_SET_THROW );
    uno::Any aAny = xDataPointPropSet->getPropertyValue( "LabelPlacement" );
    CPPUNIT_ASSERT( aAny.hasValue() );
    sal_Int32 nLabelPlacement = 0;
    CPPUNIT_ASSERT( aAny >>= nLabelPlacement );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Data point label should be placed bestfit", chart::DataLabelPlacement::CUSTOM, nLabelPlacement );

    // test data series label position
    Reference<beans::XPropertySet> xSeriesPropSet(xChart1Doc->getDiagram()->getDataRowProperties(0), uno::UNO_SET_THROW);
    aAny = xSeriesPropSet->getPropertyValue( "LabelPlacement" );
    CPPUNIT_ASSERT( aAny >>= nLabelPlacement );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Data series labels should be placed outside", chart::DataLabelPlacement::OUTSIDE, nLabelPlacement );
}

void Chart2ImportTest::testTdf130105()
{
    load(u"/chart2/qa/extras/data/xlsx/", "barchart_outend.xlsx");
    uno::Reference< chart2::XChartDocument > xChartDoc = getChartDocFromSheet(0, mxComponent);
    CPPUNIT_ASSERT(xChartDoc.is());
    uno::Reference<chart2::XDataSeries> xDataSeries(getDataSeriesFromDoc(xChartDoc, 0));
    CPPUNIT_ASSERT(xDataSeries.is());

    uno::Reference<beans::XPropertySet> xPropertySet(xDataSeries->getDataPointByIndex(0), uno::UNO_SET_THROW);
    uno::Any aAny = xPropertySet->getPropertyValue("LabelPlacement");
    CPPUNIT_ASSERT(aAny.hasValue());
    sal_Int32 nLabelPlacement = 0;
    CPPUNIT_ASSERT(aAny >>= nLabelPlacement);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Data label should be placed outend", chart::DataLabelPlacement::OUTSIDE, nLabelPlacement);
}

void Chart2ImportTest::testTdf111173()
{
    load(u"/chart2/qa/extras/data/xlsx/", "tdf111173.xlsx");
    uno::Reference< chart::XChartDocument > xChart1Doc( getChartCompFromSheet( 0, mxComponent ), UNO_QUERY_THROW );
}

void Chart2ImportTest::testTdf122226()
{
    load( u"/chart2/qa/extras/data/docx/", "testTdf122226.docx" );
    uno::Reference< chart2::XChartDocument > xChartDoc ( getChartDocFromWriter(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT( xChartDoc.is() );

    css::uno::Reference<chart2::XDiagram> xDiagram(xChartDoc->getFirstDiagram(), UNO_SET_THROW);
    Reference<chart2::XDataSeries> xDataSeries = getDataSeriesFromDoc(xChartDoc, 0);
    uno::Reference<beans::XPropertySet> xPropertySet(xDataSeries->getDataPointByIndex(0), uno::UNO_SET_THROW);
    CPPUNIT_ASSERT(xPropertySet.is());

    uno::Any aAny = xPropertySet->getPropertyValue( "LabelSeparator" );
    CPPUNIT_ASSERT( aAny.hasValue() );
    OUString nLabelSeparator;
    CPPUNIT_ASSERT( aAny >>= nLabelSeparator );
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Data labels should be separated into new lines", OUString("\n"), nLabelSeparator );
}

void Chart2ImportTest::testTdf115107()
{
    load(u"/chart2/qa/extras/data/pptx/", "tdf115107.pptx");

    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    uno::Reference<chart2::XDataSeries> xDataSeries(getDataSeriesFromDoc(xChartDoc, 0));
    CPPUNIT_ASSERT(xDataSeries.is());
    float nFontSize;
    sal_Int64 nFontColor;
    sal_Int32 nCharUnderline;
    uno::Reference<beans::XPropertySet> xPropertySet;
    uno::Sequence<uno::Reference<chart2::XDataPointCustomLabelField>> aFields;

    // 1
    xPropertySet.set(xDataSeries->getDataPointByIndex(0), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("CustomLabelFields") >>= aFields;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aFields.getLength());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_TEXT, aFields[0]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("90.0 = "), aFields[0]->getString());
    aFields[0]->getPropertyValue("CharHeight") >>= nFontSize;
    aFields[0]->getPropertyValue("CharColor") >>= nFontColor;
    CPPUNIT_ASSERT_EQUAL(static_cast<float>(18), nFontSize);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int64>(0xed7d31), nFontColor);

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_VALUE, aFields[1]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("90"), aFields[1]->getString());

    // 2
    xPropertySet.set(xDataSeries->getDataPointByIndex(1), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("CustomLabelFields") >>= aFields;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(8), aFields.getLength());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_TEXT, aFields[0]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), aFields[0]->getString());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_TEXT, aFields[1]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString(" : "), aFields[1]->getString());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_CATEGORYNAME, aFields[2]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("B"), aFields[2]->getString());
    aFields[2]->getPropertyValue("CharHeight") >>= nFontSize;
    aFields[2]->getPropertyValue("CharColor") >>= nFontColor;
    CPPUNIT_ASSERT_EQUAL(static_cast<float>(16), nFontSize);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int64>(0xed7d31), nFontColor);

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_NEWLINE, aFields[3]->getFieldType());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_TEXT, aFields[4]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("Multi"), aFields[4]->getString());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_TEXT, aFields[5]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("line"), aFields[5]->getString());
    aFields[5]->getPropertyValue("CharHeight") >>= nFontSize;
    aFields[5]->getPropertyValue("CharColor") >>= nFontColor;
    CPPUNIT_ASSERT_EQUAL(static_cast<float>(11.97), nFontSize);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int64>(0xbf9000), nFontColor);

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_NEWLINE, aFields[6]->getFieldType());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_TEXT, aFields[7]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("Abc"), aFields[7]->getString());
    aFields[7]->getPropertyValue("CharHeight") >>= nFontSize;
    aFields[7]->getPropertyValue("CharColor") >>= nFontColor;
    aFields[7]->getPropertyValue("CharUnderline") >>= nCharUnderline;
    CPPUNIT_ASSERT_EQUAL(static_cast<float>(12), nFontSize);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int64>(0xa9d18e), nFontColor);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), nCharUnderline);

    // 3
    xPropertySet.set(xDataSeries->getDataPointByIndex(2), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("CustomLabelFields") >>= aFields;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aFields.getLength());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_SERIESNAME, aFields[0]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("DATA"), aFields[0]->getString());

    // 4
    xPropertySet.set(xDataSeries->getDataPointByIndex(3), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("CustomLabelFields") >>= aFields;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aFields.getLength());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_CELLREF, aFields[0]->getFieldType());
    //CPPUNIT_ASSERT_EQUAL(OUString("70"), aFields[0]->getString()); TODO: Not implemented yet

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_TEXT, aFields[1]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString(" <CELLREF"), aFields[1]->getString());
}

void Chart2ImportTest::testTdf115107_2()
{
    load(u"/chart2/qa/extras/data/pptx/", "tdf115107-2.pptx");

    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    uno::Reference<chart2::XDataSeries> xDataSeries(getDataSeriesFromDoc(xChartDoc, 0));
    CPPUNIT_ASSERT(xDataSeries.is());
    float nFontSize;
    sal_Int64 nFontColor;
    uno::Reference<beans::XPropertySet> xPropertySet;
    uno::Sequence<uno::Reference<chart2::XDataPointCustomLabelField>> aFields;

    // First series
    xPropertySet.set(xDataSeries->getDataPointByIndex(0), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("CustomLabelFields") >>= aFields;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), aFields.getLength());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_VALUE, aFields[0]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("4.3"), aFields[0]->getString());
    aFields[0]->getPropertyValue("CharHeight") >>= nFontSize;
    aFields[0]->getPropertyValue("CharColor") >>= nFontColor;
    CPPUNIT_ASSERT_EQUAL(static_cast<float>(18), nFontSize);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int64>(0xc00000), nFontColor);

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_TEXT, aFields[1]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString(" "), aFields[1]->getString());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_SERIESNAME, aFields[2]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("Bars"), aFields[2]->getString());

    // Second series
    xDataSeries = getDataSeriesFromDoc(xChartDoc, 0, 1);
    CPPUNIT_ASSERT(xDataSeries.is());

    xPropertySet.set(xDataSeries->getDataPointByIndex(0), uno::UNO_SET_THROW);
    xPropertySet->getPropertyValue("CustomLabelFields") >>= aFields;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), aFields.getLength());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_VALUE, aFields[0]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("2"), aFields[0]->getString());
    aFields[0]->getPropertyValue("CharHeight") >>= nFontSize;
    aFields[0]->getPropertyValue("CharColor") >>= nFontColor;
    CPPUNIT_ASSERT_EQUAL(static_cast<float>(18), nFontSize);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int64>(0xffd966), nFontColor);

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_TEXT, aFields[1]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString(" "), aFields[1]->getString());

    CPPUNIT_ASSERT_EQUAL(chart2::DataPointCustomLabelFieldType::DataPointCustomLabelFieldType_SERIESNAME, aFields[2]->getFieldType());
    CPPUNIT_ASSERT_EQUAL(OUString("Line"), aFields[2]->getString());

}

void Chart2ImportTest::testTdf116163()
{
    load(u"/chart2/qa/extras/data/pptx/", "tdf116163.pptx");

    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XAxis> xHAxis = getAxisFromDoc(xChartDoc, 0, 0, 0);
    CPPUNIT_ASSERT(xHAxis.is());

    chart2::ScaleData aScaleData = xHAxis->getScaleData();
    CPPUNIT_ASSERT(aScaleData.Categories.is());

    Reference<chart2::data::XLabeledDataSequence> xLabeledDataSequence = aScaleData.Categories;
    CPPUNIT_ASSERT(xLabeledDataSequence.is());

    Reference<chart2::data::XDataSequence> xDataSequence = xLabeledDataSequence->getValues();
    CPPUNIT_ASSERT(xDataSequence.is());

    Reference<chart2::data::XTextualDataSequence> xTextualDataSequence(xDataSequence, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xTextualDataSequence.is());

    std::vector<OUString> aCategories;
    const Sequence<OUString> aTextData(xTextualDataSequence->getTextualData());
    ::std::copy(aTextData.begin(), aTextData.end(),
        ::std::back_inserter(aCategories));

    CPPUNIT_ASSERT_EQUAL(OUString("Aaaa"), aCategories[0]);
    CPPUNIT_ASSERT_EQUAL(OUString("Bbbbbbb"), aCategories[1]);
    CPPUNIT_ASSERT_EQUAL(OUString("Ccc"), aCategories[2]);
    CPPUNIT_ASSERT_EQUAL(OUString("Ddddddddddddd"), aCategories[3]);

    // Check visible text

    uno::Reference<drawing::XDrawPageSupplier> xDrawPageSupplier(xChartDoc, uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage = xDrawPageSupplier->getDrawPage();
    uno::Reference<drawing::XShapes> xShapes(xDrawPage->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xShapes.is());

    uno::Reference<drawing::XShape> xXAxis = getShapeByName(xShapes, "CID/D=0:CS=0:Axis=0,0",
        // Axis occurs twice in chart xshape representation so need to get the one related to labels
        [](const uno::Reference<drawing::XShape>& rXShape) -> bool
    {
        uno::Reference<drawing::XShapes> xAxisShapes(rXShape, uno::UNO_QUERY);
        CPPUNIT_ASSERT(xAxisShapes.is());
        uno::Reference<drawing::XShape> xChildShape(xAxisShapes->getByIndex(0), uno::UNO_QUERY);
        uno::Reference< drawing::XShapeDescriptor > xShapeDescriptor(xChildShape, uno::UNO_QUERY_THROW);
        return (xShapeDescriptor->getShapeType() == "com.sun.star.drawing.TextShape");
    });
    CPPUNIT_ASSERT(xXAxis.is());

    uno::Reference<container::XIndexAccess> xIndexAccess(xXAxis, UNO_QUERY_THROW);

    // Check text
    uno::Reference<text::XTextRange> xLabel0(xIndexAccess->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Aaaa"), xLabel0->getString());
    uno::Reference<text::XTextRange> xLabel1(xIndexAccess->getByIndex(1), uno::UNO_QUERY);
    // If there is space for 3 chars only then don't show "..."
    CPPUNIT_ASSERT_EQUAL(OUString("Bbb"), xLabel1->getString());
    uno::Reference<text::XTextRange> xLabel2(xIndexAccess->getByIndex(2), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Ccc"), xLabel2->getString());
    uno::Reference<text::XTextRange> xLabel3(xIndexAccess->getByIndex(3), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Dddd..."), xLabel3->getString());
}

void Chart2ImportTest::testTdf48041()
{
    load(u"/chart2/qa/extras/data/pptx/", "tdf48041.pptx");

    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XAxis> xYAxis = getAxisFromDoc(xChartDoc, 0, 1, 0);
    CPPUNIT_ASSERT(xYAxis.is());

    chart2::ScaleData aScaleData = xYAxis->getScaleData();
    CPPUNIT_ASSERT(aScaleData.Scaling.is());

    // Check visible text
    uno::Reference<drawing::XDrawPageSupplier> xDrawPageSupplier(xChartDoc, uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage = xDrawPageSupplier->getDrawPage();
    uno::Reference<drawing::XShapes> xShapes(xDrawPage->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xShapes.is());

    uno::Reference<drawing::XShape> xYAxisShape = getShapeByName(xShapes, "CID/D=0:CS=0:Axis=1,0", // Y Axis
        // Axis occurs twice in chart xshape representation so need to get the one related to labels
        [](const uno::Reference<drawing::XShape>& rXShape) -> bool
    {
        uno::Reference<drawing::XShapes> xAxisShapes(rXShape, uno::UNO_QUERY);
        CPPUNIT_ASSERT(xAxisShapes.is());
        uno::Reference<drawing::XShape> xChildShape(xAxisShapes->getByIndex(0), uno::UNO_QUERY);
        uno::Reference< drawing::XShapeDescriptor > xShapeDescriptor(xChildShape, uno::UNO_QUERY_THROW);
        return (xShapeDescriptor->getShapeType() == "com.sun.star.drawing.TextShape");
    });
    CPPUNIT_ASSERT(xYAxisShape.is());

    // Check label count
    uno::Reference<container::XIndexAccess> xIndexAccess(xYAxisShape, UNO_QUERY_THROW);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(6), xIndexAccess->getCount());

    // Check text
    uno::Reference<text::XTextRange> xLabel0(xIndexAccess->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("0"), xLabel0->getString());
    uno::Reference<text::XTextRange> xLabel1(xIndexAccess->getByIndex(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("1"), xLabel1->getString());
    uno::Reference<text::XTextRange> xLabel2(xIndexAccess->getByIndex(2), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("2"), xLabel2->getString());
    uno::Reference<text::XTextRange> xLabel3(xIndexAccess->getByIndex(3), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("3"), xLabel3->getString());
    uno::Reference<text::XTextRange> xLabel4(xIndexAccess->getByIndex(4), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("4"), xLabel4->getString());
    uno::Reference<text::XTextRange> xLabel5(xIndexAccess->getByIndex(5), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("5"), xLabel5->getString());
}

void Chart2ImportTest::testTdf121205()
{
    load(u"/chart2/qa/extras/data/pptx/", "tdf121205.pptx");
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);

    uno::Reference<chart2::XTitled> xTitled(xChartDoc, uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT_MESSAGE("chart doc does not have title", xTitled.is());
    OUString aTitle = getTitleString(xTitled);

    // We expect title split in 3 lines
    CPPUNIT_ASSERT_EQUAL(OUString("Firstline\nSecondline\nThirdline"), aTitle);
}

void Chart2ImportTest::testFixedSizeBarChartVeryLongLabel()
{
    // Bar chart area size is fixed (not automatic) so we can't resize
    // the chart area to let the label break into multiple lines. In this
    // case the best course of action is to just crop the label text. This
    // test checks that the rendered text is actually cropped.

    load(u"/chart2/qa/extras/data/odp/", "BarChartVeryLongLabel.odp");

    // Select shape 0 which has fixed size chart
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XAxis> xHAxis = getAxisFromDoc(xChartDoc, 0, 0, 0);
    CPPUNIT_ASSERT(xHAxis.is());

    chart2::ScaleData aScaleData = xHAxis->getScaleData();
    CPPUNIT_ASSERT(aScaleData.Categories.is());

    Reference<chart2::data::XLabeledDataSequence> xLabeledDataSequence = aScaleData.Categories;
    CPPUNIT_ASSERT(xLabeledDataSequence.is());

    Reference<chart2::data::XDataSequence> xDataSequence = xLabeledDataSequence->getValues();
    CPPUNIT_ASSERT(xDataSequence.is());

    Reference<chart2::data::XTextualDataSequence> xTextualDataSequence(xDataSequence, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xTextualDataSequence.is());

    std::vector<OUString> aCategories;
    const Sequence<OUString> aTextData(xTextualDataSequence->getTextualData());
    ::std::copy(aTextData.begin(), aTextData.end(),
        ::std::back_inserter(aCategories));

    // Check that we have a very very long label text
    CPPUNIT_ASSERT_EQUAL(OUString("Very very very very very very very very very very very loooooooooooong label"), aCategories[0]);

    // Check visible text
    uno::Reference<drawing::XDrawPageSupplier> xDrawPageSupplier(xChartDoc, uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage = xDrawPageSupplier->getDrawPage();
    uno::Reference<drawing::XShapes> xShapes(xDrawPage->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xShapes.is());

    uno::Reference<drawing::XShape> xXAxis = getShapeByName(xShapes, "CID/D=0:CS=0:Axis=0,0",
        // Axis occurs twice in chart xshape representation so need to get the one related to labels
        [](const uno::Reference<drawing::XShape>& rXShape) -> bool
    {
        uno::Reference<drawing::XShapes> xAxisShapes(rXShape, uno::UNO_QUERY);
        CPPUNIT_ASSERT(xAxisShapes.is());
        uno::Reference<drawing::XShape> xChildShape(xAxisShapes->getByIndex(0), uno::UNO_QUERY);
        uno::Reference< drawing::XShapeDescriptor > xShapeDescriptor(xChildShape, uno::UNO_QUERY_THROW);
        return (xShapeDescriptor->getShapeType() == "com.sun.star.drawing.TextShape");
    });
    CPPUNIT_ASSERT(xXAxis.is());

    uno::Reference<container::XIndexAccess> xIndexAccess(xXAxis, UNO_QUERY_THROW);

    // Check text is actually cropped
    uno::Reference<text::XTextRange> xLabel(xIndexAccess->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Very very very very very very..."), xLabel->getString());

    uno::Reference<drawing::XShape> xChartWall = getShapeByName(xShapes, "CID/DiagramWall=");
    CPPUNIT_ASSERT(xChartWall.is());

    // The text shape width should be smaller than the chart wall
    CPPUNIT_ASSERT_EQUAL(sal_Int32(5085), xChartWall->getSize().Width);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(7113), xChartWall->getSize().Height);

    CPPUNIT_ASSERT_EQUAL(sal_Int32(4870), xXAxis->getSize().Width);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(398), xXAxis->getSize().Height);
}

void Chart2ImportTest::testAutomaticSizeBarChartVeryLongLabel()
{
    // Bar chart area size is automatic so we expect the label to be broken
    // into multiple lines.

    load(u"/chart2/qa/extras/data/odp/", "BarChartVeryLongLabel.odp");

    // Select shape 1, which has an automatic sized chart
    Reference<chart2::XChartDocument> xChartDoc(getChartDocFromDrawImpress(0, 1), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference<chart2::XAxis> xHAxis = getAxisFromDoc(xChartDoc, 0, 0, 0);
    CPPUNIT_ASSERT(xHAxis.is());

    chart2::ScaleData aScaleData = xHAxis->getScaleData();
    CPPUNIT_ASSERT(aScaleData.Categories.is());

    Reference<chart2::data::XLabeledDataSequence> xLabeledDataSequence = aScaleData.Categories;
    CPPUNIT_ASSERT(xLabeledDataSequence.is());

    Reference<chart2::data::XDataSequence> xDataSequence = xLabeledDataSequence->getValues();
    CPPUNIT_ASSERT(xDataSequence.is());

    Reference<chart2::data::XTextualDataSequence> xTextualDataSequence(xDataSequence, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xTextualDataSequence.is());

    std::vector<OUString> aCategories;
    const Sequence<OUString> aTextData(xTextualDataSequence->getTextualData());
    ::std::copy(aTextData.begin(), aTextData.end(),
        ::std::back_inserter(aCategories));

    // Check that we have a very very long label text
    CPPUNIT_ASSERT_EQUAL(OUString("Very very very very very very very very very very very loooooooooooong label"), aCategories[0]);

    // Check visible text
    uno::Reference<drawing::XDrawPageSupplier> xDrawPageSupplier(xChartDoc, uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPage> xDrawPage = xDrawPageSupplier->getDrawPage();
    uno::Reference<drawing::XShapes> xShapes(xDrawPage->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xShapes.is());

    uno::Reference<drawing::XShape> xXAxis = getShapeByName(xShapes, "CID/D=0:CS=0:Axis=0,0",
        // Axis occurs twice in chart xshape representation so need to get the one related to labels
        [](const uno::Reference<drawing::XShape>& rXShape) -> bool
    {
        uno::Reference<drawing::XShapes> xAxisShapes(rXShape, uno::UNO_QUERY);
        CPPUNIT_ASSERT(xAxisShapes.is());
        uno::Reference<drawing::XShape> xChildShape(xAxisShapes->getByIndex(0), uno::UNO_QUERY);
        uno::Reference< drawing::XShapeDescriptor > xShapeDescriptor(xChildShape, uno::UNO_QUERY_THROW);
        return (xShapeDescriptor->getShapeType() == "com.sun.star.drawing.TextShape");
    });
    CPPUNIT_ASSERT(xXAxis.is());

    uno::Reference<container::XIndexAccess> xIndexAccess(xXAxis, UNO_QUERY_THROW);

    // Check text is unmodified
    uno::Reference<text::XTextRange> xLabel(xIndexAccess->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Very very very very very very very very very very very loooooooooooong label"), xLabel->getString());

    uno::Reference<drawing::XShape> xChartWall = getShapeByName(xShapes, "CID/DiagramWall=");
    CPPUNIT_ASSERT(xChartWall.is());

    // The text shape width should be smaller than the chart wall
    CPPUNIT_ASSERT_EQUAL(sal_Int32(5761), xChartWall->getSize().Width);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(7200), xChartWall->getSize().Height);

    CPPUNIT_ASSERT_EQUAL(sal_Int32(5320), xXAxis->getSize().Width);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1192), xXAxis->getSize().Height);
}

CPPUNIT_TEST_SUITE_REGISTRATION(Chart2ImportTest);

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
