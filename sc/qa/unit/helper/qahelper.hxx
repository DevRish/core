/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <docsh.hxx>
#include <address.hxx>

#include <cppunit/SourceLine.h>

#include <test/bootstrapfixture.hxx>
#include <comphelper/documentconstants.hxx>

#include <comphelper/fileformat.h>
#include <formula/grammar.hxx>
#include "scqahelperdllapi.h"

#include <string>
#include <string_view>
#include <sstream>
#include <undoblk.hxx>

#include <sal/types.h>

#include <memory>

namespace utl { class TempFile; }

#define ODS_FORMAT_TYPE      (SfxFilterFlags::IMPORT | SfxFilterFlags::EXPORT | SfxFilterFlags::TEMPLATE | SfxFilterFlags::OWN | SfxFilterFlags::DEFAULT | SfxFilterFlags::ENCRYPTION | SfxFilterFlags::PASSWORDTOMODIFY)
#define XLS_FORMAT_TYPE      (SfxFilterFlags::IMPORT | SfxFilterFlags::EXPORT | SfxFilterFlags::ALIEN | SfxFilterFlags::ENCRYPTION | SfxFilterFlags::PASSWORDTOMODIFY | SfxFilterFlags::PREFERED)
#define XLSX_FORMAT_TYPE     (SfxFilterFlags::IMPORT | SfxFilterFlags::EXPORT | SfxFilterFlags::ALIEN | SfxFilterFlags::STARONEFILTER | SfxFilterFlags::PREFERED)
#define LOTUS123_FORMAT_TYPE (SfxFilterFlags::IMPORT |                          SfxFilterFlags::ALIEN | SfxFilterFlags::PREFERED)
#define CSV_FORMAT_TYPE      (SfxFilterFlags::IMPORT | SfxFilterFlags::EXPORT | SfxFilterFlags::ALIEN )
#define HTML_FORMAT_TYPE     (SfxFilterFlags::IMPORT | SfxFilterFlags::EXPORT | SfxFilterFlags::ALIEN )
#define DIF_FORMAT_TYPE      (SfxFilterFlags::IMPORT | SfxFilterFlags::EXPORT | SfxFilterFlags::ALIEN )
#define XLS_XML_FORMAT_TYPE  (SfxFilterFlags::IMPORT | SfxFilterFlags::ALIEN | SfxFilterFlags::PREFERED)
#define XLSB_XML_FORMAT_TYPE (SfxFilterFlags::IMPORT |                          SfxFilterFlags::ALIEN | SfxFilterFlags::STARONEFILTER | SfxFilterFlags::PREFERED)
#define FODS_FORMAT_TYPE     (SfxFilterFlags::IMPORT | SfxFilterFlags::EXPORT | SfxFilterFlags::OWN | SfxFilterFlags::STARONEFILTER )
#define GNUMERIC_FORMAT_TYPE (SfxFilterFlags::IMPORT | SfxFilterFlags::ALIEN | SfxFilterFlags::PREFERED )
#define XLTX_FORMAT_TYPE     (SfxFilterFlags::IMPORT | SfxFilterFlags::EXPORT | SfxFilterFlags::TEMPLATE |SfxFilterFlags::ALIEN | SfxFilterFlags::STARONEFILTER | SfxFilterFlags::PREFERED)

#define FORMAT_ODS      0
#define FORMAT_XLS      1
#define FORMAT_XLSX     2
#define FORMAT_XLSM     3
#define FORMAT_CSV      4
#define FORMAT_HTML     5
#define FORMAT_LOTUS123 6
#define FORMAT_DIF      7
#define FORMAT_XLS_XML  8
#define FORMAT_XLSB     9
#define FORMAT_FODS     10
#define FORMAT_GNUMERIC 11
#define FORMAT_XLTX     12

enum class StringType { PureString, StringValue };

SCQAHELPER_DLLPUBLIC bool testEqualsWithTolerance( tools::Long nVal1, tools::Long nVal2, tools::Long nTol );

#define CHECK_OPTIMAL 0x1

class SdrOle2Obj;
class ScRangeList;
class ScTokenArray;


// data format for row height tests
struct TestParam
{
    struct RowData
    {
        SCROW nStartRow;
        SCROW nEndRow;
        SCTAB nTab;
        int nExpectedHeight; // -1 for default height
        int nCheck; // currently only CHECK_OPTIMAL ( we could add CHECK_MANUAL etc.)
        bool bOptimal;
    };
    const char* sTestDoc;
    int nImportType;
    int nExportType; // -1 for import test, otherwise this is an export test
    int nRowData;
    RowData const * pData;
};

struct RangeNameDef
{
    const char* mpName;
    const char* mpExpr;
    sal_uInt16 mnIndex;
};

struct FileFormat {
    const char* pName; const char* pFilterName; const char* pTypeName; SfxFilterFlags nFormatType;
};

// Printers for the calc data structures. Needed for the EQUAL assertion
// macros from CPPUNIT.

SCQAHELPER_DLLPUBLIC std::ostream& operator<<(std::ostream& rStrm, const ScAddress& rAddr);

SCQAHELPER_DLLPUBLIC std::ostream& operator<<(std::ostream& rStrm, const ScRange& rRange);

SCQAHELPER_DLLPUBLIC std::ostream& operator<<(std::ostream& rStrm, const ScRangeList& rList);

SCQAHELPER_DLLPUBLIC std::ostream& operator<<(std::ostream& rStrm, const Color& rColor);

SCQAHELPER_DLLPUBLIC std::ostream& operator<<(std::ostream& rStrm, const OpCode& rCode);

// Why is this here and not in osl, and using the already existing file
// handling APIs? Do we really want to add arbitrary new file handling
// wrappers here and there (and then having to handle the Android (and
// eventually perhaps iOS) special cases here, too)?  Please move this to osl,
// it sure looks generally useful. Or am I missing something?

void loadFile(const OUString& aFileName, std::string& aContent);

SCQAHELPER_DLLPUBLIC void testFile(const OUString& aFileName, ScDocument& rDoc, SCTAB nTab, StringType aStringFormat = StringType::StringValue);

//need own handler because conditional formatting strings must be generated
SCQAHELPER_DLLPUBLIC void testCondFile(const OUString& aFileName, ScDocument* pDoc, SCTAB nTab);

SCQAHELPER_DLLPUBLIC const SdrOle2Obj* getSingleOleObject(ScDocument& rDoc, sal_uInt16 nPage);

SCQAHELPER_DLLPUBLIC const SdrOle2Obj* getSingleChartObject(ScDocument& rDoc, sal_uInt16 nPage);

SCQAHELPER_DLLPUBLIC ScRangeList getChartRanges(ScDocument& rDoc, const SdrOle2Obj& rChartObj);

bool checkFormula(ScDocument& rDoc, const ScAddress& rPos, const char* pExpected);

bool checkFormulaPosition(ScDocument& rDoc, const ScAddress& rPos);
bool checkFormulaPositions(
    ScDocument& rDoc, SCTAB nTab, SCCOL nCol, const SCROW* pRows, size_t nRowCount);

std::unique_ptr<ScTokenArray> compileFormula(
    ScDocument* pDoc, const OUString& rFormula,
    formula::FormulaGrammar::Grammar eGram = formula::FormulaGrammar::GRAM_NATIVE );

SCQAHELPER_DLLPUBLIC bool checkOutput(
    const ScDocument* pDoc, const ScRange& aOutRange,
    const std::vector<std::vector<const char*>>& aCheck, const char* pCaption );

void clearFormulaCellChangedFlag( ScDocument& rDoc, const ScRange& rRange );

/**
 * Check if the cell at specified position is a formula cell that doesn't
 * have an error value.
 */
SCQAHELPER_DLLPUBLIC bool isFormulaWithoutError(ScDocument& rDoc, const ScAddress& rPos);

/**
 * Convert formula token array to a formula string.
 */
SCQAHELPER_DLLPUBLIC OUString toString(
    ScDocument& rDoc, const ScAddress& rPos, ScTokenArray& rArray,
    formula::FormulaGrammar::Grammar eGram);

inline std::string print(const ScAddress& rAddr)
{
    std::ostringstream str;
    str << "Col: " << rAddr.Col();
    str << " Row: " << rAddr.Row();
    str << " Tab: " << rAddr.Tab();
    return str.str();
}

/**
 * Temporarily set formula grammar.
 */
class FormulaGrammarSwitch
{
    ScDocument* mpDoc;
    formula::FormulaGrammar::Grammar meOldGrammar;

public:
    FormulaGrammarSwitch(ScDocument* pDoc, formula::FormulaGrammar::Grammar eGrammar);
    ~FormulaGrammarSwitch();
};

class SCQAHELPER_DLLPUBLIC ScBootstrapFixture : public test::BootstrapFixture
{
    static const FileFormat aFileFormats[];
protected:
    OUString m_aBaseString;

    ScDocShellRef load(
        bool bReadWrite, const OUString& rURL, const OUString& rFilter, const OUString &rUserData,
        const OUString& rTypeName, SfxFilterFlags nFilterFlags, SotClipboardFormatId nClipboardID,
        sal_uIntPtr nFilterVersion = SOFFICE_FILEFORMAT_CURRENT, const OUString* pPassword = nullptr );

    ScDocShellRef load(
        const OUString& rURL, const OUString& rFilter, const OUString &rUserData,
        const OUString& rTypeName, SfxFilterFlags nFilterFlags, SotClipboardFormatId nClipboardID,
        sal_uIntPtr nFilterVersion = SOFFICE_FILEFORMAT_CURRENT, const OUString* pPassword = nullptr );

    ScDocShellRef load(const OUString& rURL, sal_Int32 nFormat, bool bReadWrite = false);

    ScDocShellRef loadDoc(
        std::u16string_view rFileName, sal_Int32 nFormat, bool bReadWrite = false );

public:
    static const FileFormat* getFileFormats() { return aFileFormats; }

    explicit ScBootstrapFixture( const OUString& rsBaseString );
    virtual ~ScBootstrapFixture() override;

    void createFileURL(std::u16string_view aFileBase, std::u16string_view aFileExtension, OUString& rFilePath);
    void createCSVPath(const char* aFileBase, OUString& rCSVPath);
    void createCSVPath(std::u16string_view aFileBase, OUString& rCSVPath);

    ScDocShellRef saveAndReload(ScDocShell& rShell, const OUString &rFilter,
    const OUString &rUserData, const OUString& rTypeName, SfxFilterFlags nFormatType,
    std::shared_ptr<utl::TempFile>* pTempFile = nullptr, const OUString* pPassword = nullptr, bool bClose = true );

    ScDocShellRef saveAndReload( ScDocShell& rShell, sal_Int32 nFormat, std::shared_ptr<utl::TempFile>* pTempFile = nullptr );
    ScDocShellRef saveAndReloadPassword( ScDocShell& rShell, sal_Int32 nFormat, std::shared_ptr<utl::TempFile>* pTempFile = nullptr );
    ScDocShellRef saveAndReloadNoClose( ScDocShell& rShell, sal_Int32 nFormat, std::shared_ptr<utl::TempFile>* pTempFile = nullptr );

    std::shared_ptr<utl::TempFile> saveAs(ScDocShell& rShell, sal_Int32 nFormat);
    std::shared_ptr<utl::TempFile> exportTo(ScDocShell& rShell, sal_Int32 nFormat);

    void miscRowHeightsTest( TestParam const * aTestValues, unsigned int numElems );
};

#define ASSERT_DOUBLES_EQUAL( expected, result )    \
    CPPUNIT_ASSERT_DOUBLES_EQUAL( (expected), (result), 1e-14 )

#define ASSERT_DOUBLES_EQUAL_MESSAGE( message, expected, result )   \
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE( (message), (expected), (result), 1e-14 )

SCQAHELPER_DLLPUBLIC void checkFormula(ScDocument& rDoc, const ScAddress& rPos,
        const char* expected, const char* msg, CppUnit::SourceLine const & sourceLine);

#define ASSERT_FORMULA_EQUAL(doc, pos, expected, msg) \
    checkFormula(doc, pos, expected, msg, CPPUNIT_SOURCELINE())

SCQAHELPER_DLLPUBLIC void testFormats(ScBootstrapFixture* pTest, ScDocument* pDoc, sal_Int32 nFormat);

SCQAHELPER_DLLPUBLIC ScTokenArray* getTokens(ScDocument& rDoc, const ScAddress& rPos);

SCQAHELPER_DLLPUBLIC std::string to_std_string(const OUString& rStr);

SCQAHELPER_DLLPUBLIC ScUndoCut* cutToClip(ScDocShell& rDocSh, const ScRange& rRange, ScDocument* pClipDoc,
                                    bool bCreateUndo);
SCQAHELPER_DLLPUBLIC void copyToClip(ScDocument* pSrcDoc, const ScRange& rRange, ScDocument* pClipDoc);
SCQAHELPER_DLLPUBLIC void pasteFromClip(ScDocument* pDestDoc, const ScRange& rDestRange,
                                    ScDocument* pClipDoc);
SCQAHELPER_DLLPUBLIC ScUndoPaste* createUndoPaste(ScDocShell& rDocSh, const ScRange& rRange,
                                    ScDocumentUniquePtr pUndoDoc);
SCQAHELPER_DLLPUBLIC void pasteOneCellFromClip(ScDocument* pDestDoc, const ScRange& rDestRange,
                                     ScDocument* pClipDoc,
                                     InsertDeleteFlags eFlags = InsertDeleteFlags::ALL);
SCQAHELPER_DLLPUBLIC void setCalcAsShown(ScDocument* pDoc, bool bCalcAsShown);
SCQAHELPER_DLLPUBLIC ScDocShell* findLoadedDocShellByName(std::u16string_view rName);
SCQAHELPER_DLLPUBLIC ScRange insertRangeData(ScDocument* pDoc, const ScAddress& rPos,
                                   const std::vector<std::vector<const char*>>& rData);
SCQAHELPER_DLLPUBLIC bool insertRangeNames(ScDocument* pDoc, ScRangeName* pNames, const RangeNameDef* p,
                                   const RangeNameDef* pEnd);
SCQAHELPER_DLLPUBLIC OUString getRangeByName(ScDocument* pDoc, const OUString& aRangeName);
SCQAHELPER_DLLPUBLIC OUString getFormula(ScDocument* pDoc, SCCOL nCol, SCROW nRow, SCTAB nTab);
SCQAHELPER_DLLPUBLIC void printFormula(ScDocument* pDoc, SCCOL nCol, SCROW nRow, SCTAB nTab,
                                       const char* pCaption = nullptr);
SCQAHELPER_DLLPUBLIC void printRange(ScDocument* pDoc, const ScRange& rRange, const char* pCaption,
                                     const bool printFormula = false);
SCQAHELPER_DLLPUBLIC void printRange(ScDocument* pDoc, const ScRange& rRange,
                                     const OString& rCaption, const bool printFormula = false);
SCQAHELPER_DLLPUBLIC void clearRange(ScDocument* pDoc, const ScRange& rRange);
SCQAHELPER_DLLPUBLIC void clearSheet(ScDocument* pDoc, SCTAB nTab);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
