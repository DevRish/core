/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <swmodeltestbase.hxx>

#include <com/sun/star/drawing/FillStyle.hpp>
#include <com/sun/star/text/RelOrientation.hpp>
#include <com/sun/star/text/XTextViewCursorSupplier.hpp>
#include <com/sun/star/text/XPageCursor.hpp>
#include <com/sun/star/text/XTextColumns.hpp>
#include <com/sun/star/text/XTextTable.hpp>
#include <com/sun/star/text/XTextTablesSupplier.hpp>
#include <com/sun/star/text/XTextFieldsSupplier.hpp>
#include <com/sun/star/text/XTextField.hpp>

constexpr OUStringLiteral DATA_DIRECTORY = u"/sw/qa/extras/ooxmlexport/data/";

class Test : public SwModelTestBase
{
public:
    Test() : SwModelTestBase(DATA_DIRECTORY, "Office Open XML Text") {}

protected:
    /**
     * Denylist handling
     */
    bool mustTestImportOf(const char* filename) const override {
        // If the testcase is stored in some other format, it's pointless to test.
        return OString(filename).endsWith(".docx");
    }
};

CPPUNIT_TEST_FIXTURE(Test, testTdf123621)
{
    loadAndSave("tdf123621.docx");
    xmlDocUniquePtr pXmlDocument = parseExport("word/document.xml");

    assertXPathContent(pXmlDocument, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor"
        "/wp:positionV/wp:posOffset", "1080135");
}

DECLARE_OOXMLEXPORT_TEST(testTdf131540, "tdf131540.odt")
{
    CPPUNIT_ASSERT_EQUAL(2, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // There are 2 OLEs test if one of them moved on save:
    CPPUNIT_ASSERT_EQUAL_MESSAGE("The shape1 moved on saving!", text::RelOrientation::PAGE_FRAME,
                                 getProperty<sal_Int16>(getShape(1), "HoriOrientRelation"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("The shape2 moved on saving!", text::RelOrientation::PAGE_FRAME,
                                 getProperty<sal_Int16>(getShape(2), "HoriOrientRelation"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf131801, "tdf131801.docx")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());

    xmlDocUniquePtr pDump = parseLayoutDump();
    // "1." is red
    CPPUNIT_ASSERT_EQUAL(OUString("1."), getXPath(pDump, "//page[1]/body/txt[1]/Special", "rText"));
    CPPUNIT_ASSERT_EQUAL(OUString("00ff0000"), getXPath(pDump, "//page[1]/body/txt[1]/Special/SwFont", "color"));
    // "2." is red
    CPPUNIT_ASSERT_EQUAL(OUString("2."), getXPath(pDump, "//page[1]/body/txt[2]/Special", "rText"));
    CPPUNIT_ASSERT_EQUAL(OUString("00ff0000"), getXPath(pDump, "//page[1]/body/txt[2]/Special/SwFont", "color"));
    // "3." is black
    CPPUNIT_ASSERT_EQUAL(OUString("3."), getXPath(pDump, "//page[1]/body/txt[3]/Special", "rText"));
    CPPUNIT_ASSERT_EQUAL(OUString("ffffffff"), getXPath(pDump, "//page[1]/body/txt[3]/Special/SwFont", "color"));
    // "4." is black
    CPPUNIT_ASSERT_EQUAL(OUString("4."), getXPath(pDump, "//page[1]/body/txt[4]/Special", "rText"));
    CPPUNIT_ASSERT_EQUAL(OUString("ffffffff"), getXPath(pDump, "//page[1]/body/txt[4]/Special/SwFont", "color"));
    // "5." is red
    CPPUNIT_ASSERT_EQUAL(OUString("5."), getXPath(pDump, "//page[1]/body/txt[5]/Special", "rText"));
    CPPUNIT_ASSERT_EQUAL(OUString("00ff0000"), getXPath(pDump, "//page[1]/body/txt[5]/Special/SwFont", "color"));
    // "6." is red
    CPPUNIT_ASSERT_EQUAL(OUString("6."), getXPath(pDump, "//page[1]/body/txt[6]/Special", "rText"));
    CPPUNIT_ASSERT_EQUAL(OUString("00ff0000"), getXPath(pDump, "//page[1]/body/txt[6]/Special/SwFont", "color"));
    // "7." is black
    CPPUNIT_ASSERT_EQUAL(OUString("7."), getXPath(pDump, "//page[1]/body/txt[7]/Special", "rText"));
    CPPUNIT_ASSERT_EQUAL(OUString("ffffffff"), getXPath(pDump, "//page[1]/body/txt[7]/Special/SwFont", "color"));
    // "8." is black
    CPPUNIT_ASSERT_EQUAL(OUString("8."), getXPath(pDump, "//page[1]/body/txt[8]/Special[1]", "rText"));
    CPPUNIT_ASSERT_EQUAL(OUString("ffffffff"), getXPath(pDump, "//page[1]/body/txt[8]/Special[1]/SwFont", "color"));

    xmlDocUniquePtr pXmlDocument = parseExport("word/document.xml");
    if (!pXmlDocument)
        return;

    assertXPath(pXmlDocument, "/w:document/w:body/w:p[1]/w:pPr/w:rPr/w:rStyle",
        "val", "Emphasis");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:pPr/w:rPr/w:rStyle",
        "val", "Emphasis");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[3]/w:pPr/w:rPr/w:rStyle", 0);
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[4]/w:pPr/w:rPr/w:rStyle", 0);
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[5]/w:pPr/w:rPr/w:rStyle",
        "val", "Emphasis");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[5]/w:pPr/w:rPr/w:sz",
        "val", "32");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[6]/w:pPr/w:rPr/w:rStyle",
        "val", "Emphasis");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[6]/w:pPr/w:rPr/w:sz",
        "val", "32");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[7]/w:pPr/w:rPr/w:rStyle", 0);
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[8]/w:pPr/w:rPr/w:rStyle", 0);
}

DECLARE_OOXMLEXPORT_TEST(testTdf133334_followPgStyle, "tdf133334_followPgStyle.odt")
{
    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf137850_compat14ZOrder, "tdf137850_compat14ZOrder.docx")
{
    // The file contains 2 shapes which have a different value of behindDoc.
    // Test that the textbox is hidden behind the arrow (for Word <= 2010/compatibilityMode==14)
    uno::Reference<text::XText> xShape(getShape(2), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("2015"), xShape->getString());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Textbox is in the background", false, getProperty<bool>(xShape, "Opaque"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf137850_compat15ZOrder, "tdf137850_compat15ZOrder.docx")
{
    // The file contains 2 shapes which have a different value of behindDoc.
    // Test that the textbox is not hidden behind the arrow (for Word >= 2013/compatibilityMode==15)
    uno::Reference<text::XText> xShape(getShape(2), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("2015"), xShape->getString());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Textbox is in the foreground", true, getProperty<bool>(xShape, "Opaque"));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf118701)
{
    loadAndSave("tdf118701.docx");
    // This was 6, related to moving inline images after the page breaks
    CPPUNIT_ASSERT_EQUAL(4, getPages());

    xmlDocUniquePtr pXmlDoc = parseExport();

    assertXPath(pXmlDoc, "/w:document/w:body/w:p[1]/w:pPr[1]/w:numPr", 0);
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[2]/w:pPr[1]/w:numPr", 0);
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[3]/w:pPr[1]/w:numPr", 0);
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[4]/w:pPr[1]/w:numPr", 1);

    // Keep numbering of the paragraph of the inline image
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[8]/w:pPr[1]/w:numPr", 0);
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[9]/w:pPr[1]/w:numPr", 1);
    // This was 0
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[10]/w:pPr[1]/w:numPr", 1);
}

DECLARE_OOXMLEXPORT_TEST(testTdf123388, "tdf123388.docx")
{
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // Tests new cell formula PRODUCT
    uno::Reference<text::XTextField> xEnumerationAccess1(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("PRODUCT(<B2:B3>)"), xEnumerationAccess1->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("640"), xEnumerationAccess1->getPresentation(false).trim());
}

DECLARE_OOXMLEXPORT_TEST(testTdf123401, "tdf123401.fodt")
{
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // Tests new cell formula AVERAGE
    uno::Reference<text::XTextField> xEnumerationAccess1(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AVERAGE(<A1:A2>)"), xEnumerationAccess1->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("3"), xEnumerationAccess1->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess2(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AVERAGE(<A1:A3>)"), xEnumerationAccess2->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("3"), xEnumerationAccess2->getPresentation(false).trim());

    xmlDocUniquePtr pXmlDoc = parseExport();

    // MEAN converted to AVERAGE
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[3]/w:tc/w:p/w:r[2]/w:instrText", " =AVERAGE(A1:A2)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[4]/w:tc/w:p/w:r[2]/w:instrText", " =AVERAGE(A1:A3)");
}

DECLARE_OOXMLEXPORT_TEST(testTdf116394, "tdf116394.docx")
{
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    uno::Reference<text::XTextField> xEnumerationAccess(xFields->nextElement(), uno::UNO_QUERY);

    // Without the fix in place, this test would have failed with
    // - Expected: ab=cd..
    // - Actual  : abcd..
    CPPUNIT_ASSERT_EQUAL(OUString("ab=cd.."), xEnumerationAccess->getPresentation(true).trim());

    xmlDocUniquePtr pXmlDoc = parseExport();
    if (!pXmlDoc)
        return;
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:p/w:r[2]/w:instrText", " MERGEFIELD ab=cd ");
}

DECLARE_OOXMLEXPORT_TEST(testTdf123356, "tdf123356.fodt")
{
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // Tests new cell formula COUNT
    uno::Reference<text::XTextField> xEnumerationAccess1(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("COUNT(<A1>)"), xEnumerationAccess1->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("1"), xEnumerationAccess1->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess2(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("COUNT(<A1:B2>)"), xEnumerationAccess2->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("4"), xEnumerationAccess2->getPresentation(false).trim());
}

DECLARE_OOXMLEXPORT_TEST(testTdf136404, "tdf136404.fodt")
{
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // Ignore empty cells or cells with text content with new interoperability functions COUNT, AVERAGE and PRODUCT
    uno::Reference<text::XTextField> xEnumerationAccess1(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("COUNT(<A1:F1>)"), xEnumerationAccess1->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("2"), xEnumerationAccess1->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess2(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AVERAGE(<B1:C1>)"), xEnumerationAccess2->getPresentation(true).trim());
    // This was 0
    CPPUNIT_ASSERT_EQUAL(OUString("** Expression is faulty **"), xEnumerationAccess2->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess3(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AVERAGE(<B1>)"), xEnumerationAccess3->getPresentation(true).trim());
    // This was 0
    CPPUNIT_ASSERT_EQUAL(OUString("** Expression is faulty **"), xEnumerationAccess3->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess4(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("COUNT(<B1:C1>)"), xEnumerationAccess4->getPresentation(true).trim());
    // This was 2
    CPPUNIT_ASSERT_EQUAL(OUString("0"), xEnumerationAccess4->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess5(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("COUNT(<B1>)"), xEnumerationAccess5->getPresentation(true).trim());
    // This was 1
    CPPUNIT_ASSERT_EQUAL(OUString("0"), xEnumerationAccess5->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess6(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("PRODUCT(<A1:F1>)"), xEnumerationAccess6->getPresentation(true).trim());
    // This was 0
    CPPUNIT_ASSERT_EQUAL(OUString("60"), xEnumerationAccess6->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess7(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AVERAGE(<A1:F1>)"), xEnumerationAccess7->getPresentation(true).trim());
    // This was 2
    CPPUNIT_ASSERT_EQUAL(OUString("8"), xEnumerationAccess7->getPresentation(false).trim());
}

DECLARE_OOXMLEXPORT_TEST(testTdf138739, "tdf138739.docx")
{
    uno::Reference<beans::XPropertySet> xParaProps(getParagraph(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Font type name does not match!", OUString("Comic Sans MS"),
                                 xParaProps->getPropertyValue("CharFontName").get<OUString>());
}

DECLARE_OOXMLEXPORT_TEST(testTdf123390, "tdf123390.fodt")
{
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // Tests new cell formula SIGN
    uno::Reference<text::XTextField> xEnumerationAccess1(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("SIGN(<A1>)"), xEnumerationAccess1->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("-1"), xEnumerationAccess1->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess2(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("SIGN(<C1>)"), xEnumerationAccess2->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("1"), xEnumerationAccess2->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess3(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("SIGN(<B1>)"), xEnumerationAccess3->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("0"), xEnumerationAccess3->getPresentation(false).trim());
}

DECLARE_OOXMLEXPORT_TEST(testTdf123354, "tdf123354.fodt")
{
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // Tests new cell formula SIGN
    uno::Reference<text::XTextField> xEnumerationAccess1(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("ABS(<A1>)"), xEnumerationAccess1->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("10"), xEnumerationAccess1->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess2(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("ABS(<C1>)"), xEnumerationAccess2->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("10"), xEnumerationAccess2->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess3(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("ABS(<B1>)"), xEnumerationAccess3->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("0"), xEnumerationAccess3->getPresentation(false).trim());
}

DECLARE_OOXMLEXPORT_TEST(testTdf123355, "tdf123355.docx")
{
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // Tests conversion of range IDs ABOVE, BELOW, LEFT and RIGHT
    uno::Reference<text::XTextField> xEnumerationAccess1(xFields->nextElement(), uno::UNO_QUERY);
    // Note: range ends at B4 here, which is a cell with text content
    CPPUNIT_ASSERT_EQUAL(OUString("average( <B2:B3> )"), xEnumerationAccess1->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("5,5"), xEnumerationAccess1->getPresentation(false).trim());

    // range ends at the end of the empty cells
    uno::Reference<text::XTextField> xEnumerationAccess6(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("SUM(<C6:A6>)"), xEnumerationAccess6->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("0"), xEnumerationAccess6->getPresentation(false).trim());

    // range starts at the first cell above D5
    uno::Reference<text::XTextField> xEnumerationAccess2(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AVERAGE(<D4:D1>)"), xEnumerationAccess2->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("5,33"), xEnumerationAccess2->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess3(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AVERAGE(<C2:C1>)"), xEnumerationAccess3->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("3,5"), xEnumerationAccess3->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess4(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AVERAGE(<D2:D2>)"), xEnumerationAccess4->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("5"), xEnumerationAccess4->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess5(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AVERAGE(<A2:A2>)"), xEnumerationAccess5->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("4"), xEnumerationAccess5->getPresentation(false).trim());

    xmlDocUniquePtr pXmlDoc = parseExport();
    if (!pXmlDoc)
        return;

    // keep original formula IDs
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[1]/w:tc[2]/w:p/w:r[2]/w:instrText", " =average( below )");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[2]/w:tc[2]/w:p/w:r[2]/w:instrText", " =AVERAGE(LEFT)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[2]/w:tc[3]/w:p/w:r[2]/w:instrText", " =AVERAGE(RIGHT)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[3]/w:tc[3]/w:p/w:r[2]/w:instrText", " =AVERAGE(ABOVE)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[5]/w:tc[4]/w:p/w:r[2]/w:instrText", " =AVERAGE(ABOVE)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[6]/w:tc[4]/w:p/w:r[2]/w:instrText", " =SUM(LEFT)");
}

DECLARE_OOXMLEXPORT_TEST(testTdf123382, "tdf123382.docx")
{
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // Tests conversion of range IDs ABOVE, BELOW, LEFT and RIGHT
    uno::Reference<text::XTextField> xEnumerationAccess1(xFields->nextElement(), uno::UNO_QUERY);
    // Note: range ends at B4 here, which is a cell with text content
    CPPUNIT_ASSERT_EQUAL(OUString("MAX(<B1:D1>)"), xEnumerationAccess1->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("12"), xEnumerationAccess1->getPresentation(false).trim());

    // range ends at the end of the empty cells
    uno::Reference<text::XTextField> xEnumerationAccess6(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("MAX(<C4:D4>)"), xEnumerationAccess6->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("9"), xEnumerationAccess6->getPresentation(false).trim());

    // range starts at the first cell above D5
    uno::Reference<text::XTextField> xEnumerationAccess2(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("SUM(<B3:D3>)"), xEnumerationAccess2->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("30"), xEnumerationAccess2->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess3(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("MAX(<C2:A2>)"), xEnumerationAccess3->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("7"), xEnumerationAccess3->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess4(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("MAX(<B2:D2>)"), xEnumerationAccess4->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("7"), xEnumerationAccess4->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess5(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("MAX(<D2:D4>)"), xEnumerationAccess5->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("12"), xEnumerationAccess5->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess7(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("MAX(<B2:B4>)"), xEnumerationAccess7->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("10"), xEnumerationAccess7->getPresentation(false).trim());

    xmlDocUniquePtr pXmlDoc = parseExport();
    if (!pXmlDoc)
        return;

    // keep original formula IDs
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[1]/w:tc[1]/w:p/w:r[2]/w:instrText", " =MAX(RIGHT)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[2]/w:tc[1]/w:p/w:r[2]/w:instrText", " =MAX(RIGHT)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[3]/w:tc[1]/w:p/w:r[2]/w:instrText", " =SUM(RIGHT)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[1]/w:tc[2]/w:p/w:r[2]/w:instrText", " =MAX(BELOW)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[4]/w:tc[2]/w:p/w:r[2]/w:instrText", " =MAX(RIGHT)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[1]/w:tc[4]/w:p/w:r[2]/w:instrText", " =MAX(BELOW)");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl/w:tr[2]/w:tc[4]/w:p/w:r[2]/w:instrText", " =MAX(LEFT)");
}

DECLARE_OOXMLEXPORT_TEST(testTdf122648, "tdf122648.docx")
{
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // table formula conversion worked only in the first table
    uno::Reference<text::XTextField> xEnumerationAccess1(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("<A1>"), xEnumerationAccess1->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("1"), xEnumerationAccess1->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess2(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("SUM(<A1:B1>)"), xEnumerationAccess2->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("2"), xEnumerationAccess2->getPresentation(false).trim());

    // These were <?> and SUM(<?:?>) with zero values
    uno::Reference<text::XTextField> xEnumerationAccess3(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("<A1>"), xEnumerationAccess3->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("1"), xEnumerationAccess3->getPresentation(false).trim());

    uno::Reference<text::XTextField> xEnumerationAccess4(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("SUM(<A1:B1>)"), xEnumerationAccess4->getPresentation(true).trim());
    CPPUNIT_ASSERT_EQUAL(OUString("2"), xEnumerationAccess4->getPresentation(false).trim());

    xmlDocUniquePtr pXmlDoc = parseExport();
    if (!pXmlDoc)
        return;

    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl[1]/w:tr[1]/w:tc[2]/w:p/w:r[2]/w:instrText", " =A1");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl[1]/w:tr[2]/w:tc[2]/w:p/w:r[2]/w:instrText", " =SUM(A1:B1)");
    // These were =<?> and =SUM(<?:?>)
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl[2]/w:tr[1]/w:tc[2]/w:p/w:r[2]/w:instrText", " =A1");
    assertXPathContent(pXmlDoc, "/w:document/w:body/w:tbl[2]/w:tr[2]/w:tc[2]/w:p/w:r[2]/w:instrText", " =SUM(A1:B1)");
}

DECLARE_OOXMLEXPORT_TEST(testTdf98000_changePageStyle, "tdf98000_changePageStyle.odt")
{
    uno::Reference<frame::XModel> xModel(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XTextViewCursorSupplier> xTextViewCursorSupplier(xModel->getCurrentController(), uno::UNO_QUERY);

    uno::Reference<text::XPageCursor> xCursor(xTextViewCursorSupplier->getViewCursor(), uno::UNO_QUERY_THROW);
    OUString sPageOneStyle = getProperty<OUString>( xCursor, "PageStyleName" );

    xCursor->jumpToNextPage();
    OUString sPageTwoStyle = getProperty<OUString>( xCursor, "PageStyleName" );
    CPPUNIT_ASSERT_MESSAGE("Different page1/page2 styles", sPageOneStyle != sPageTwoStyle);
}

DECLARE_OOXMLEXPORT_TEST(testTdf135216_evenOddFooter, "tdf135216_evenOddFooter.odt")
{
    uno::Reference<frame::XModel> xModel(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XTextViewCursorSupplier> xTextViewCursorSupplier(xModel->getCurrentController(), uno::UNO_QUERY);
    uno::Reference<text::XPageCursor> xCursor(xTextViewCursorSupplier->getViewCursor(), uno::UNO_QUERY);

    // get LO page style for the first page (even page #2)
    OUString pageStyleName = getProperty<OUString>(xCursor, "PageStyleName");
    uno::Reference<container::XNameAccess> xPageStyles = getStyles("PageStyles");
    uno::Reference<style::XStyle> xPageStyle(xPageStyles->getByName(pageStyleName), uno::UNO_QUERY);

    xCursor->jumpToFirstPage();  // Even/Left page #2
    uno::Reference<text::XText> xFooter = getProperty<uno::Reference<text::XText>>(xPageStyle, "FooterTextLeft");
    CPPUNIT_ASSERT_EQUAL(OUString("even page"), xFooter->getString());

    xCursor->jumpToNextPage();
    pageStyleName = getProperty<OUString>(xCursor, "PageStyleName");
    xPageStyle.set(xPageStyles->getByName(pageStyleName), uno::UNO_QUERY);
    xFooter.set(getProperty<uno::Reference<text::XText>>(xPageStyle, "FooterTextFirst"));
    CPPUNIT_ASSERT_EQUAL(OUString("odd page - first footer"), xFooter->getString());

    xCursor->jumpToNextPage();
    pageStyleName = getProperty<OUString>(xCursor, "PageStyleName");
    xPageStyle.set(xPageStyles->getByName(pageStyleName), uno::UNO_QUERY);
    xFooter.set(getProperty<uno::Reference<text::XText>>(xPageStyle, "FooterTextLeft"));
    CPPUNIT_ASSERT_EQUAL(OUString("even page"), xFooter->getString());

    // The contents of paragraph 2 should be the page number (2) located on page 1.
    getParagraph(2, "2");
}

DECLARE_OOXMLEXPORT_TEST(testTdf136929_framesOfParagraph, "tdf136929_framesOfParagraph.odt")
{
    // Before this fix, the image was placed in the footer instead of in the text body - messing everything up.
    CPPUNIT_ASSERT_EQUAL_MESSAGE( "Number of Pages", 5, getPages() );
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Header2 text", OUString("* | *"), parseDump("/root/page[4]/footer/txt"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf136589_paraHadField, "tdf136589_paraHadField.docx")
{
    // The section break should not add an additional CR - which equals an empty page two.
    CPPUNIT_ASSERT_EQUAL(2, getPages());

    //tdf#118711 - don't explicitly specify the default page style at the beginning of the document
    uno::Reference<beans::XPropertySet> xPara(getParagraph(1), uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT_EQUAL(uno::Any(), xPara->getPropertyValue("PageDescName"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf133370_columnBreak, "tdf133370_columnBreak.odt")
{
    // Since non-DOCX formats ignores column breaks in non-column situations, don't export to docx.
    CPPUNIT_ASSERT_EQUAL(1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf134649_pageBreak, "tdf134649_pageBreak.fodt")
{
    // This was 1 (missing page break between tables).
    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf135343_columnSectionBreak_c14, "tdf135343_columnSectionBreak_c14.docx")
{
    uno::Reference<beans::XPropertySet> xTextSection = getProperty<uno::Reference<beans::XPropertySet>>(getParagraph(1), "TextSection");
    uno::Reference<text::XTextColumns> xTextColumns = getProperty<uno::Reference<text::XTextColumns>>(xTextSection, "TextColumns");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Section one's columns", sal_Int16(2), xTextColumns->getColumnCount());

    // Old Word 2010 version - nextColumn breaks inside column sections are just treated as regular column breaks.
    //xTextSection = getProperty<uno::Reference<beans::XPropertySet>>(getParagraph(12, "RTL 2"), "TextSection");
    //xTextColumns = getProperty<uno::Reference<text::XTextColumns>>(xTextSection, "TextColumns");
    //CPPUNIT_ASSERT_EQUAL_MESSAGE("Section four's columns", sal_Int16(3), xTextColumns->getColumnCount());
    //CPPUNIT_ASSERT_EQUAL(1, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf135973, "tdf135973.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(2), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(OUString("1."), getProperty<OUString>(xPara, "ListLabelString"));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(3), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(OUString("2."), getProperty<OUString>(xPara, "ListLabelString"));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(5), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(OUString("1."), getProperty<OUString>(xPara, "ListLabelString"));
    }
    {
        uno::Reference<beans::XPropertySet> xPara(getParagraph(6), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(OUString("2."), getProperty<OUString>(xPara, "ListLabelString"));
    }
}

DECLARE_OOXMLEXPORT_TEST(testTdf135343_columnSectionBreak_c14v2, "tdf135343_columnSectionBreak_c14v2.docx")
{
    // In this Word 2010 v2, section three was changed to start with a nextColumn break instead of a continuous break.
    // The previous section has no columns, so this time start the columns on a new page.
    uno::Reference<beans::XPropertySet> xTextSection = getProperty<uno::Reference<beans::XPropertySet>>(getParagraph(10, ""), "TextSection");
    uno::Reference<text::XTextColumns> xTextColumns = getProperty<uno::Reference<text::XTextColumns>>(xTextSection, "TextColumns");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Section three's columns", sal_Int16(3), xTextColumns->getColumnCount());
    //CPPUNIT_ASSERT_EQUAL(2, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf135343_columnSectionBreak_c12v3, "tdf135343_columnSectionBreak_c12v3.docx")
{
    // In this Word 20-3 v3, section one and two have different number of columns. It acts like a page break.
    uno::Reference<beans::XPropertySet> xTextSection = getProperty<uno::Reference<beans::XPropertySet>>(getParagraph(1, "Four columns,"), "TextSection");
    uno::Reference<text::XTextColumns> xTextColumns = getProperty<uno::Reference<text::XTextColumns>>(xTextSection, "TextColumns");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Section one's columns", sal_Int16(4), xTextColumns->getColumnCount());

    xTextSection = getProperty<uno::Reference<beans::XPropertySet>>(getParagraph(6, "RTL 2"), "TextSection");
    xTextColumns = getProperty<uno::Reference<text::XTextColumns>>(xTextSection, "TextColumns");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Section two's columns", sal_Int16(2), xTextColumns->getColumnCount());
    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf135343_columnSectionBreak_c15, "tdf135343_columnSectionBreak_c15.docx")
{
    // Word 2013+ version - nextColumn breaks inside column sections are always handled like nextPage breaks.
    uno::Reference<beans::XPropertySet> xTextSection = getProperty<uno::Reference<beans::XPropertySet>>(getParagraph(12, "RTL 2"), "TextSection");
    uno::Reference<text::XTextColumns> xTextColumns = getProperty<uno::Reference<text::XTextColumns>>(xTextSection, "TextColumns");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Section four's columns", sal_Int16(3), xTextColumns->getColumnCount());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Fits on two pages", 2, getPages());
}

DECLARE_OOXMLEXPORT_TEST(testTdf121669_equalColumns, "tdf121669_equalColumns.docx")
{
    uno::Reference<beans::XPropertySet> xTextSection = getProperty< uno::Reference<beans::XPropertySet> >(getParagraph(1), "TextSection");
    CPPUNIT_ASSERT(xTextSection.is());
    uno::Reference<text::XTextColumns> xTextColumns = getProperty< uno::Reference<text::XTextColumns> >(xTextSection, "TextColumns");
    // The property was ignored when deciding at export whether the columns were equal or not. Layout isn't reliable.
    CPPUNIT_ASSERT(getProperty<bool>(xTextColumns, "IsAutomatic"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf132149_pgBreak, "tdf132149_pgBreak.odt")
{
    // This 5 page document is designed to visually exaggerate the problems
    // of emulating LO's followed-by-page-style into MSWord's sections.
    // While much has been improved, there are extra pages present, which still need fixing.
    xmlDocUniquePtr pDump = parseLayoutDump();

    // No header on pages 1,2,3.
    assertXPath(pDump, "//page[2]/header", 0);

    // Margins/page orientation between Right and Left page styles are different
    assertXPath(pDump, "//page[1]/infos/prtBounds", "left", "1134");  //Right page style
    assertXPath(pDump, "//page[2]/infos/prtBounds", "left", "2268");  //Left page style

    assertXPath(pDump, "//page[1]/infos/bounds", "width", "8391");  //landscape
    assertXPath(pDump, "//page[2]/infos/bounds", "width", "5953");  //portrait
    // This two-line 3rd page ought not to exist. DID YOU FIX ME? The real page 3 should be "8391" landscape.
    assertXPath(pDump, "//page[3]/infos/bounds", "width", "5953");
    // This really ought to be on odd page 3, but now it is on odd page 5.
    assertXPath(pDump, "//page[5]/infos/bounds", "width", "8391");
    assertXPath(pDump, "//page[5]/infos/prtBounds", "right", "6122");  //Left page style


    //Page style change here must not be lost. This SHOULD be on page 4, but sadly it is not.
    assertXPathContent(pDump, "//page[6]/header/txt", "First Page Style");
    CPPUNIT_ASSERT(getXPath(pDump, "//page[6]/body/txt[1]/Text[1]", "Portion").startsWith("Lorem ipsum"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf132149_pgBreakB, "tdf132149_pgBreakB.odt")
{
    // This 5 page document is designed to visually exaggerate the problems
    // of emulating LO's followed-by-page-style into MSWord's sections.
    xmlDocUniquePtr pDump = parseLayoutDump();

    //Sanity check to ensure the correct page is being tested. This SHOULD be on page 3, but sadly it is not.
    CPPUNIT_ASSERT(getXPath(pDump, "//page[5]/body/txt[1]/Text[1]", "Portion").startsWith("Lorem ipsum"));
    //Prior to this fix, the original alternation between portrait and landscape was completely lost.
    assertXPath(pDump, "//page[5]/infos/bounds", "width", "8391");  //landscape
}

DECLARE_OOXMLEXPORT_TEST(testTdf132149_pgBreak2, "tdf132149_pgBreak2.odt")
{
    // This 3 page document is designed to visually exaggerate the problems
    // of emulating LO's followed-by-page-style into MSWord's sections.

    // The only specified page style change should be between page 1 and 2.
    // When the first paragraph was split into 3, each paragraph specified a page break. The last was unnecessary.
    uno::Reference<beans::XPropertySet> xParaThree(getParagraph(3), uno::UNO_QUERY_THROW);
    CPPUNIT_ASSERT_EQUAL(uno::Any(), xParaThree->getPropertyValue("PageDescName"));
    // The ODT is only 2 paragraphs, but a hack to get the right page style breaks para1 into pieces.
    // This was 4 paragraphs - the unnecessary page break had hacked in another paragraph split.
    CPPUNIT_ASSERT_LESSEQUAL( 3, getParagraphs() );
}

DECLARE_OOXMLEXPORT_TEST(testTdf136952_pgBreak3B, "tdf136952_pgBreak3B.odt")
{
    // This 4 page document is designed to visually exaggerate the problems
    // of emulating LO's followed-by-page-style into MSWord's sections.
    xmlDocUniquePtr pDump = parseLayoutDump();

    //page::breakAfter must not be lost.
    //Prior to this bug fix, the Lorem ipsum paragraph was in the middle of a portrait page, with no switch to landscape occurring.
    CPPUNIT_ASSERT(getXPath(pDump, "//page[3]/body/txt[1]/Text[1]", "Portion").startsWith("Lorem ipsum"));
    assertXPath(pDump, "//page[3]/infos/bounds", "width", "8391");  //landscape
}

DECLARE_OOXMLEXPORT_TEST(testTdf135949_anchoredBeforeBreak, "tdf135949_anchoredBeforeBreak.docx")
{
    xmlDocUniquePtr pDump = parseLayoutDump();
    //The picture was shown on page 2, because the empty paragraph before the page break was removed
    assertXPath(pDump, "//page[1]/body/txt/anchored/fly", 1);
}

DECLARE_OOXMLEXPORT_TEST(testTdf129452_excessBorder, "tdf129452_excessBorder.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTextTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTextTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY_THROW);

    // The outside border should not be applied on inside cells. The merge doesn't extend to the table bottom.
    // [Note: as humans, we would call this cell D3, but since row 4 hasn't been analyzed yet, it is considered column C.]
    table::BorderLine2 aBorder = getProperty<table::BorderLine2>(xTable->getCellByName("C3"), "BottomBorder");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("No bottom border on merged cell", sal_uInt32(0), aBorder.LineWidth);

    // [Note: as humans, we would call this cell C3, but since row 4 hasn't been analyzed yet, it is considered column B.]
    aBorder = getProperty<table::BorderLine2>(xTable->getCellByName("B3"), "BottomBorder");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("No bottom border on merged cell", sal_uInt32(0), aBorder.LineWidth);
}

DECLARE_OOXMLEXPORT_TEST(testTdf132898_missingBorder, "tdf132898_missingBorder.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTextTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTextTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY);

    // The bottom border from the last merged cell was not showing
    table::BorderLine2 aBorder = getProperty<table::BorderLine2>(xTable->getCellByName("A1"), "BottomBorder");
    CPPUNIT_ASSERT_MESSAGE("Bottom border on merged cell", aBorder.LineWidth > 0);
}

DECLARE_OOXMLEXPORT_TEST(testTdf132898_extraBorder, "tdf132898_extraBorder.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTextTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTextTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY);

    // A border defined on an earlier merged cell was showing
    table::BorderLine2 aBorder = getProperty<table::BorderLine2>(xTable->getCellByName("C1"), "BottomBorder");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("No bottom border on merged cell", sal_uInt32(0), aBorder.LineWidth);
    // MS Word is interesting here. 2/3 of the merged cell has the right border, so what to do?
    aBorder = getProperty<table::BorderLine2>(xTable->getCellByName("C1"), "RightBorder");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("No right border on merged cell", sal_uInt32(0), aBorder.LineWidth);
}

DECLARE_OOXMLEXPORT_TEST(testTdf131561_necessaryBorder, "tdf131561_necessaryBorder.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTextTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTextTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY);

    // Hand-crafted pre-emptive test to make sure borders aren't lost.
    // MS Word is interesting here. 2/3 of the merged cell has the right border, so what to do?
    table::BorderLine2 aBorderR = getProperty<table::BorderLine2>(xTable->getCellByName("A1"), "RightBorder");
    table::BorderLine2 aBorderL = getProperty<table::BorderLine2>(xTable->getCellByName("B1"), "LeftBorder");
    CPPUNIT_ASSERT_MESSAGE("Border between A1 and B1", (aBorderR.LineWidth + aBorderL.LineWidth) > 0);
    aBorderR = getProperty<table::BorderLine2>(xTable->getCellByName("A3"), "RightBorder");
    aBorderL = getProperty<table::BorderLine2>(xTable->getCellByName("B3"), "LeftBorder");
    CPPUNIT_ASSERT_MESSAGE("Border between A3 and B3", (aBorderR.LineWidth + aBorderL.LineWidth) > 0);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf135655)
{
    loadAndSave("tdf135655.odt");
    const xmlDocUniquePtr pExpDoc = parseExport();
    const OUString sXFillColVal = getXPath(pExpDoc, "/w:document/w:body/w:p/w:r/w:object/v:shape", "fillcolor");
    CPPUNIT_ASSERT_EQUAL(OUString("#00A933"), sXFillColVal);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf138374)
{
    loadAndSave("tdf138374.odt");
    xmlDocUniquePtr pXmlDocument = parseExport("word/document.xml");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:r/w:pict/v:shape", "fillcolor", "#ffd320");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:r/w:pict/v:shape", "coordsize", "1315,6116");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:r/w:pict/v:shape", "path",
            "m0,0l1314,0l1314,5914l416,5914l416,6115l106,5715l416,5415l416,5715l1014,5715l1014,224l0,224l0,16l0,0e");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:r/w:pict/v:shape", "style",
            "position:absolute;margin-left:394.3pt;margin-top:204pt;width:37.2pt;height:173.3pt;mso-wrap-style:none;v-text-anchor:middle");
}

DECLARE_OOXMLEXPORT_TEST(testTdf134609_gridAfter, "tdf134609_gridAfter.docx")
{
    uno::Reference<text::XTextTablesSupplier> xTextTablesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xTextTablesSupplier->getTextTables(), uno::UNO_QUERY);
    uno::Reference<text::XTextTable> xTable(xTables->getByIndex(0), uno::UNO_QUERY);

    // Table borders (width 159) apply to edge cells, even in uneven cases caused by gridBefore/gridAfter,
    table::BorderLine2 aBorder = getProperty<table::BorderLine2>(xTable->getCellByName("A1"), "RightBorder");
    CPPUNIT_ASSERT_MESSAGE("Right border before gridAfter cells", aBorder.LineWidth > 0);
    aBorder = getProperty<table::BorderLine2>(xTable->getCellByName("E2"), "LeftBorder");
    CPPUNIT_ASSERT_MESSAGE("Left edge border after gridBefore cells", aBorder.LineWidth > 100);
    aBorder = getProperty<table::BorderLine2>(xTable->getCellByName("E2"), "TopBorder");
    // but only for left/right borders, not top and bottom.
    // So somewhat inconsistently, gridBefore/After affects outside edges of columns, but not of rows.
    // insideH borders are width 53. (no insideV borders defined to emphasize missing edge borders)
    CPPUNIT_ASSERT_MESSAGE("Top border on 'inside' cell", aBorder.LineWidth > 0);
    CPPUNIT_ASSERT_MESSAGE("Top border is not an edge border", aBorder.LineWidth < 100);
}

DECLARE_OOXMLEXPORT_TEST(testTdf135329_lostImage, "tdf135329_lostImage.odt")
{
    // the character-anchored image was being skipped, since searchNext didn't notice it.
    uno::Reference<beans::XPropertySet> xImageProps(getShape(2), uno::UNO_QUERY_THROW);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf136441_commentInFootnote)
{
    loadAndReload("tdf136441_commentInFootnote.odt");
    // failed to load without error if footnote contained a comment.
    // (MS Word's UI doesn't allow adding comments to a footnote.)
}

DECLARE_OOXMLEXPORT_TEST(testTdf137683_charHighlightTests, "tdf137683_charHighlightTests.docx")
{
    // Don't export unnecessary w:highlight="none" (Unnecessary one intentionally hand-added to original .docx)
    xmlDocUniquePtr pXmlStyles = parseExport("word/styles.xml");
    if (pXmlStyles)
        assertXPath(pXmlStyles, "/w:styles/w:style[@w:styleId='Normal']/w:rPr/w:highlight", 0);

    uno::Reference<beans::XPropertySet> xRun(getRun(getParagraph(10), 2, "no highlight"), uno::UNO_QUERY_THROW);
    // This test was failing with a cyan charHighlight of 65535 (0x00FFFF), instead of COL_TRANSPARENT (0xFFFFFFFF)
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(COL_AUTO), getProperty<sal_Int32>(xRun, "CharHighlight"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf138345_charStyleHighlight, "tdf138345_charStyleHighlight.docx")
{
    // MS Word ignores the w:highlight setting in character styles. So shall we.
    // Without the fix, there would be an orange or yellow background on some words.
    const uno::Reference<beans::XPropertySet> xRun(getRun(getParagraph(1), 2, "orange background"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(COL_TRANSPARENT), getProperty<sal_Int32>(xRun,"CharHighlight"));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(COL_TRANSPARENT), getProperty<sal_Int32>(xRun,"CharBackColor"));
}

DECLARE_OOXMLEXPORT_TEST(testTdf125268, "tdf125268.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    const uno::Reference<beans::XPropertySet> xRun(getRun(getParagraph(1), 1, "Hello"), uno::UNO_QUERY);
    // Without the fix in place, this test would have failed with
    // - Expected: -1
    // - Actual  : 0
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(COL_TRANSPARENT), getProperty<sal_Int32>(xRun,"CharHighlight"));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(COL_BLACK), getProperty<sal_Int32>(xRun,"CharBackColor"));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf138345_numberingHighlight)
{
    loadAndSave("tdf138345_numberingHighlight.docx");
    // Before the fix, the highlight was completely lost.
    xmlDocUniquePtr pXmlStyles = parseExport("word/numbering.xml");
    if (pXmlStyles)
        assertXPath(pXmlStyles, "/w:numbering/w:abstractNum[@w:abstractNumId='1']/w:lvl[@w:ilvl='0']/w:rPr/w:highlight", "val", "red");
}

DECLARE_OOXMLEXPORT_TEST(testTdf134063, "tdf134063.docx")
{
    CPPUNIT_ASSERT_EQUAL(2, getPages());

    xmlDocUniquePtr pDump = parseLayoutDump();

    // There are three tabs with default width
    CPPUNIT_ASSERT_EQUAL(sal_Int32(720), getXPath(pDump, "//page[1]/body/txt[1]/Text[1]", "nWidth").toInt32());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(720), getXPath(pDump, "//page[1]/body/txt[1]/Text[2]", "nWidth").toInt32());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(720), getXPath(pDump, "//page[1]/body/txt[1]/Text[3]", "nWidth").toInt32());
}

DECLARE_OOXMLEXPORT_TEST(TestTdf135653, "tdf135653.docx")
{
    uno::Reference<beans::XPropertySet> xOLEProps(getShape(1), uno::UNO_QUERY_THROW);
    drawing::FillStyle nFillStyle = static_cast<drawing::FillStyle>(-1);
    xOLEProps->getPropertyValue("FillStyle") >>= nFillStyle;
    Color aFillColor(COL_AUTO);
    xOLEProps->getPropertyValue("FillColor") >>= aFillColor;

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Fill style setting does not match!",
                                 drawing::FillStyle::FillStyle_SOLID, nFillStyle);
    Color aExpectedColor;
    aExpectedColor.SetRed(255);
    aExpectedColor.SetGreen(0);
    aExpectedColor.SetBlue(0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("OLE bg color does not match!", aExpectedColor, aFillColor);
}

DECLARE_OOXMLEXPORT_TEST(testTdf135665, "tdf135665.docx")
{
    uno::Reference<beans::XPropertySet> xOLEProps1(getShape(1), uno::UNO_QUERY_THROW);
    uno::Reference<beans::XPropertySet> xOLEProps2(getShape(2), uno::UNO_QUERY_THROW);
    bool bSurroundContour1 = false;
    bool bSurroundContour2 = false;
    xOLEProps1->getPropertyValue("SurroundContour") >>= bSurroundContour1;
    xOLEProps2->getPropertyValue("SurroundContour") >>= bSurroundContour2;

    CPPUNIT_ASSERT_EQUAL_MESSAGE("OLE tight wrap setting not imported correctly", true, bSurroundContour1);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("OLE tight wrap setting not imported correctly", false, bSurroundContour2);
}

CPPUNIT_TEST_FIXTURE(Test, testAtPageShapeRelOrientation)
{
    loadAndSave("rotated_shape.fodt");
    // invalid combination of at-page anchor and horizontal-rel="paragraph"
    // caused relativeFrom="column" instead of relativeFrom="page"

    xmlDocUniquePtr pXmlDocument = parseExport("word/document.xml");

    assertXPathContent(pXmlDocument, "/w:document/w:body/w:p/w:r/mc:AlternateContent[1]/mc:Choice/w:drawing/wp:anchor"
        "/wp:positionH/wp:posOffset", "-480060");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p/w:r/mc:AlternateContent[1]/mc:Choice/w:drawing/wp:anchor"
        "/wp:positionH", "relativeFrom", "page");
    assertXPathContent(pXmlDocument, "/w:document/w:body/w:p/w:r/mc:AlternateContent[1]/mc:Choice/w:drawing/wp:anchor"
        "/wp:positionV/wp:posOffset", "8147685");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p/w:r/mc:AlternateContent[1]/mc:Choice/w:drawing/wp:anchor"
        "/wp:positionV", "relativeFrom", "page");

    // same for sw
    assertXPathContent(pXmlDocument, "/w:document/w:body/w:p/w:r/w:drawing/wp:anchor"
        "/wp:positionH/wp:posOffset", "720090");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p/w:r/w:drawing/wp:anchor"
        "/wp:positionH", "relativeFrom", "page");
    assertXPathContent(pXmlDocument, "/w:document/w:body/w:p/w:r/w:drawing/wp:anchor"
        "/wp:positionV/wp:posOffset", "1080135");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p/w:r/w:drawing/wp:anchor"
        "/wp:positionV", "relativeFrom", "page");

    // now test text rotation -> VML writing direction
    assertXPath(pXmlDocument, "/w:document/w:body/w:p/w:r/mc:AlternateContent[1]/mc:Fallback/w:pict/v:shape/v:textbox", "style", "mso-layout-flow-alt:bottom-to-top");
    // text wrap -> VML
    assertXPath(pXmlDocument, "/w:document/w:body/w:p/w:r/mc:AlternateContent[1]/mc:Fallback/w:pict/v:shape/w10:wrap", "type", "none");
    // vertical alignment -> VML
    OUString const style = getXPath(pXmlDocument, "/w:document/w:body/w:p/w:r/mc:AlternateContent[1]/mc:Fallback/w:pict/v:shape", "style");
    CPPUNIT_ASSERT(style.indexOf("v-text-anchor:middle") != -1);
}

CPPUNIT_TEST_FIXTURE(Test, testVMLallowincell)
{
    loadAndSave("shape-atpage-in-table.fodt");
    xmlDocUniquePtr pXmlDocument = parseExport("word/document.xml");

    // VML o:allowincell, apparently the default is "t"
    assertXPath(pXmlDocument, "/w:document/w:body/w:tbl[1]/w:tr[1]/w:tc[1]/w:p[1]/w:r/mc:AlternateContent[1]/mc:Fallback/w:pict/v:shape", "allowincell", "f");

    // DML layoutInCell
    assertXPath(pXmlDocument, "/w:document/w:body/w:tbl[1]/w:tr[1]/w:tc[1]/w:p[1]/w:r/mc:AlternateContent[1]/mc:Choice/w:drawing/wp:anchor", "layoutInCell", "0");
}

CPPUNIT_TEST_FIXTURE(Test, testRelativeAnchorHeightFromBottomMarginHasFooter)
{
    loadAndSave("tdf133070_testRelativeAnchorHeightFromBottomMarginHasFooter.docx");
    // tdf#133070 The height was set relative to page print area bottom,
    // but this was handled relative to page height.
    // Note: page print area bottom = margin + footer height.
    // In this case the footer exists.
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    assertXPath(pXmlDoc, "//anchored/SwAnchoredDrawObject/bounds", "height", "1147");
}

DECLARE_OOXMLEXPORT_TEST(TestTdf132483, "tdf132483.docx")
{
    uno::Reference<beans::XPropertySet> xOLEProps(getShape(1), uno::UNO_QUERY_THROW);
    sal_Int16 nVRelPos = -1;
    sal_Int16 nHRelPos = -1;
    xOLEProps->getPropertyValue("VertOrientRelation") >>= nVRelPos;
    xOLEProps->getPropertyValue("HoriOrientRelation") >>= nHRelPos;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("The OLE is shifted vertically",
        text::RelOrientation::PAGE_FRAME , nVRelPos);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("The OLE is shifted horizontally",
        text::RelOrientation::PAGE_FRAME , nHRelPos);
}

CPPUNIT_TEST_FIXTURE(Test, TestTdf143028)
{
    loadAndSave("fail_bracePair.odt");
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    auto pExportXml = parseExport();

    CPPUNIT_ASSERT_EQUAL(1, getXPathNode(
        pExportXml, "/w:document/w:body/w:p/w:r/mc:AlternateContent/mc:Choice/w:drawing/wp:anchor/"
                    "a:graphic/a:graphicData/wps:wsp/wps:spPr/a:xfrm")->nodesetval->nodeNr);

}

CPPUNIT_TEST_FIXTURE(Test, testRelativeAnchorHeightFromBottomMarginNoFooter)
{
    loadAndSave("tdf133070_testRelativeAnchorHeightFromBottomMarginNoFooter.docx");
    // tdf#133070 The height was set relative to page print area bottom,
    // but this was handled relative to page height.
    // Note: page print area bottom = margin + footer height.
    // In this case the footer does not exist, so OpenDocument and OOXML margins are the same.
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    assertXPath(pXmlDoc, "//anchored/SwAnchoredDrawObject/bounds", "height", "1147");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf133702)
{
    loadAndSave("tdf133702.docx");
    xmlDocUniquePtr pXmlDocument = parseExport("word/document.xml");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[1]/w:pPr/w:framePr");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf135667)
{
    loadAndSave("tdf135667.odt");
    xmlDocUniquePtr pXmlDocument = parseExport("word/document.xml");

    // This was missing.
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:r/w:object/v:shapetype");

    // line settings
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:r/w:object/v:shape", "stroked", "t");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:r/w:object/v:shape", "strokecolor", "#FF0000");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:r/w:object/v:shape", "strokeweight", "4pt");

    // line type
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:r/w:object/v:shape/v:stroke", "linestyle", "Single");
    assertXPath(pXmlDocument, "/w:document/w:body/w:p[2]/w:r/w:object/v:shape/v:stroke", "dashstyle", "Dash");
}

CPPUNIT_TEST_FIXTURE(Test, testImageSpaceSettings)
{
    loadAndSave("tdf135047_ImageSpaceSettings.fodt");
    // tdf#135047 The spaces of image were not saved.
    xmlDocUniquePtr pXmlDoc = parseExport();
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[1]/w:r[1]/w:drawing/wp:anchor", "distT", "90170");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[1]/w:r[1]/w:drawing/wp:anchor", "distB", "90170");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[1]/w:r[1]/w:drawing/wp:anchor", "distL", "90170");
    assertXPath(pXmlDoc, "/w:document/w:body/w:p[1]/w:r[1]/w:drawing/wp:anchor", "distR", "90170");
}

DECLARE_OOXMLEXPORT_TEST(testTdf137295, "tdf137295.doc")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());

    // Without the fix in place, the test would have failed with
    // - Expected: 2
    // - Actual  : 1
    CPPUNIT_ASSERT_EQUAL(2, getShapes());
}

DECLARE_OOXMLEXPORT_TEST(testTdf135660, "tdf135660.docx")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    const uno::Reference<drawing::XShape> xShape = getShape(1);
    const uno::Reference<beans::XPropertySet> xOLEProps(xShape, uno::UNO_QUERY_THROW);
    sal_Int32 nWrapDistanceLeft = -1;
    sal_Int32 nWrapDistanceRight = -1;
    sal_Int32 nWrapDistanceTop = -1;
    sal_Int32 nWrapDistanceBottom = -1;
    xOLEProps->getPropertyValue("LeftMargin") >>= nWrapDistanceLeft;
    xOLEProps->getPropertyValue("RightMargin") >>= nWrapDistanceRight;
    xOLEProps->getPropertyValue("TopMargin") >>= nWrapDistanceTop;
    xOLEProps->getPropertyValue("BottomMargin") >>= nWrapDistanceBottom;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Left wrap distance is wrong", static_cast<sal_Int32>(0), nWrapDistanceLeft);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Right wrap distance is wrong", static_cast<sal_Int32>(400), nWrapDistanceRight);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Top wrap distance is wrong", static_cast<sal_Int32>(300), nWrapDistanceTop);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Bottom wrap distance is wrong", static_cast<sal_Int32>(199), nWrapDistanceBottom);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf136814)
{
    loadAndSave("tdf136814.odt");
    xmlDocUniquePtr pXmlDocument = parseExport("word/document.xml");

    // Padding in this document is 0.10 cm which should translate to 3 pt (approx. 1.0583mm)
    assertXPath(pXmlDocument, "/w:document/w:body/w:sectPr/w:pgBorders/w:top", "space", "3");
    assertXPath(pXmlDocument, "/w:document/w:body/w:sectPr/w:pgBorders/w:left", "space", "3");
    assertXPath(pXmlDocument, "/w:document/w:body/w:sectPr/w:pgBorders/w:bottom", "space", "3");
    assertXPath(pXmlDocument, "/w:document/w:body/w:sectPr/w:pgBorders/w:right", "space", "3");
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
