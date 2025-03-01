/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <swmodeltestbase.hxx>

#include <com/sun/star/style/BreakType.hpp>
#include <com/sun/star/style/ParagraphAdjust.hpp>
#include <com/sun/star/text/WritingMode2.hpp>
#include <com/sun/star/text/XDependentTextField.hpp>
#include <com/sun/star/text/RubyAdjust.hpp>
#include <com/sun/star/text/RubyPosition.hpp>
#include <com/sun/star/text/XDocumentIndex.hpp>
#include <com/sun/star/drawing/FillStyle.hpp>
#include <com/sun/star/text/XTextTable.hpp>
#include <com/sun/star/style/TabStop.hpp>
#include <com/sun/star/packages/zip/ZipFileAccess.hpp>

#include <comphelper/processfactory.hxx>
#include <comphelper/sequenceashashmap.hxx>

#include <unotxdoc.hxx>
#include <docsh.hxx>

class Test : public SwModelTestBase
{
public:
    Test() : SwModelTestBase("/sw/qa/extras/ooxmlexport/data/", "Office Open XML Text") {}

protected:
    /**
     * Denylist handling
     */
    bool mustTestImportOf(const char* filename) const override {
        // If the testcase is stored in some other format, it's pointless to test.
        return OString(filename).endsWith(".docx");
    }
};

DECLARE_OOXMLEXPORT_TEST(testTdf57589_hashColor, "tdf57589_hashColor.docx")
{
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_SOLID, getProperty<drawing::FillStyle>(getParagraph(1), "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(COL_LIGHTMAGENTA, Color(ColorTransparency, getProperty<sal_uInt32>(getParagraph(1), "ParaBackColor")));
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_NONE, getProperty<drawing::FillStyle>(getParagraph(2), "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(COL_AUTO, Color(ColorTransparency, getProperty<sal_uInt32>(getParagraph(2), "ParaBackColor")));
}

DECLARE_OOXMLEXPORT_TEST(testTdf90906_colAuto, "tdf90906_colAuto.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTextTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTextTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<text::XTextRange> xCell(xTable->getCellByName("A1"), uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xParaEnumAccess(xCell->getText(), uno::UNO_QUERY);
    uno::Reference<container::XEnumeration> xParaEnum = xParaEnumAccess->createEnumeration();
    uno::Reference<text::XTextRange> xPara(xParaEnum->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(COL_AUTO, Color(ColorTransparency, getProperty<sal_uInt32>(getRun(xPara, 1, "Nazwa"), "CharBackColor")));
}

DECLARE_OOXMLEXPORT_TEST(testTdf90906_colAutoB, "tdf90906_colAutoB.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTextTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTextTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY);

    uno::Reference<table::XCell> xCell = xTable->getCellByName("A1");
    CPPUNIT_ASSERT_EQUAL(COL_LIGHTGREEN, Color(ColorTransparency, getProperty<sal_uInt32>(xCell, "BackColor")));
    xCell.set(xTable->getCellByName("A2"));
    CPPUNIT_ASSERT_EQUAL(COL_AUTO, Color(ColorTransparency, getProperty<sal_uInt32>(xCell, "BackColor")));
    xCell.set(xTable->getCellByName("B1"));
    CPPUNIT_ASSERT_EQUAL(COL_AUTO, Color(ColorTransparency, getProperty<sal_uInt32>(xCell, "BackColor")));
    xCell.set(xTable->getCellByName("B2"));
    CPPUNIT_ASSERT_EQUAL(COL_LIGHTBLUE, Color(ColorTransparency, getProperty<sal_uInt32>(xCell, "BackColor")));

    uno::Reference<text::XTextRange> xText(getParagraph(2, "Paragraphs too"));
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_NONE, getProperty<drawing::FillStyle>(xText, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(COL_AUTO, Color(ColorTransparency, getProperty<sal_uInt32>(xText, "ParaBackColor")));
}

DECLARE_OOXMLEXPORT_TEST(testTdf92524_autoColor, "tdf92524_autoColor.doc")
{
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_NONE, getProperty<drawing::FillStyle>(getParagraph(1), "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(COL_AUTO, Color(ColorTransparency, getProperty<sal_uInt32>(getParagraph(1), "ParaBackColor")));
}

DECLARE_OOXMLEXPORT_TEST(testTdf116436_rowFill, "tdf116436_rowFill.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<text::XTextTablesSupplier> xTextTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTextTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<table::XCell> xCell = xTable->getCellByName("A1");
    CPPUNIT_ASSERT_EQUAL(Color(0xF8DF7C), Color(ColorTransparency, getProperty<sal_Int32>(xCell, "BackColor")));
}

DECLARE_OOXMLEXPORT_TEST(testTdf121665_back2backColumnBreaks, "tdf121665_back2backColumnBreaks.docx")
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Column break type",
        style::BreakType_COLUMN_BEFORE, getProperty<style::BreakType>(getParagraph(2), "BreakType"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf126795_TabsRelativeToIndent0, "tdf126795_TabsRelativeToIndent0.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Sequence< style::TabStop > stops = getProperty< uno::Sequence<style::TabStop> >(getParagraph( 2 ), "ParaTabStops");
    CPPUNIT_ASSERT_EQUAL( sal_Int32(1), stops.getLength());
    CPPUNIT_ASSERT_EQUAL( css::style::TabAlign_LEFT, stops[ 0 ].Alignment );
    CPPUNIT_ASSERT_EQUAL( sal_Int32(499), stops[ 0 ].Position );
}

DECLARE_OOXMLEXPORT_TEST(testTdf126795_TabsRelativeToIndent1, "tdf126795_TabsRelativeToIndent1.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Sequence< style::TabStop > stops = getProperty< uno::Sequence<style::TabStop> >(getParagraph( 2 ), "ParaTabStops");
    CPPUNIT_ASSERT_EQUAL( sal_Int32(1), stops.getLength());
    CPPUNIT_ASSERT_EQUAL( css::style::TabAlign_LEFT, stops[ 0 ].Alignment );
    CPPUNIT_ASSERT_EQUAL( sal_Int32(499), stops[ 0 ].Position );
}

DECLARE_OOXMLEXPORT_TEST(testTdf46938_clearTabStop, "tdf46938_clearTabStop.docx")
{
    // Number of tabstops should be zero, overriding the one in the style
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty< uno::Sequence<style::TabStop> >(getParagraph(1), "ParaTabStops").getLength());
}

DECLARE_OOXMLEXPORT_TEST(testTdf63561_clearTabs, "tdf63561_clearTabs.docx")
{
    CPPUNIT_ASSERT_EQUAL(sal_Int32(5), getProperty< uno::Sequence<style::TabStop> >(getParagraph(1), "ParaTabStops").getLength());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(7), getProperty< uno::Sequence<style::TabStop> >(getParagraph(3), "ParaTabStops").getLength());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(4), getProperty< uno::Sequence<style::TabStop> >(getParagraph(4), "ParaTabStops").getLength());
}

DECLARE_OOXMLEXPORT_TEST(testTdf63561_clearTabs2, "tdf63561_clearTabs2.docx")
{
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), getProperty< uno::Sequence<style::TabStop> >(getParagraph(1), "ParaTabStops").getLength());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(3), getProperty< uno::Sequence<style::TabStop> >(getParagraph(3), "ParaTabStops").getLength());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(4), getProperty< uno::Sequence<style::TabStop> >(getParagraph(4), "ParaTabStops").getLength());
}

DECLARE_OOXMLEXPORT_TEST(testTdf124384, "tdf124384.docx")
{
    // There should be no crash during loading of the document
    // so, let's check just how much pages we have
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf121456_tabsOffset, "tdf121456_tabsOffset.odt")
{
    for (int i=2; i<8; i++)
    {
        uno::Sequence< style::TabStop > stops = getProperty< uno::Sequence<style::TabStop> >(getParagraph( i ), "ParaTabStops");
        CPPUNIT_ASSERT_EQUAL( sal_Int32(1), stops.getLength());
        CPPUNIT_ASSERT_EQUAL( css::style::TabAlign_RIGHT, stops[ 0 ].Alignment );
        CPPUNIT_ASSERT_EQUAL( sal_Int32(17000), stops[ 0 ].Position );
    }
}

// tdf#121561: make sure w:sdt/w:sdtContent around TOC is written during ODT->DOCX conversion
CPPUNIT_TEST_FIXTURE(Test, testTdf121561_tocTitle)
{
    loadAndSave("tdf121456_tabsOffset.odt");
    xmlDocUniquePtr pXmlDoc = parseExport();
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:sdt/w:sdtContent/w:p/w:r/w:t", "Inhaltsverzeichnis");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:sdt/w:sdtContent/w:p/w:r/w:instrText", " TOC \\f \\o \"1-9\" \\h");
    assertXPath(pXmlDoc, "/w:document/w:body/w:sdt/w:sdtPr/w:docPartObj/w:docPartGallery", "val", "Table of Contents");
    assertXPath(pXmlDoc, "/w:document/w:body/w:sdt/w:sdtPr/w:docPartObj/w:docPartUnique", 1);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf129525)
{
    loadAndSave("tdf129525.rtf");
    xmlDocUniquePtr pXmlDoc = parseExport();
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:sdt/w:sdtContent/w:p[1]/w:r[4]/w:t", "Overview");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:sdt/w:sdtContent/w:p[1]/w:r[5]/w:t", "3");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:sdt/w:sdtContent/w:p[2]/w:r[1]/w:t", "More detailed description");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:sdt/w:sdtContent/w:p[2]/w:r[2]/w:t", "4");
}

// Related issue tdf#121561: w:sdt/w:sdtContent around TOC
DECLARE_OOXMLEXPORT_TEST(testTdf124106, "tdf121456.docx")
{
    uno::Reference<text::XTextDocument> textDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XText> text = textDocument->getText();
    // -1 if the 'Y' character does not occur
    CPPUNIT_ASSERT_EQUAL(sal_Int32(-1), text->getString().indexOf('Y'));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(-1), text->getString().indexOf('y'));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf121561_tocTitleDocx)
{
    loadAndSave("tdf121456_tabsOffset.odt");
    CPPUNIT_ASSERT_EQUAL(7, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport();

    // get TOC node
    uno::Reference<text::XDocumentIndexesSupplier> xIndexSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xIndexes = xIndexSupplier->getDocumentIndexes( );
    uno::Reference<text::XDocumentIndex> xTOCIndex(xIndexes->getByIndex(0), uno::UNO_QUERY);

    // ensure TOC title was set in TOC properties
    CPPUNIT_ASSERT_EQUAL(OUString("Inhaltsverzeichnis"), getProperty<OUString>(xTOCIndex, "Title"));

    // ensure TOC end-field mark is placed inside TOC section
    assertXPath(pXmlDoc, "/w:document/w:body/w:sdt/w:sdtContent/w:p[16]/w:r/w:fldChar", "fldCharType", "end");
}

DECLARE_OOXMLEXPORT_TEST(testTdf106174_rtlParaAlign, "tdf106174_rtlParaAlign.docx")
{
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_CENTER), getProperty<sal_Int16>(getParagraph(1), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_CENTER), getProperty<sal_Int16>(getParagraph(2), "ParaAdjust"));
    uno::Reference<beans::XPropertySet> xPropertySet(getStyles("ParagraphStyles")->getByName("Another paragraph aligned to right"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_RIGHT), getProperty<sal_Int16>(xPropertySet, "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_RIGHT), getProperty<sal_Int16>(getParagraph(3), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_RIGHT), getProperty<sal_Int16>(getParagraph(4), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_RIGHT), getProperty<sal_Int16>(getParagraph(5), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_LEFT),  getProperty<sal_Int16>(getParagraph(6), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_RIGHT), getProperty<sal_Int16>(getParagraph(7), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_RIGHT), getProperty<sal_Int16>(getParagraph(8), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_LEFT),  getProperty<sal_Int16>(getParagraph(9), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_LEFT),  getProperty<sal_Int16>(getParagraph(10), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_RIGHT), getProperty<sal_Int16>(getParagraph(11), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_LEFT),  getProperty<sal_Int16>(getParagraph(12), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_LEFT),  getProperty<sal_Int16>(getParagraph(13), "ParaAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(style::ParagraphAdjust_RIGHT), getProperty<sal_Int16>(getParagraph(14), "ParaAdjust"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf82065_Ind_start_strict, "tdf82065_Ind_start_strict.docx")
{
    uno::Reference<beans::XPropertySet> xPropertySet(getStyles("NumberingStyles")->getByName("WWNum1"), uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xLevels(xPropertySet->getPropertyValue("NumberingRules"), uno::UNO_QUERY);
    uno::Sequence<beans::PropertyValue> aProps;
    xLevels->getByIndex(0) >>= aProps; // 1st level
    bool bFoundIndentAt = false;
    for (beans::PropertyValue const & rProp : std::as_const(aProps))
    {
        if (rProp.Name == "IndentAt")
        {
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("IndentAt", double(6001), rProp.Value.get<double>(), 10 );
            bFoundIndentAt = true;
        }
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("IndentAt defined", true, bFoundIndentAt);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf76683_negativeTwipsMeasure)
{
    loadAndSave("tdf76683_negativeTwipsMeasure.docx");
    xmlDocUniquePtr pXmlDoc = parseExport("word/document.xml");
    assertXPath(pXmlDoc, "/w:document/w:body/w:sectPr/w:cols/w:col", 2);
    sal_uInt32 nColumn1 = getXPath(pXmlDoc, "/w:document/w:body/w:sectPr/w:cols/w:col[1]", "w").toUInt32();
    sal_uInt32 nColumn2 = getXPath(pXmlDoc, "/w:document/w:body/w:sectPr/w:cols/w:col[2]", "w").toUInt32();
    CPPUNIT_ASSERT( nColumn1 > nColumn2 );
}

DECLARE_OOXMLEXPORT_TEST(testTdf112694, "tdf112694.docx")
{
    uno::Any aPageStyle = getStyles("PageStyles")->getByName("Standard");
    // Header was on when header for file was for explicit first pages only
    // (marked via <w:titlePg>).
    CPPUNIT_ASSERT(!getProperty<bool>(aPageStyle, "HeaderIsOn"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf113849_evenAndOddHeaders, "tdf113849_evenAndOddHeaders.odt")
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Header2 text", OUString("L. J. Kendall"), parseDump("/root/page[2]/header/txt"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Footer2 text", OUString("*"), parseDump("/root/page[2]/footer/txt"));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Header3 text", OUString("Shadow Hunt"), parseDump("/root/page[3]/header/txt"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Footer3 text", OUString("*"), parseDump("/root/page[3]/footer/txt"));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Header4 text", OUString("L. J. Kendall"), parseDump("/root/page[4]/header/txt"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Footer4 text", OUString("*"), parseDump("/root/page[4]/footer/txt"));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Footer5 text", OUString(""), parseDump("/root/page[5]/footer/txt"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Footer6 text", OUString(""), parseDump("/root/page[6]/footer/txt"));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Number of pages", 6, getPages() );
}

DECLARE_OOXMLEXPORT_TEST(testTdf118361_RTLfootnoteSeparator, "tdf118361_RTLfootnoteSeparator.docx")
{
    uno::Any aPageStyle = getStyles("PageStyles")->getByName("Standard");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Footnote separator RTL", sal_Int16(2), getProperty<sal_Int16>(aPageStyle, "FootnoteLineAdjust"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf115861, "tdf115861.docx")
{
    // Second item in the paragraph enumeration was a table, 2nd paragraph was
    // lost.
    CPPUNIT_ASSERT_EQUAL(OUString("(k)"), getParagraph(2)->getString());
}

DECLARE_OOXMLEXPORT_TEST(testTdf67207_MERGEFIELD, "mailmerge.docx")
{
    uno::Reference<beans::XPropertySet> xTextField = getProperty< uno::Reference<beans::XPropertySet> >(getRun(getParagraph(1), 2), "TextField");
    CPPUNIT_ASSERT(xTextField.is());
    uno::Reference<lang::XServiceInfo> xServiceInfo(xTextField, uno::UNO_QUERY_THROW);
    uno::Reference<text::XDependentTextField> xDependent(xTextField, uno::UNO_QUERY_THROW);

    CPPUNIT_ASSERT(xServiceInfo->supportsService("com.sun.star.text.TextField.Database"));
    OUString sValue;
    xTextField->getPropertyValue("Content") >>= sValue;
    CPPUNIT_ASSERT_EQUAL(OUString(u"«Name»"), sValue);

    uno::Reference<beans::XPropertySet> xFiledMaster = xDependent->getTextFieldMaster();
    uno::Reference<lang::XServiceInfo> xFiledMasterServiceInfo(xFiledMaster, uno::UNO_QUERY_THROW);

    CPPUNIT_ASSERT(xFiledMasterServiceInfo->supportsService("com.sun.star.text.fieldmaster.Database"));

    // Defined properties: DataBaseName, Name, DataTableName, DataColumnName, DependentTextFields, DataCommandType, InstanceName, DataBaseURL
    CPPUNIT_ASSERT(xFiledMaster->getPropertyValue("Name") >>= sValue);
    CPPUNIT_ASSERT_EQUAL(OUString("Name"), sValue);
    CPPUNIT_ASSERT(xFiledMaster->getPropertyValue("DataColumnName") >>= sValue);
    CPPUNIT_ASSERT_EQUAL(OUString("Name"), sValue);
    CPPUNIT_ASSERT(xFiledMaster->getPropertyValue("InstanceName") >>= sValue);
    CPPUNIT_ASSERT_EQUAL(OUString("com.sun.star.text.fieldmaster.DataBase.Name"), sValue);
}

DECLARE_OOXMLEXPORT_TEST(testTdf115719, "tdf115719.docx")
{
    // This was a single page, instead of pushing the textboxes to the second
    // page.
    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf115719b, "tdf115719b.docx")
{
    // This is similar to testTdf115719, but here the left textbox is not aligned "from left, by
    // 0cm" but simply aligned to left, which is a different codepath.

    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 2
    // - Actual  : 1
    // i.e. the textboxes did not appear on the 2nd page, but everything was on a single page.
    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf123243, "tdf123243.docx")
{
    // Without the accompanying fix in place, this test would have failed with 'Expected: 1; Actual:
    // 2'; i.e. unexpected paragraph margin created 2 pages.
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf116410, "tdf116410.docx")
{
    // Opposite of the above, was 2 pages, should be 1 page.
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testDefaultStyle, "defaultStyle.docx")
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Default Style", OUString("Title"), getProperty<OUString>(getParagraph(1), "ParaStyleName") );
    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf147115_defaultStyle, "tdf147115_defaultStyle.docx")
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Default Style", OUString("Standard"), getProperty<OUString>(getParagraph(1), "ParaStyleName") );
}

DECLARE_OOXMLEXPORT_TEST(testTdf117988, "tdf117988.docx")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf94801, "tdf94801.docx")
{
    // This was a 2-page document with unwanted line breaking in table cells, because
    // the table was narrower, than defined (< 1/100 mm loss during twip to 1/100 mm conversion)
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

CPPUNIT_TEST_FIXTURE(Test, testParagraphSplitOnSectionBorder)
{
    loadAndSave("parasplit-on-section-border.odt");
    CPPUNIT_ASSERT_EQUAL(2, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("word/document.xml");

    // Test document has only two paragraphs. After splitting, it should contain
    // three of them.
    assertXPath(pXmlDoc, "//w:sectPr", 2);
    assertXPath(pXmlDoc, "//w:p", 3);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf44832_testSectionWithDifferentHeader)
{
    loadAndSave("tdf44832_section_new_header.odt");
    CPPUNIT_ASSERT_EQUAL(2, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("word/document.xml");
    assertXPath(pXmlDoc, "/w:document/w:body/w:sectPr/w:headerReference", 1);
}

DECLARE_OOXMLEXPORT_TEST(testSignatureLineShape, "signature-line-all-props-set.docx")
{
    uno::Reference<drawing::XShape> xSignatureLineShape = getShape(1);
    uno::Reference<beans::XPropertySet> xPropSet(xSignatureLineShape, uno::UNO_QUERY);

    bool bIsSignatureLine;
    xPropSet->getPropertyValue("IsSignatureLine") >>= bIsSignatureLine;
    CPPUNIT_ASSERT_EQUAL(true, bIsSignatureLine);

    bool bShowSignDate;
    xPropSet->getPropertyValue("SignatureLineShowSignDate") >>= bShowSignDate;
    CPPUNIT_ASSERT_EQUAL(true, bShowSignDate);

    bool bCanAddComment;
    xPropSet->getPropertyValue("SignatureLineCanAddComment") >>= bCanAddComment;
    CPPUNIT_ASSERT_EQUAL(true, bCanAddComment);

    OUString aSignatureLineId;
    xPropSet->getPropertyValue("SignatureLineId") >>= aSignatureLineId;
    CPPUNIT_ASSERT_EQUAL(OUString("{0EBE47D5-A1BD-4C9E-A52E-6256E5C345E9}"), aSignatureLineId);

    OUString aSuggestedSignerName;
    xPropSet->getPropertyValue("SignatureLineSuggestedSignerName") >>= aSuggestedSignerName;
    CPPUNIT_ASSERT_EQUAL(OUString("John Doe"), aSuggestedSignerName);

    OUString aSuggestedSignerTitle;
    xPropSet->getPropertyValue("SignatureLineSuggestedSignerTitle") >>= aSuggestedSignerTitle;
    CPPUNIT_ASSERT_EQUAL(OUString("Farmer"), aSuggestedSignerTitle);

    OUString aSuggestedSignerEmail;
    xPropSet->getPropertyValue("SignatureLineSuggestedSignerEmail") >>= aSuggestedSignerEmail;
    CPPUNIT_ASSERT_EQUAL(OUString("john@thefarmers.com"), aSuggestedSignerEmail);

    OUString aSigningInstructions;
    xPropSet->getPropertyValue("SignatureLineSigningInstructions") >>= aSigningInstructions;
    CPPUNIT_ASSERT_EQUAL(OUString("Check the machines!"), aSigningInstructions);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf117805)
{
    loadAndSave("tdf117805.odt");
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<packages::zip::XZipFileAccess2> xNameAccess
        = packages::zip::ZipFileAccess::createWithURL(comphelper::getComponentContext(m_xSFactory),
                                                      maTempFile.GetURL());
    // This failed, the header was lost. It's still referenced at an incorrect
    // location in document.xml, though.
    CPPUNIT_ASSERT(xNameAccess->hasByName("word/header1.xml"));

    uno::Reference<text::XText> textbox(getShape(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(8, getParagraphs(textbox));
}

DECLARE_OOXMLEXPORT_TEST(testTdf113183, "tdf113183.docx")
{
    // The horizontal positioning of the star shape affected the positioning of
    // the triangle one, so the triangle was outside the page frame.
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    sal_Int32 nPageLeft = getXPath(pXmlDoc, "/root/page[1]/infos/bounds", "left").toInt32();
    sal_Int32 nPageWidth = getXPath(pXmlDoc, "/root/page[1]/infos/bounds", "width").toInt32();
    sal_Int32 nShapeLeft
        = getXPath(pXmlDoc, "/root/page/body/txt/anchored/SwAnchoredDrawObject[2]/bounds", "left")
              .toInt32();
    sal_Int32 nShapeWidth
        = getXPath(pXmlDoc, "/root/page/body/txt/anchored/SwAnchoredDrawObject[2]/bounds", "width")
              .toInt32();
    // Make sure the second triangle shape is within the page bounds (with ~1px tolerance).
    CPPUNIT_ASSERT_GREATEREQUAL(nShapeLeft + nShapeWidth, nPageLeft + nPageWidth + 21);
}

DECLARE_OOXMLEXPORT_TEST(testGraphicObjectFliph, "graphic-object-fliph.docx")
{
    CPPUNIT_ASSERT(getProperty<bool>(getShape(1), "HoriMirroredOnEvenPages"));
    CPPUNIT_ASSERT(getProperty<bool>(getShape(1), "HoriMirroredOnOddPages"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf113547, "tdf113547.docx")
{
    uno::Reference<beans::XPropertySet> xPropertySet(
        getStyles("NumberingStyles")->getByName("WWNum1"), uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xLevels(
        xPropertySet->getPropertyValue("NumberingRules"), uno::UNO_QUERY);
    comphelper::SequenceAsHashMap aProps(xLevels->getByIndex(0)); // 1st level
    // This was 0, first-line left margin of the numbering was lost.
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(-635), aProps["FirstLineIndent"].get<sal_Int32>());
}

DECLARE_OOXMLEXPORT_TEST(testTdf113399, "tdf113399.doc")
{
    // 0 padding was not preserved
    // In LO 0 is the default, but in OOXML format the default is 254 / 127
    uno::Reference<beans::XPropertySet> xPropSet(getShape(1), uno::UNO_QUERY);
    sal_Int32 nPaddingValue;
    xPropSet->getPropertyValue("TextLeftDistance") >>= nPaddingValue;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), nPaddingValue);
    xPropSet->getPropertyValue("TextRightDistance") >>= nPaddingValue;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), nPaddingValue);
    xPropSet->getPropertyValue("TextUpperDistance") >>= nPaddingValue;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), nPaddingValue);
    xPropSet->getPropertyValue("TextLowerDistance") >>= nPaddingValue;
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), nPaddingValue);
}

DECLARE_OOXMLEXPORT_TEST(testTdf114882, "tdf114882.docx")
{
    // fastserializer must not fail assertion because of mismatching elements
}

DECLARE_OOXMLEXPORT_TEST(testTdf49073, "tdf49073.docx")
{
    // test case for Asian phontic guide (ruby text.)
    sal_Unicode aRuby[3] = {0x304D,0x3082,0x3093};
    OUString sRuby(aRuby, SAL_N_ELEMENTS(aRuby));
    CPPUNIT_ASSERT_EQUAL(sRuby,getProperty<OUString>(getParagraph(1)->getStart(), "RubyText"));
    OUString sStyle = getProperty<OUString>( getParagraph(1)->getStart(), "RubyCharStyleName");
    uno::Reference<beans::XPropertySet> xPropertySet(getStyles("CharacterStyles")->getByName(sStyle), uno::UNO_QUERY );
    CPPUNIT_ASSERT_EQUAL(5.f, getProperty<float>(xPropertySet, "CharHeight"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(text::RubyAdjust_CENTER) ,getProperty<sal_Int16>(getParagraph(2)->getStart(),"RubyAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(text::RubyAdjust_BLOCK)  ,getProperty<sal_Int16>(getParagraph(3)->getStart(),"RubyAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(text::RubyAdjust_INDENT_BLOCK),getProperty<sal_Int16>(getParagraph(4)->getStart(),"RubyAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(text::RubyAdjust_LEFT)   ,getProperty<sal_Int16>(getParagraph(5)->getStart(),"RubyAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(text::RubyAdjust_RIGHT)  ,getProperty<sal_Int16>(getParagraph(6)->getStart(),"RubyAdjust"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(text::RubyPosition::INTER_CHARACTER)  ,getProperty<sal_Int16>(getParagraph(7)->getStart(),"RubyPosition"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf114703, "tdf114703.docx")
{
    uno::Reference<container::XIndexAccess> xRules
        = getProperty<uno::Reference<container::XIndexAccess>>(
            getStyles("NumberingStyles")->getByName("WWNum1"), "NumberingRules");
    // This was 0, level override "default" replaced the non-default value from
    // the abstract level.
    CPPUNIT_ASSERT_EQUAL(
        static_cast<sal_Int32>(-1000),
        comphelper::SequenceAsHashMap(xRules->getByIndex(0))["FirstLineIndent"].get<sal_Int32>());
}

DECLARE_OOXMLEXPORT_TEST(testTdf113258, "tdf113258.docx")
{
    uno::Reference<text::XTextRange> xShape(getShape(1), uno::UNO_QUERY);
    // This was 494, i.e. automatic spacing resulted in non-zero paragraph top
    // margin for the first paragraph in a shape.
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0),
                         getProperty<sal_Int32>(xShape->getStart(), "ParaTopMargin"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf113258_noBeforeAutospacing, "tdf113258_noBeforeAutospacing.docx")
{
    uno::Reference<text::XTextRange> xShape(getShape(1), uno::UNO_QUERY);
    // This was 0, i.e. disabled automatic spacing still resulted in zero paragraph
    // top margin for the first paragraph in a shape.
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1764),
                         getProperty<sal_Int32>(xShape->getStart(), "ParaTopMargin"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf142542_cancelledAutospacing, "tdf142542_cancelledAutospacing.docx")
{
    //Direct formatting disabling autoSpacing must override paragraph-style's autoSpacing.
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), getProperty<sal_Int32>(getParagraph(1), "ParaTopMargin"));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), getProperty<sal_Int32>(getParagraph(1), "ParaBottomMargin"));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf137655)
{
    loadAndSave("tdf137655.docx");
    xmlDocUniquePtr pXmlDoc = parseExport("word/document.xml");
    // These were 280.
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[2]/w:tc[1]/w:p[1]/w:pPr/w:spacing", "before", "0");
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[2]/w:tc[2]/w:p[1]/w:pPr/w:spacing", "before", "0");

    //tdf#142542: ensure that the original beforeAutospacing = 0 is not changed.
    assertXPath(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[2]/w:tc[1]/w:p[1]/w:pPr/w:spacing", "beforeAutospacing", "0");
}

DECLARE_OOXMLEXPORT_TEST(testTdf120511_eatenSection, "tdf120511_eatenSection.docx")
{
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    sal_Int32 nHeight = getXPath(pXmlDoc, "/root/page[1]/infos/prtBounds", "height").toInt32();
    sal_Int32 nWidth  = getXPath(pXmlDoc, "/root/page[1]/infos/prtBounds", "width").toInt32();
    CPPUNIT_ASSERT_MESSAGE( "Page1 is portrait", nWidth < nHeight );
    nHeight = getXPath(pXmlDoc, "/root/page[2]/infos/prtBounds", "height").toInt32();
    nWidth  = getXPath(pXmlDoc, "/root/page[2]/infos/prtBounds", "width").toInt32();
    CPPUNIT_ASSERT_MESSAGE( "Page2 is landscape", nWidth > nHeight );
}

DECLARE_OOXMLEXPORT_TEST(testTdf104354, "tdf104354.docx")
{
    uno::Reference<text::XTextRange> xShape(getShape(1), uno::UNO_QUERY);
    // This was 494, i.e. automatic spacing resulted in non-zero paragraph top
    // margin for the first paragraph in a text frame.
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0),
                         getProperty<sal_Int32>(xShape->getStart(), "ParaTopMargin"));
    // still 494 in the second paragraph
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(494),
                         getProperty<sal_Int32>(xShape->getEnd(), "ParaTopMargin"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf104354_firstParaInSection, "tdf104354_firstParaInSection.docx")
{
    uno::Reference<text::XFootnotesSupplier> xFootnotesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xFootnotes = xFootnotesSupplier->getFootnotes();
    uno::Reference<text::XText> xText(xFootnotes->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(494),
                         getProperty<sal_Int32>(getParagraphOfText(1, xText), "ParaTopMargin"));
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testPageBreak_after, "pageBreak_after.odt")
{
    // The problem was that the page breakAfter put the empty page BEFORE the table
    xmlDocUniquePtr pDump = parseLayoutDump();
    assertXPath(pDump, "/root/page[1]/body/tab", 1);
    // There should be two pages actually - a blank page after a page break.
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Did you fix?? Table should be on page one of two", 1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf107035, "tdf107035.docx")
{
    // Select the second run containing the page number field
    auto xPgNumRun = getRun(getParagraph(1), 2, "1");

    // Check that the page number field colour is set to "automatic".
    Color nPgNumColour = getProperty<Color>(xPgNumRun, "CharColor");
    CPPUNIT_ASSERT_EQUAL(COL_AUTO, nPgNumColour);
}

DECLARE_OOXMLEXPORT_TEST(testTdf112118_DOCX, "tdf112118.docx")
{
    // The resulting left margin width (2081) differs from its DOC counterpart from ww8export2.cxx,
    // because DOCX import does two conversions between mm/100 and twips on the route, losing one
    // twip on the road and arriving with a value that is 2 mm/100 less. I don't see an obvious way
    // to avoid that.
    static const struct {
        const char* styleName;
        struct {
            const char* sideName;
            sal_Int32 nMargin;
            sal_Int32 nBorderDistance;
            sal_Int32 nBorderWidth;
        } sideParams[4];
    } styleParams[] = {                      // Margin (MS-style), border distance, border width
        {
            "Standard",
            {
                { "Top", 496, 847, 159 },    //  851 twip, 24 pt (from text), 4.5 pt
                { "Left", 2081, 706, 212 },  // 1701 twip, 20 pt (from text), 6.0 pt
                { "Bottom", 1401, 564, 35 }, // 1134 twip, 16 pt (from text), 1.0 pt
                { "Right", 3471, 423, 106 }  // 2268 twip, 12 pt (from text), 3.0 pt
            }
        },
        {
            "Converted1",
            {
                { "Top", 847, 496, 159 },    //  851 twip, 24 pt (from edge), 4.5 pt
                { "Left", 706, 2081, 212 },  // 1701 twip, 20 pt (from edge), 6.0 pt
                { "Bottom", 564, 1401, 35 }, // 1134 twip, 16 pt (from edge), 1.0 pt
                { "Right", 423, 3471, 106 }  // 2268 twip, 12 pt (from edge), 3.0 pt
            }
        }
    };
    auto xStyles = getStyles("PageStyles");

    for (const auto& style : styleParams)
    {
        const OUString sName = OUString::createFromAscii(style.styleName);
        uno::Reference<beans::XPropertySet> xStyle(xStyles->getByName(sName), uno::UNO_QUERY_THROW);
        for (const auto& side : style.sideParams)
        {
            const OUString sSide = OUString::createFromAscii(side.sideName);
            const OString sStage = style.styleName + OString::Concat(" ") + side.sideName;

            sal_Int32 nMargin = getProperty<sal_Int32>(xStyle, sSide + "Margin");
            CPPUNIT_ASSERT_EQUAL_MESSAGE(OString(sStage + " margin width").getStr(),
                side.nMargin, nMargin);

            sal_Int32 nBorderDistance = getProperty<sal_Int32>(xStyle, sSide + "BorderDistance");
            CPPUNIT_ASSERT_EQUAL_MESSAGE(OString(sStage + " border distance").getStr(),
                side.nBorderDistance, nBorderDistance);

            table::BorderLine aBorder = getProperty<table::BorderLine>(xStyle, sSide + "Border");
            CPPUNIT_ASSERT_EQUAL_MESSAGE(OString(sStage + " border width").getStr(),
                side.nBorderWidth,
                sal_Int32(aBorder.OuterLineWidth + aBorder.InnerLineWidth + aBorder.LineDistance));

            // tdf#116472: check that AUTO border color is imported as black
            CPPUNIT_ASSERT_EQUAL_MESSAGE(OString(sStage + " border color").getStr(),
                sal_Int32(COL_BLACK), aBorder.Color);
        }
    }
}

DECLARE_OOXMLEXPORT_TEST(testTdf82177_outsideCellBorders, "tdf82177_outsideCellBorders.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference< text::XTextTable > xTable( xTables->getByIndex(0), uno::UNO_QUERY );
    uno::Reference< table::XCell > xCell = xTable->getCellByName( "E4" );
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt32>(0), getProperty<table::BorderLine2>(xCell, "TopBorder").LineWidth);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt32>(0), getProperty<table::BorderLine2>(xCell, "LeftBorder").LineWidth);
}

DECLARE_OOXMLEXPORT_TEST(testTdf82177_insideCellBorders, "tdf82177_insideCellBorders.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference< text::XTextTable > xTable( xTables->getByIndex(0), uno::UNO_QUERY );
    uno::Reference< table::XCell > xCell = xTable->getCellByName( "E4" );
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt32>(0), getProperty<table::BorderLine2>(xCell, "TopBorder").LineWidth);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt32>(0), getProperty<table::BorderLine2>(xCell, "LeftBorder").LineWidth);
}

DECLARE_OOXMLEXPORT_TEST(testTdf82177_tblBorders, "tdf82177_tblBorders.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference< text::XTextTable > xTable( xTables->getByIndex(0), uno::UNO_QUERY );
    uno::Reference< table::XCell > xCell = xTable->getCellByName( "A5" );
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt32>(0), getProperty<table::BorderLine2>(xCell, "BottomBorder").LineWidth);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt32>(0), getProperty<table::BorderLine2>(xCell, "LeftBorder").LineWidth);
    xCell.set(xTable->getCellByName( "E5" ));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt32>(0), getProperty<table::BorderLine2>(xCell, "TopBorder").LineWidth);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt32>(0), getProperty<table::BorderLine2>(xCell, "LeftBorder").LineWidth);
}

DECLARE_OOXMLEXPORT_TEST(testTdf119760_positionCellBorder, "tdf119760_positionCellBorder.docx")
{
    //inconsistent in Word even. 2016 positions on last row, 2003 positions on first cell.
    sal_Int32 nRowLeft = parseDump("/root/page/body/tab[4]/row[1]/infos/bounds", "left").toInt32();
    sal_Int32 nTextLeft  = parseDump("/root/page/body/tab[4]/row[1]/cell[1]/txt/infos/bounds", "left").toInt32();
    CPPUNIT_ASSERT( nRowLeft < nTextLeft );
}

DECLARE_OOXMLEXPORT_TEST(testTdf98620_environmentBiDi, "tdf98620_environmentBiDi.odt")
{
    CPPUNIT_ASSERT_EQUAL(2, getPages());
    CPPUNIT_ASSERT_EQUAL(text::WritingMode2::RL_TB, getProperty<sal_Int16>( getParagraph(1), "WritingMode" ));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(style::ParagraphAdjust_RIGHT), getProperty<sal_Int32>( getParagraph(1), "ParaAdjust" ));

    CPPUNIT_ASSERT_EQUAL(text::WritingMode2::LR_TB, getProperty<sal_Int16>( getParagraph(2), "WritingMode" ));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(style::ParagraphAdjust_RIGHT), getProperty<sal_Int32>( getParagraph(2), "ParaAdjust" ));
}

DECLARE_OOXMLEXPORT_TEST(testTdf116976, "tdf116976.docx")
{
    // This was 0, relative size of shape after bitmap was ignored.
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int16>(40),
                         getProperty<sal_Int16>(getShape(1), "RelativeWidth"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf116985, "tdf116985.docx")
{
    // Body frame width is 10800, 40% is the requested relative width, with 144
    // spacing to text on the left/right side.  So ideal width would be 4032,
    // was 3431. Allow one pixel tolerance, though.
    sal_Int32 nWidth
        = parseDump("/root/page[1]/body/txt[1]/anchored/fly/infos/bounds", "width").toInt32();
    CPPUNIT_ASSERT(nWidth > 4000);
}

DECLARE_OOXMLEXPORT_TEST(testTdf116801, "tdf116801.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTablesSupplier->getTextTables(),
                                                    uno::UNO_QUERY);
    // This raised a lang::IndexOutOfBoundsException, table was missing from
    // the import result.
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<text::XTextRange> xCell(xTable->getCellByName("D1"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("D1"), xCell->getString());
}

DECLARE_OOXMLEXPORT_TEST(testTdf107969, "tdf107969.docx")
{
    // A VML object in a footnote's tracked changes caused write past end of document.xml at export to docx.
    // After that, importing after export failed with
    // SAXParseException: '[word/document.xml line 2]: Extra content at the end of the document', Stream 'word/document.xml'.
}

DECLARE_OOXMLEXPORT_TEST(testOpenDocumentAsReadOnly, "open-as-read-only.docx")
{
    SwXTextDocument* pTextDoc = dynamic_cast<SwXTextDocument *>(mxComponent.get());
    CPPUNIT_ASSERT(pTextDoc);
    CPPUNIT_ASSERT(pTextDoc->GetDocShell()->IsSecurityOptOpenReadOnly());
}

DECLARE_OOXMLEXPORT_TEST(testNoDefault, "noDefault.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTextTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTextTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<text::XTextRange> xCell(xTable->getCellByName("A1"), uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xParaEnumAccess(xCell->getText(), uno::UNO_QUERY);
    uno::Reference<container::XEnumeration> xParaEnum = xParaEnumAccess->createEnumeration();
    uno::Reference<text::XTextRange> xPara(xParaEnum->nextElement(), uno::UNO_QUERY);

    // Row 1: color directly applied to the paragraph, overrides table and style colors
    CPPUNIT_ASSERT_EQUAL(Color(0x2E74B5), Color(ColorTransparency, getProperty<sal_Int32>(getRun(xPara,1), "CharColor")));

    // Row2: (still part of firstRow table-style) ought to use the Normal style color, not the table-style color(5B9BD5)
    //xCell.set(xTable->getCellByName("A2"), uno::UNO_QUERY);
    //xParaEnumAccess.set(xCell->getText(), uno::UNO_QUERY);
    //xParaEnum = xParaEnumAccess->createEnumeration();
    //xPara.set(xParaEnum->nextElement(), uno::UNO_QUERY);
    //CPPUNIT_ASSERT_EQUAL(sal_Int32(COL_LIGHTMAGENTA), getProperty<sal_Int32>(getRun(xPara,1), "CharColor"));

    // Row 3+: Normal style still applied, even if nothing is specified with w:default="1"
    xCell.set(xTable->getCellByName("A3"), uno::UNO_QUERY);
    xParaEnumAccess.set(xCell->getText(), uno::UNO_QUERY);
    xParaEnum = xParaEnumAccess->createEnumeration();
    xPara.set(xParaEnum->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(COL_LIGHTMAGENTA), getProperty<sal_Int32>(getRun(xPara,1), "CharColor"));
}

DECLARE_OOXMLEXPORT_TEST(testMarginsFromStyle, "margins_from_style.docx")
{
    // tdf#118521 paragraphs with direct formatting of top or bottom margins have
    // lost the other margin comes from paragraph style, getting a bad
    // margin from the default style

    // from direct formatting
    CPPUNIT_ASSERT_EQUAL(sal_Int32(35), getProperty<sal_Int32>(getParagraph(1), "ParaTopMargin"));
    // from paragraph style
    CPPUNIT_ASSERT_EQUAL(sal_Int32(106), getProperty<sal_Int32>(getParagraph(1), "ParaBottomMargin"));

    // from paragraph style
    CPPUNIT_ASSERT_EQUAL(sal_Int32(388), getProperty<sal_Int32>(getParagraph(3), "ParaTopMargin"));
    // from direct formatting
    CPPUNIT_ASSERT_EQUAL(sal_Int32(600), getProperty<sal_Int32>(getParagraph(3), "ParaBottomMargin"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf134784, "tdf134784.docx")
{
    uno::Reference<text::XText> textbox(getShape(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(3, getParagraphs(textbox));
    uno::Reference<text::XTextRange> xParagraph = getParagraphOfText(1, textbox);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(212), getProperty<sal_Int32>(xParagraph, "ParaBottomMargin"));

    // This wasn't zero (it was inherited from style of the previous paragraph in the main text)
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xParagraph, "ParaTopMargin"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf136955, "tdf134784.docx")
{
    uno::Reference<text::XText> textbox(getShape(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(3, getParagraphs(textbox));
    uno::Reference<text::XTextRange> xParagraph = getParagraphOfText(2, textbox);

    // These weren't zero (values inherited from style of the previous paragraph in the main text)
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xParagraph, "ParaBottomMargin"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xParagraph, "ParaTopMargin"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf104348_contextMargin, "tdf104348_contextMargin.docx")
{
    // tdf#104348 shows that ContextMargin belongs with Top/Bottom handling

    uno::Reference<beans::XPropertySet> xMyStyle(getStyles("ParagraphStyles")->getByName("MyStyle"), uno::UNO_QUERY);
    // from paragraph style - this is what direct formatting should equal
    sal_Int32 nMargin = getProperty<sal_Int32>(xMyStyle, "ParaBottomMargin");
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), nMargin);
    // from direct formatting
    CPPUNIT_ASSERT_EQUAL(nMargin, getProperty<sal_Int32>(getParagraph(2), "ParaBottomMargin"));
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
