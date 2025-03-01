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

#include <sfx2/objsh.hxx>
#include <rtl/ustring.hxx>

#include <document.hxx>
#include <scextopt.hxx>
#include <docoptio.hxx>
#include <tabprotection.hxx>
#include <postit.hxx>
#include <root.hxx>

#include <excdoc.hxx>
#include <xeextlst.hxx>
#include <biffhelper.hxx>

#include <xcl97rec.hxx>
#include <xetable.hxx>
#include <xelink.hxx>
#include <xepage.hxx>
#include <xeview.hxx>
#include <xecontent.hxx>
#include <xeescher.hxx>
#include <xepivot.hxx>
#include <XclExpChangeTrack.hxx>
#include <xepivotxml.hxx>
#include <xedbdata.hxx>
#include <xlcontent.hxx>
#include <xlname.hxx>
#include <xllink.hxx>
#include <xltools.hxx>

#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <o3tl/safeint.hxx>
#include <oox/token/tokens.hxx>
#include <oox/token/namespaces.hxx>
#include <memory>

using namespace oox;

static OUString lcl_GetVbaTabName( SCTAB n )
{
    OUString aRet = "__VBA__"  + OUString::number( static_cast<sal_uInt16>(n) );
    return aRet;
}

static void lcl_AddBookviews( XclExpRecordList<>& aRecList, const ExcTable& self )
{
    aRecList.AppendNewRecord( new XclExpXmlStartElementRecord( XML_bookViews ) );
    aRecList.AppendNewRecord( new XclExpWindow1( self.GetRoot() ) );
    aRecList.AppendNewRecord( new XclExpXmlEndElementRecord( XML_bookViews ) );
}

static void lcl_AddCalcPr( XclExpRecordList<>& aRecList, const ExcTable& self )
{
    ScDocument& rDoc = self.GetDoc();

    aRecList.AppendNewRecord( new XclExpXmlStartSingleElementRecord( XML_calcPr ) );
    // OOXTODO: calcCompleted, calcId, calcMode, calcOnSave,
    //          concurrentCalc, concurrentManualCount,
    //          forceFullCalc, fullCalcOnLoad, fullPrecision
    aRecList.AppendNewRecord( new XclCalccount( rDoc ) );
    aRecList.AppendNewRecord( new XclRefmode( rDoc ) );
    aRecList.AppendNewRecord( new XclIteration( rDoc ) );
    aRecList.AppendNewRecord( new XclDelta( rDoc ) );
    aRecList.AppendNewRecord( new XclExpBoolRecord(oox::xls::BIFF_ID_SAVERECALC, true) );
    aRecList.AppendNewRecord( new XclExpXmlEndSingleElementRecord() );  // XML_calcPr
}

static void lcl_AddWorkbookProtection( XclExpRecordList<>& aRecList, const ExcTable& self )
{
    aRecList.AppendNewRecord( new XclExpXmlStartSingleElementRecord( XML_workbookProtection ) );

    const ScDocProtection* pProtect = self.GetDoc().GetDocProtection();
    if (pProtect && pProtect->isProtected())
    {
        aRecList.AppendNewRecord( new XclExpWindowProtection(pProtect->isOptionEnabled(ScDocProtection::WINDOWS)) );
        aRecList.AppendNewRecord( new XclExpProtection(pProtect->isOptionEnabled(ScDocProtection::STRUCTURE)) );
        aRecList.AppendNewRecord( new XclExpPassHash(pProtect->getPasswordHash(PASSHASH_XL)) );
    }

    aRecList.AppendNewRecord( new XclExpXmlEndSingleElementRecord() );   // XML_workbookProtection
}

static void lcl_AddScenariosAndFilters( XclExpRecordList<>& aRecList, const XclExpRoot& rRoot, SCTAB nScTab )
{
    // Scenarios
    aRecList.AppendNewRecord( new ExcEScenarioManager( rRoot, nScTab ) );
    // filter
    aRecList.AppendRecord( rRoot.GetFilterManager().CreateRecord( nScTab ) );
}

ExcTable::ExcTable( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot ),
    mnScTab( SCTAB_GLOBAL ),
    nExcTab( EXC_NOTAB ),
    mxNoteList( new XclExpNoteList )
{
}

ExcTable::ExcTable( const XclExpRoot& rRoot, SCTAB nScTab ) :
    XclExpRoot( rRoot ),
    mnScTab( nScTab ),
    nExcTab( rRoot.GetTabInfo().GetXclTab( nScTab ) ),
    mxNoteList( new XclExpNoteList )
{
}

ExcTable::~ExcTable()
{
}

void ExcTable::Add( XclExpRecordBase* pRec )
{
    OSL_ENSURE( pRec, "-ExcTable::Add(): pRec is NULL!" );
    aRecList.AppendNewRecord( pRec );
}

void ExcTable::FillAsHeaderBinary( ExcBoundsheetList& rBoundsheetList )
{
    InitializeGlobals();

    RootData& rR = GetOldRoot();
    ScDocument& rDoc = GetDoc();
    XclExpTabInfo& rTabInfo = GetTabInfo();

    if ( GetBiff() <= EXC_BIFF5 )
        Add( new ExcBofW );
    else
        Add( new ExcBofW8 );

    sal_uInt16  nExcTabCount    = rTabInfo.GetXclTabCount();
    sal_uInt16  nCodenames      = static_cast< sal_uInt16 >( GetExtDocOptions().GetCodeNameCount() );

    SfxObjectShell* pShell = GetDocShell();
    sal_uInt16 nWriteProtHash = pShell ? pShell->GetModifyPasswordHash() : 0;
    bool bRecommendReadOnly = pShell && pShell->IsLoadReadonly();

    if( (nWriteProtHash > 0) || bRecommendReadOnly )
        Add( new XclExpEmptyRecord( EXC_ID_WRITEPROT ) );

    // TODO: correct codepage for BIFF5?
    sal_uInt16 nCodePage = XclTools::GetXclCodePage( (GetBiff() <= EXC_BIFF5) ? RTL_TEXTENCODING_MS_1252 : RTL_TEXTENCODING_UNICODE );

    if( GetBiff() <= EXC_BIFF5 )
    {
        Add( new XclExpEmptyRecord( EXC_ID_INTERFACEHDR ) );
        Add( new XclExpUInt16Record( EXC_ID_MMS, 0 ) );
        Add( new XclExpEmptyRecord( EXC_ID_TOOLBARHDR ) );
        Add( new XclExpEmptyRecord( EXC_ID_TOOLBAREND ) );
        Add( new XclExpEmptyRecord( EXC_ID_INTERFACEEND ) );
        Add( new ExcDummy_00 );
    }
    else
    {
        if( IsDocumentEncrypted() )
            Add( new XclExpFileEncryption( GetRoot() ) );
        Add( new XclExpInterfaceHdr( nCodePage ) );
        Add( new XclExpUInt16Record( EXC_ID_MMS, 0 ) );
        Add( new XclExpInterfaceEnd );
        Add( new XclExpWriteAccess );
    }

    Add( new XclExpFileSharing( GetRoot(), nWriteProtHash, bRecommendReadOnly ) );
    Add( new XclExpUInt16Record( EXC_ID_CODEPAGE, nCodePage ) );

    if( GetBiff() == EXC_BIFF8 )
    {
        Add( new XclExpBoolRecord( EXC_ID_DSF, false ) );
        Add( new XclExpEmptyRecord( EXC_ID_XL9FILE ) );
        rR.pTabId = new XclExpChTrTabId( std::max( nExcTabCount, nCodenames ) );
        Add( rR.pTabId );
        if( HasVbaStorage() )
        {
            Add( new XclObproj );
            const OUString& rCodeName = GetExtDocOptions().GetDocSettings().maGlobCodeName;
            if( !rCodeName.isEmpty() )
                Add( new XclCodename( rCodeName ) );
        }
    }

    Add( new XclExpUInt16Record( EXC_ID_FNGROUPCOUNT, 14 ) );

    if ( GetBiff() <= EXC_BIFF5 )
    {
        // global link table: EXTERNCOUNT, EXTERNSHEET, NAME
        aRecList.AppendRecord( CreateRecord( EXC_ID_EXTERNSHEET ) );
        aRecList.AppendRecord( CreateRecord( EXC_ID_NAME ) );
    }

    // document protection options
    lcl_AddWorkbookProtection( aRecList, *this );

    if( GetBiff() == EXC_BIFF8 )
    {
        Add( new XclExpProt4Rev );
        Add( new XclExpProt4RevPass );
    }

    lcl_AddBookviews( aRecList, *this );

    Add( new XclExpXmlStartSingleElementRecord( XML_workbookPr ) );
    if ( GetBiff() == EXC_BIFF8 && GetOutput() != EXC_OUTPUT_BINARY )
    {
        Add( new XclExpBoolRecord(0x0040, false, XML_backupFile ) );    // BACKUP
        Add( new XclExpBoolRecord(0x008D, false, XML_showObjects ) );   // HIDEOBJ
    }

    if ( GetBiff() == EXC_BIFF8 )
    {
        Add( new XclExpBoolRecord(0x0040, false) ); // BACKUP
        Add( new XclExpBoolRecord(0x008D, false) ); // HIDEOBJ
    }

    if( GetBiff() <= EXC_BIFF5 )
    {
        Add( new ExcDummy_040 );
        Add( new Exc1904( rDoc ) );
        Add( new ExcDummy_041 );
    }
    else
    {
        // BIFF8
        Add( new Exc1904( rDoc ) );
        Add( new XclExpBoolRecord( 0x000E, !rDoc.GetDocOptions().IsCalcAsShown() ) );
        Add( new XclExpBoolRecord(0x01B7, false) ); // REFRESHALL
        Add( new XclExpBoolRecord(0x00DA, false) ); // BOOKBOOL
    }

    // Formatting: FONT, FORMAT, XF, STYLE, PALETTE
    aRecList.AppendRecord( CreateRecord( EXC_ID_FONTLIST ) );
    aRecList.AppendRecord( CreateRecord( EXC_ID_FORMATLIST ) );
    aRecList.AppendRecord( CreateRecord( EXC_ID_XFLIST ) );
    aRecList.AppendRecord( CreateRecord( EXC_ID_PALETTE ) );

    SCTAB   nC;
    SCTAB  nScTabCount     = rTabInfo.GetScTabCount();
    if( GetBiff() <= EXC_BIFF5 )
    {
        // Bundlesheet
        for( nC = 0 ; nC < nScTabCount ; nC++ )
            if( rTabInfo.IsExportTab( nC ) )
            {
                ExcBoundsheetList::RecordRefType xBoundsheet = new ExcBundlesheet( rR, nC );
                aRecList.AppendRecord( xBoundsheet );
                rBoundsheetList.AppendRecord( xBoundsheet );
            }
    }
    else
    {
        // Pivot Cache
        GetPivotTableManager().CreatePivotTables();
        aRecList.AppendRecord( GetPivotTableManager().CreatePivotCachesRecord() );

        // Change tracking
        if( rDoc.GetChangeTrack() )
        {
            rR.pUserBViewList = new XclExpUserBViewList( *rDoc.GetChangeTrack() );
            Add( rR.pUserBViewList );
        }

        // Natural Language Formulas Flag
        aRecList.AppendNewRecord( new XclExpBoolRecord( EXC_ID_USESELFS, GetDoc().GetDocOptions().IsLookUpColRowNames() ) );

        // Bundlesheet
        for( nC = 0 ; nC < nScTabCount ; nC++ )
            if( rTabInfo.IsExportTab( nC ) )
            {
                ExcBoundsheetList::RecordRefType xBoundsheet = new ExcBundlesheet8( rR, nC );
                aRecList.AppendRecord( xBoundsheet );
                rBoundsheetList.AppendRecord( xBoundsheet );
            }

        OUString aTmpString;
        for( SCTAB nAdd = 0; nC < static_cast<SCTAB>(nCodenames) ; nC++, nAdd++ )
        {
            aTmpString = lcl_GetVbaTabName( nAdd );
            ExcBoundsheetList::RecordRefType xBoundsheet = new ExcBundlesheet8( aTmpString );
            aRecList.AppendRecord( xBoundsheet );
            rBoundsheetList.AppendRecord( xBoundsheet );
        }

        // COUNTRY - in BIFF8 in workbook globals
        Add( new XclExpCountry( GetRoot() ) );

        // link table: SUPBOOK, XCT, CRN, EXTERNNAME, EXTERNSHEET, NAME
        aRecList.AppendRecord( CreateRecord( EXC_ID_EXTERNSHEET ) );
        aRecList.AppendRecord( CreateRecord( EXC_ID_NAME ) );

        Add( new XclExpRecalcId );

        // MSODRAWINGGROUP per-document data
        aRecList.AppendRecord( GetObjectManager().CreateDrawingGroup() );
        // Shared string table: SST, EXTSST
        aRecList.AppendRecord( CreateRecord( EXC_ID_SST ) );

        Add( new XclExpBookExt );
    }

    Add( new ExcEof );
}

void ExcTable::FillAsHeaderXml( ExcBoundsheetList& rBoundsheetList )
{
    InitializeGlobals();

    RootData& rR = GetOldRoot();
    ScDocument& rDoc = GetDoc();
    XclExpTabInfo& rTabInfo = GetTabInfo();

    sal_uInt16  nExcTabCount    = rTabInfo.GetXclTabCount();
    sal_uInt16  nCodenames      = static_cast< sal_uInt16 >( GetExtDocOptions().GetCodeNameCount() );

    rR.pTabId = new XclExpChTrTabId( std::max( nExcTabCount, nCodenames ) );
    Add( rR.pTabId );

    Add( new XclExpXmlStartSingleElementRecord( XML_workbookPr ) );
    Add( new XclExpBoolRecord(0x0040, false, XML_backupFile ) );    // BACKUP
    Add( new XclExpBoolRecord(0x008D, false, XML_showObjects ) );   // HIDEOBJ

    Add( new Exc1904( rDoc ) );
    // OOXTODO: The following /workbook/workbookPr attributes are mapped
    //          to various BIFF records that are not currently supported:
    //
    //          XML_allowRefreshQuery:          QSISTAG 802h: fEnableRefresh
    //          XML_autoCompressPictures:       COMPRESSPICTURES 89Bh: fAutoCompressPictures
    //          XML_checkCompatibility:         COMPAT12 88Ch: fNoCompatChk
    //          XML_codeName:                   "Calc"
    //          XML_defaultThemeVersion:        ???
    //          XML_filterPrivacy:              BOOKEXT 863h: fFilterPrivacy
    //          XML_hidePivotFieldList:         BOOKBOOL DAh: fHidePivotTableFList
    //          XML_promptedSolutions:          BOOKEXT 863h: fBuggedUserAboutSolution
    //          XML_publishItems:               NAMEPUBLISH 893h: fPublished
    //          XML_saveExternalLinkValues:     BOOKBOOL DAh: fNoSavSupp
    //          XML_showBorderUnselectedTables: BOOKBOOL DAh: fHideBorderUnsels
    //          XML_showInkAnnotation:          BOOKEXT 863h: fShowInkAnnotation
    //          XML_showPivotChart:             PIVOTCHARTBITS 859h: fGXHide??
    //          XML_updateLinks:                BOOKBOOL DAh: grbitUpdateLinks
    Add( new XclExpXmlEndSingleElementRecord() );   // XML_workbookPr

    // Formatting: FONT, FORMAT, XF, STYLE, PALETTE
    aRecList.AppendNewRecord( new XclExpXmlStyleSheet( *this ) );

    // Change tracking
    if( rDoc.GetChangeTrack() )
    {
        rR.pUserBViewList = new XclExpUserBViewList( *rDoc.GetChangeTrack() );
        Add( rR.pUserBViewList );
    }

    lcl_AddWorkbookProtection( aRecList, *this );
    lcl_AddBookviews( aRecList, *this );

    // Bundlesheet
    SCTAB nC;
    SCTAB nScTabCount = rTabInfo.GetScTabCount();
    aRecList.AppendNewRecord( new XclExpXmlStartElementRecord( XML_sheets ) );
    for( nC = 0 ; nC < nScTabCount ; nC++ )
        if( rTabInfo.IsExportTab( nC ) )
        {
            ExcBoundsheetList::RecordRefType xBoundsheet = new ExcBundlesheet8( rR, nC );
            aRecList.AppendRecord( xBoundsheet );
            rBoundsheetList.AppendRecord( xBoundsheet );
        }
    aRecList.AppendNewRecord( new XclExpXmlEndElementRecord( XML_sheets ) );

    OUString aTmpString;
    for( SCTAB nAdd = 0; nC < static_cast<SCTAB>(nCodenames) ; nC++, nAdd++ )
    {
        aTmpString = lcl_GetVbaTabName( nAdd );
        ExcBoundsheetList::RecordRefType xBoundsheet = new ExcBundlesheet8( aTmpString );
        aRecList.AppendRecord( xBoundsheet );
        rBoundsheetList.AppendRecord( xBoundsheet );
    }

    // link table: SUPBOOK, XCT, CRN, EXTERNNAME, EXTERNSHEET, NAME
    aRecList.AppendRecord( CreateRecord( EXC_ID_EXTERNSHEET ) );
    aRecList.AppendRecord( CreateRecord( EXC_ID_NAME ) );

    lcl_AddCalcPr( aRecList, *this );

    // MSODRAWINGGROUP per-document data
    aRecList.AppendRecord( GetObjectManager().CreateDrawingGroup() );
    // Shared string table: SST, EXTSST
    aRecList.AppendRecord( CreateRecord( EXC_ID_SST ) );
}

void ExcTable::FillAsTableBinary( SCTAB nCodeNameIdx )
{
    InitializeTable( mnScTab );

    RootData& rR = GetOldRoot();
    XclBiff eBiff = GetBiff();
    ScDocument& rDoc = GetDoc();

    OSL_ENSURE( (mnScTab >= 0) && (mnScTab <= MAXTAB), "-ExcTable::Table(): mnScTab - no ordinary table!" );
    OSL_ENSURE( nExcTab <= o3tl::make_unsigned(MAXTAB), "-ExcTable::Table(): nExcTab - no ordinary table!" );

    // create a new OBJ list for this sheet (may be used by notes, autofilter, data validation)
    if( eBiff == EXC_BIFF8 )
        GetObjectManager().StartSheet();

    // cell table: DEFROWHEIGHT, DEFCOLWIDTH, COLINFO, DIMENSIONS, ROW, cell records
    mxCellTable = new XclExpCellTable( GetRoot() );

    //export cell notes
    std::vector<sc::NoteEntry> aNotes;
    rDoc.GetAllNoteEntries(aNotes);
    for (const auto& rNote : aNotes)
    {
        if (rNote.maPos.Tab() != mnScTab)
            continue;

        mxNoteList->AppendNewRecord(new XclExpNote(GetRoot(), rNote.maPos, rNote.mpNote, u""));
    }

    // WSBOOL needs data from page settings, create it here, add it later
    rtl::Reference<XclExpPageSettings> xPageSett = new XclExpPageSettings( GetRoot() );
    bool bFitToPages = xPageSett->GetPageData().mbFitToPages;

    if( eBiff <= EXC_BIFF5 )
    {
        Add( new ExcBof );
        Add( new ExcDummy_02a );
    }
    else
    {
        Add( new ExcBof8 );
        lcl_AddCalcPr( aRecList, *this );
    }

    // GUTS (count & size of outline icons)
    aRecList.AppendRecord( mxCellTable->CreateRecord( EXC_ID_GUTS ) );
    // DEFROWHEIGHT, created by the cell table
    aRecList.AppendRecord( mxCellTable->CreateRecord( EXC_ID2_DEFROWHEIGHT ) );

    // COUNTRY - in BIFF5/7 in every worksheet
    if( eBiff <= EXC_BIFF5 )
        Add( new XclExpCountry( GetRoot() ) );

    Add( new XclExpWsbool( bFitToPages ) );

    // page settings (SETUP and various other records)
    aRecList.AppendRecord( xPageSett );

    const ScTableProtection* pTabProtect = rDoc.GetTabProtection(mnScTab);
    if (pTabProtect && pTabProtect->isProtected())
    {
        Add( new XclExpProtection(true) );
        Add( new XclExpBoolRecord(oox::xls::BIFF_ID_SCENPROTECT, pTabProtect->isOptionEnabled(ScTableProtection::SCENARIOS)) );
        if (pTabProtect->isOptionEnabled(ScTableProtection::OBJECTS))
            Add( new XclExpBoolRecord(oox::xls::BIFF_ID_OBJECTPROTECT, true ));
        Add( new XclExpPassHash(pTabProtect->getPasswordHash(PASSHASH_XL)) );
    }

    // local link table: EXTERNCOUNT, EXTERNSHEET
    if( eBiff <= EXC_BIFF5 )
        aRecList.AppendRecord( CreateRecord( EXC_ID_EXTERNSHEET ) );

    if ( eBiff == EXC_BIFF8 )
        lcl_AddScenariosAndFilters( aRecList, GetRoot(), mnScTab );

    // cell table: DEFCOLWIDTH, COLINFO, DIMENSIONS, ROW, cell records
    aRecList.AppendRecord( mxCellTable );

    // MERGEDCELLS record, generated by the cell table
    aRecList.AppendRecord( mxCellTable->CreateRecord( EXC_ID_MERGEDCELLS ) );
    // label ranges
    if( eBiff == EXC_BIFF8 )
        Add( new XclExpLabelranges( GetRoot() ) );
    // data validation (DVAL and list of DV records), generated by the cell table
    aRecList.AppendRecord( mxCellTable->CreateRecord( EXC_ID_DVAL ) );

    if( eBiff == EXC_BIFF8 )
    {
        // all MSODRAWING and OBJ stuff of this sheet goes here
        aRecList.AppendRecord( GetObjectManager().ProcessDrawing( GetSdrPage( mnScTab ) ) );
        // pivot tables
        aRecList.AppendRecord( GetPivotTableManager().CreatePivotTablesRecord( mnScTab ) );
    }

    // list of NOTE records, generated by the cell table
    aRecList.AppendRecord( mxNoteList );

    // sheet view settings: WINDOW2, SCL, PANE, SELECTION
    aRecList.AppendNewRecord( new XclExpTabViewSettings( GetRoot(), mnScTab ) );

    if( eBiff == EXC_BIFF8 )
    {
        // sheet protection options
        Add( new XclExpSheetProtectOptions( GetRoot(), mnScTab ) );

        // enhanced protections if there are
        if (pTabProtect)
        {
            const ::std::vector<ScEnhancedProtection>& rProts( pTabProtect->getEnhancedProtection());
            for (const auto& rProt : rProts)
            {
                Add( new XclExpSheetEnhancedProtection( GetRoot(), rProt));
            }
        }

        // web queries
        Add( new XclExpWebQueryBuffer( GetRoot() ) );

        // conditional formats
        Add( new XclExpCondFormatBuffer( GetRoot(), XclExtLstRef() ) );

        if( HasVbaStorage() )
            if( nCodeNameIdx < GetExtDocOptions().GetCodeNameCount() )
                Add( new XclCodename( GetExtDocOptions().GetCodeName( nCodeNameIdx ) ) );
    }

    // list of HLINK records, generated by the cell table
    aRecList.AppendRecord( mxCellTable->CreateRecord( EXC_ID_HLINK ) );

    // change tracking
    if( rR.pUserBViewList )
    {
        XclExpUserBViewList::const_iterator iter;
        for ( iter = rR.pUserBViewList->cbegin(); iter != rR.pUserBViewList->cend(); ++iter)
        {
            Add( new XclExpUsersViewBegin( (*iter).GetGUID(), nExcTab ) );
            Add( new XclExpUsersViewEnd );
        }
    }

    // EOF
    Add( new ExcEof );
}

void ExcTable::FillAsTableXml()
{
    InitializeTable( mnScTab );

    ScDocument& rDoc = GetDoc();

    OSL_ENSURE( (mnScTab >= 0) && (mnScTab <= MAXTAB), "-ExcTable::Table(): mnScTab - no ordinary table!" );
    OSL_ENSURE( nExcTab <= o3tl::make_unsigned(MAXTAB), "-ExcTable::Table(): nExcTab - no ordinary table!" );

    // create a new OBJ list for this sheet (may be used by notes, autofilter, data validation)
    GetObjectManager().StartSheet();

    // cell table: DEFROWHEIGHT, DEFCOLWIDTH, COLINFO, DIMENSIONS, ROW, cell records
    mxCellTable = new XclExpCellTable( GetRoot() );

    //export cell notes
    std::vector<sc::NoteEntry> aNotes;
    rDoc.GetAllNoteEntries(aNotes);
    for (const auto& rNote : aNotes)
    {
        if (rNote.maPos.Tab() != mnScTab)
            continue;

        mxNoteList->AppendNewRecord(new XclExpNote(GetRoot(), rNote.maPos, rNote.mpNote, u""));
    }

    // WSBOOL needs data from page settings, create it here, add it later
    rtl::Reference<XclExpPageSettings> xPageSett = new XclExpPageSettings( GetRoot() );
    XclExtLstRef xExtLst = new XclExtLst( GetRoot() );
    bool bFitToPages = xPageSett->GetPageData().mbFitToPages;

    Color aTabColor = GetRoot().GetDoc().GetTabBgColor(mnScTab);
    Add(new XclExpXmlSheetPr(bFitToPages, mnScTab, aTabColor, &GetFilterManager()));

    // GUTS (count & size of outline icons)
    aRecList.AppendRecord( mxCellTable->CreateRecord( EXC_ID_GUTS ) );
    // DEFROWHEIGHT, created by the cell table
    aRecList.AppendRecord( mxCellTable->CreateRecord( EXC_ID2_DEFROWHEIGHT ) );

    aRecList.AppendRecord( mxCellTable->CreateRecord( EXC_ID3_DIMENSIONS ) );

    // sheet view settings: WINDOW2, SCL, PANE, SELECTION
    aRecList.AppendNewRecord( new XclExpTabViewSettings( GetRoot(), mnScTab ) );

    // cell table: DEFCOLWIDTH, COLINFO, DIMENSIONS, ROW, cell records
    aRecList.AppendRecord( mxCellTable );

    // list of NOTE records, generated by the cell table
    // not in the worksheet file
    if( mxNoteList != nullptr && !mxNoteList->IsEmpty() )
        aRecList.AppendNewRecord( new XclExpComments( mnScTab, *mxNoteList ) );

    const ScTableProtection* pTabProtect = rDoc.GetTabProtection(mnScTab);
    if (pTabProtect && pTabProtect->isProtected())
        Add( new XclExpSheetProtection(true, mnScTab) );

    lcl_AddScenariosAndFilters( aRecList, GetRoot(), mnScTab );

    // MERGEDCELLS record, generated by the cell table
    aRecList.AppendRecord( mxCellTable->CreateRecord( EXC_ID_MERGEDCELLS ) );

    // conditional formats
    Add( new XclExpCondFormatBuffer( GetRoot(), xExtLst ) );

    // data validation (DVAL and list of DV records), generated by the cell table
    aRecList.AppendRecord( mxCellTable->CreateRecord( EXC_ID_DVAL ) );

    // list of HLINK records, generated by the cell table
    XclExpRecordRef xHyperlinks = mxCellTable->CreateRecord( EXC_ID_HLINK );
    XclExpHyperlinkList* pHyperlinkList = dynamic_cast<XclExpHyperlinkList*>(xHyperlinks.get());
    if( pHyperlinkList != nullptr && !pHyperlinkList->IsEmpty() )
    {
        aRecList.AppendNewRecord( new XclExpXmlStartElementRecord( XML_hyperlinks ) );
        aRecList.AppendRecord( xHyperlinks );
        aRecList.AppendNewRecord( new XclExpXmlEndElementRecord( XML_hyperlinks ) );
    }

    aRecList.AppendRecord( xPageSett );

    // all MSODRAWING and OBJ stuff of this sheet goes here
    aRecList.AppendRecord( GetObjectManager().ProcessDrawing( GetSdrPage( mnScTab ) ) );

    XclExpImgData* pImgData = xPageSett->getGraphicExport();
    if (pImgData)
        aRecList.AppendRecord(pImgData);

    // <tableParts> after <drawing> and before <extLst>
    aRecList.AppendRecord( GetTablesManager().GetTablesBySheet( mnScTab));

    aRecList.AppendRecord( xExtLst );
}

void ExcTable::FillAsEmptyTable( SCTAB nCodeNameIdx )
{
    InitializeTable( mnScTab );

    if( !(HasVbaStorage() && (nCodeNameIdx < GetExtDocOptions().GetCodeNameCount())) )
        return;

    if( GetBiff() <= EXC_BIFF5 )
    {
        Add( new ExcBof );
    }
    else
    {
        Add( new ExcBof8 );
        Add( new XclCodename( GetExtDocOptions().GetCodeName( nCodeNameIdx ) ) );
    }
    // sheet view settings: WINDOW2, SCL, PANE, SELECTION
    aRecList.AppendNewRecord( new XclExpTabViewSettings( GetRoot(), mnScTab ) );
    Add( new ExcEof );
}

void ExcTable::Write( XclExpStream& rStrm )
{
    SetCurrScTab( mnScTab );
    if( mxCellTable )
        mxCellTable->Finalize(true);
    aRecList.Save( rStrm );
}

void ExcTable::WriteXml( XclExpXmlStream& rStrm )
{
    if (!GetTabInfo().IsExportTab(mnScTab))
    {
        // header export.
        SetCurrScTab(mnScTab);
        if (mxCellTable)
            mxCellTable->Finalize(false);
        aRecList.SaveXml(rStrm);

        return;
    }

    // worksheet export
    OUString sSheetName = XclXmlUtils::GetStreamName( "xl/", "worksheets/sheet", mnScTab+1 );

    sax_fastparser::FSHelperPtr pWorksheet = rStrm.GetStreamForPath( sSheetName );

    rStrm.PushStream( pWorksheet );

    pWorksheet->startElement( XML_worksheet,
        XML_xmlns, rStrm.getNamespaceURL(OOX_NS(xls)).toUtf8(),
        FSNS(XML_xmlns, XML_r), rStrm.getNamespaceURL(OOX_NS(officeRel)).toUtf8(),
        FSNS(XML_xmlns, XML_xdr), "http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing", // rStrm.getNamespaceURL(OOX_NS(xm)).toUtf8() -> "http://schemas.microsoft.com/office/excel/2006/main",
        FSNS(XML_xmlns, XML_x14), rStrm.getNamespaceURL(OOX_NS(xls14Lst)).toUtf8(),
        FSNS(XML_xmlns, XML_mc), rStrm.getNamespaceURL(OOX_NS(mce)).toUtf8());

    SetCurrScTab( mnScTab );
    if (mxCellTable)
        mxCellTable->Finalize(false);
    aRecList.SaveXml( rStrm );

    XclExpXmlPivotTables* pPT = GetXmlPivotTableManager().GetTablesBySheet(mnScTab);
    if (pPT)
        pPT->SaveXml(rStrm);

    rStrm.GetCurrentStream()->endElement( XML_worksheet );
    rStrm.PopStream();
}

ExcDocument::ExcDocument( const XclExpRoot& rRoot ) :
    XclExpRoot( rRoot ),
    aHeader( rRoot )
{
}

ExcDocument::~ExcDocument()
{
    maTableList.RemoveAllRecords();    // for the following assertion!
}

void ExcDocument::ReadDoc()
{
    InitializeConvert();

    if (GetOutput() == EXC_OUTPUT_BINARY)
        aHeader.FillAsHeaderBinary(maBoundsheetList);
    else
    {
        aHeader.FillAsHeaderXml(maBoundsheetList);
        GetXmlPivotTableManager().Initialize();
        GetTablesManager().Initialize();    // Move outside conditions if we wanted to support BIFF.
    }

    SCTAB nScTab = 0, nScTabCount = GetTabInfo().GetScTabCount();
    SCTAB nCodeNameIdx = 0, nCodeNameCount = GetExtDocOptions().GetCodeNameCount();

    for( ; nScTab < nScTabCount; ++nScTab )
    {
        if( GetTabInfo().IsExportTab( nScTab ) )
        {
            ExcTableList::RecordRefType xTab = new ExcTable( GetRoot(), nScTab );
            maTableList.AppendRecord( xTab );
            if (GetOutput() == EXC_OUTPUT_BINARY)
                xTab->FillAsTableBinary(nCodeNameIdx);
            else
                xTab->FillAsTableXml();

            ++nCodeNameIdx;
        }
    }
    for( ; nCodeNameIdx < nCodeNameCount; ++nScTab, ++nCodeNameIdx )
    {
        ExcTableList::RecordRefType xTab = new ExcTable( GetRoot(), nScTab );
        maTableList.AppendRecord( xTab );
        xTab->FillAsEmptyTable( nCodeNameIdx );
    }

    if ( GetBiff() == EXC_BIFF8 )
    {
        // complete temporary Escher stream
        GetObjectManager().EndDocument();

        // change tracking
        if ( GetDoc().GetChangeTrack() )
            m_xExpChangeTrack.reset(new XclExpChangeTrack( GetRoot() ));
    }
}

void ExcDocument::Write( SvStream& rSvStrm )
{
    if( !maTableList.IsEmpty() )
    {
        InitializeSave();

        XclExpStream aXclStrm( rSvStrm, GetRoot() );

        aHeader.Write( aXclStrm );

        OSL_ENSURE( maTableList.GetSize() == maBoundsheetList.GetSize(),
            "ExcDocument::Write - different number of sheets and BOUNDSHEET records" );

        for( size_t nTab = 0, nTabCount = maTableList.GetSize(); nTab < nTabCount; ++nTab )
        {
            // set current stream position in BOUNDSHEET record
            ExcBoundsheetRef xBoundsheet = maBoundsheetList.GetRecord( nTab );
            if( xBoundsheet )
                xBoundsheet->SetStreamPos( aXclStrm.GetSvStreamPos() );
            // write the table
            maTableList.GetRecord( nTab )->Write( aXclStrm );
        }

        // write the table stream positions into the BOUNDSHEET records
        for( size_t nBSheet = 0, nBSheetCount = maBoundsheetList.GetSize(); nBSheet < nBSheetCount; ++nBSheet )
            maBoundsheetList.GetRecord( nBSheet )->UpdateStreamPos( aXclStrm );
    }
    if( m_xExpChangeTrack )
        m_xExpChangeTrack->Write();
}

void ExcDocument::WriteXml( XclExpXmlStream& rStrm )
{
    SfxObjectShell* pDocShell = GetDocShell();

    using namespace ::com::sun::star;
    uno::Reference<document::XDocumentPropertiesSupplier> xDPS( pDocShell->GetModel(), uno::UNO_QUERY_THROW );
    uno::Reference<document::XDocumentProperties> xDocProps = xDPS->getDocumentProperties();

    OUString sUserName = GetUserName();
    sal_uInt32 nWriteProtHash = pDocShell->GetModifyPasswordHash();
    bool bHasPasswordHash = nWriteProtHash && !sUserName.isEmpty();
    const uno::Sequence<beans::PropertyValue> aInfo = pDocShell->GetModifyPasswordInfo();
    OUString sAlgorithm, sSalt, sHash;
    sal_Int32 nCount = 0;
    for (const auto& prop : aInfo)
    {
        if (prop.Name == "algorithm-name")
            prop.Value >>= sAlgorithm;
        else if (prop.Name == "salt")
            prop.Value >>= sSalt;
        else if (prop.Name == "iteration-count")
            prop.Value >>= nCount;
        else if (prop.Name == "hash")
            prop.Value >>= sHash;
    }
    bool bHasPasswordInfo
        = sAlgorithm != "PBKDF2" && !sSalt.isEmpty() && !sHash.isEmpty() && !sUserName.isEmpty();
    rStrm.exportDocumentProperties(xDocProps, pDocShell->IsSecurityOptOpenReadOnly()
                                                  && !bHasPasswordHash && !bHasPasswordInfo);
    rStrm.exportCustomFragments();

    sax_fastparser::FSHelperPtr& rWorkbook = rStrm.GetCurrentStream();
    rWorkbook->startElement( XML_workbook,
            XML_xmlns, rStrm.getNamespaceURL(OOX_NS(xls)).toUtf8(),
            FSNS(XML_xmlns, XML_r), rStrm.getNamespaceURL(OOX_NS(officeRel)).toUtf8() );
    rWorkbook->singleElement( XML_fileVersion,
            XML_appName, "Calc"
            // OOXTODO: XML_codeName
            // OOXTODO: XML_lastEdited
            // OOXTODO: XML_lowestEdited
            // OOXTODO: XML_rupBuild
    );

    if (bHasPasswordHash)
        rWorkbook->singleElement(XML_fileSharing,
                XML_userName, sUserName,
                XML_reservationPassword, OString::number(nWriteProtHash, 16).getStr());
    else if (bHasPasswordInfo)
        rWorkbook->singleElement(XML_fileSharing,
                XML_userName, sUserName,
                XML_algorithmName, sAlgorithm.toUtf8().getStr(),
                XML_hashValue, sHash.toUtf8().getStr(),
                XML_saltValue, sSalt.toUtf8().getStr(),
                XML_spinCount, OString::number(nCount).getStr());

    if( !maTableList.IsEmpty() )
    {
        InitializeSave();

        aHeader.WriteXml( rStrm );

        for( size_t nTab = 0, nTabCount = maTableList.GetSize(); nTab < nTabCount; ++nTab )
        {
            // write the table
            maTableList.GetRecord( nTab )->WriteXml( rStrm );
        }
    }

    if( m_xExpChangeTrack )
        m_xExpChangeTrack->WriteXml( rStrm );

    XclExpXmlPivotCaches& rCaches = GetXmlPivotTableManager().GetCaches();
    if (rCaches.HasCaches())
        rCaches.SaveXml(rStrm);

    const ScCalcConfig& rCalcConfig = GetDoc().GetCalcConfig();
    formula::FormulaGrammar::AddressConvention eConv = rCalcConfig.meStringRefAddressSyntax;

    // don't save "unspecified" string ref syntax ... query formula grammar
    // and save that instead
    if( eConv == formula::FormulaGrammar::CONV_UNSPECIFIED)
    {
        eConv = GetDoc().GetAddressConvention();
    }

    // write if it has been read|imported or explicitly changed
    // or if ref syntax isn't what would be native for our file format
    // i.e. ExcelA1 in this case
    if ( rCalcConfig.mbHasStringRefSyntax ||
         (eConv != formula::FormulaGrammar::CONV_XL_A1) )
    {
        XclExtLstRef xExtLst = new XclExtLst( GetRoot()  );
        xExtLst->AddRecord( new XclExpExtCalcPr( GetRoot(), eConv )  );
        xExtLst->SaveXml(rStrm);
    }

    rWorkbook->endElement( XML_workbook );
    rWorkbook.reset();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
