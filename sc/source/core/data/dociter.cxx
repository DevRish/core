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

#include <svl/numformat.hxx>
#include <svl/zforlist.hxx>

#include <global.hxx>
#include <dociter.hxx>
#include <document.hxx>
#include <table.hxx>
#include <column.hxx>
#include <formulacell.hxx>
#include <attarray.hxx>
#include <patattr.hxx>
#include <docoptio.hxx>
#include <cellform.hxx>
#include <segmenttree.hxx>
#include <progress.hxx>
#include <queryparam.hxx>
#include <queryentry.hxx>
#include <globstr.hrc>
#include <scresid.hxx>
#include <cellvalue.hxx>
#include <scmatrix.hxx>
#include <rowheightcontext.hxx>
#include <queryevaluator.hxx>

#include <o3tl/safeint.hxx>
#include <tools/fract.hxx>
#include <editeng/editobj.hxx>
#include <svl/sharedstring.hxx>
#include <unotools/collatorwrapper.hxx>
#include <osl/diagnose.h>
#include <sal/log.hxx>

#include <algorithm>
#include <limits>
#include <vector>

using ::rtl::math::approxEqual;
using ::std::vector;
using ::std::set;

// iterators have very high frequency use -> custom debug.
// #define debugiter(...) fprintf(stderr, __VA_ARGS__)
#define debugiter(...)

namespace {

template<typename Iter>
void incBlock(std::pair<Iter, size_t>& rPos)
{
    // Move to the next block.
    ++rPos.first;
    rPos.second = 0;
}

template<typename Iter>
void decBlock(std::pair<Iter, size_t>& rPos)
{
    // Move to the last element of the previous block.
    --rPos.first;
    rPos.second = rPos.first->size - 1;
}

}

static void ScAttrArray_IterGetNumberFormat( sal_uInt32& nFormat, const ScAttrArray*& rpArr,
        SCROW& nAttrEndRow, const ScAttrArray* pNewArr, SCROW nRow,
        const ScDocument& rDoc, const ScInterpreterContext* pContext = nullptr )
{
    if ( rpArr == pNewArr && nAttrEndRow >= nRow )
        return;

    SCROW nRowStart = 0;
    SCROW nRowEnd = rDoc.MaxRow();
    const ScPatternAttr* pPattern = pNewArr->GetPatternRange( nRowStart, nRowEnd, nRow );
    if( !pPattern )
    {
        pPattern = rDoc.GetDefPattern();
        nRowEnd = rDoc.MaxRow();
    }

    nFormat = pPattern->GetNumberFormat( pContext ? pContext->GetFormatTable() : rDoc.GetFormatTable() );
    rpArr = pNewArr;
    nAttrEndRow = nRowEnd;
}

ScValueIterator::ScValueIterator( ScDocument& rDocument, const ScRange& rRange,
            SubtotalFlags nSubTotalFlags, bool bTextZero )
    : mrDoc(rDocument)
    , pContext(nullptr)
    , pAttrArray(nullptr)
    , nNumFormat(0) // Initialized in GetNumberFormat
    , nNumFmtIndex(0)
    , maStartPos(rRange.aStart)
    , maEndPos(rRange.aEnd)
    , mnCol(0)
    , mnTab(0)
    , nAttrEndRow(0)
    , mnSubTotalFlags(nSubTotalFlags)
    , nNumFmtType(SvNumFormatType::UNDEFINED)
    , bNumValid(false)
    , bCalcAsShown(rDocument.GetDocOptions().IsCalcAsShown())
    , bTextAsZero(bTextZero)
    , mpCells(nullptr)
{
    SCTAB nDocMaxTab = rDocument.GetTableCount() - 1;

    if (!rDocument.ValidCol(maStartPos.Col())) maStartPos.SetCol(mrDoc.MaxCol());
    if (!rDocument.ValidCol(maEndPos.Col())) maEndPos.SetCol(mrDoc.MaxCol());
    if (!rDocument.ValidRow(maStartPos.Row())) maStartPos.SetRow(mrDoc.MaxRow());
    if (!rDocument.ValidRow(maEndPos.Row())) maEndPos.SetRow(mrDoc.MaxRow());
    if (!ValidTab(maStartPos.Tab()) || maStartPos.Tab() > nDocMaxTab) maStartPos.SetTab(nDocMaxTab);
    if (!ValidTab(maEndPos.Tab()) || maEndPos.Tab() > nDocMaxTab) maEndPos.SetTab(nDocMaxTab);
}

SCROW ScValueIterator::GetRow() const
{
    // Position of the head of the current block + offset within the block
    // equals the logical element position.
    return maCurPos.first->position + maCurPos.second;
}

void ScValueIterator::IncBlock()
{
    ++maCurPos.first;
    maCurPos.second = 0;
}

void ScValueIterator::IncPos()
{
    if (maCurPos.second + 1 < maCurPos.first->size)
        // Move within the same block.
        ++maCurPos.second;
    else
        // Move to the next block.
        IncBlock();
}

bool ScValueIterator::GetThis(double& rValue, FormulaError& rErr)
{
    while (true)
    {
        bool bNextColumn = !mpCells || maCurPos.first == mpCells->end();
        if (!bNextColumn)
        {
            if (GetRow() > maEndPos.Row())
                bNextColumn = true;
        }

        ScColumn* pCol;
        if (!bNextColumn)
            pCol = &(mrDoc.maTabs[mnTab])->aCol[mnCol];
        else
        {
            // Find the next available column.
            do
            {
                ++mnCol;
                if (mnCol > maEndPos.Col() || mnCol >= mrDoc.maTabs[mnTab]->GetAllocatedColumnsCount())
                {
                    mnCol = maStartPos.Col();
                    ++mnTab;
                    if (mnTab > maEndPos.Tab())
                    {
                        rErr = FormulaError::NONE;
                        return false; // Over and out
                    }
                }
                pCol = &(mrDoc.maTabs[mnTab])->aCol[mnCol];
            }
            while (pCol->IsEmptyData());

            mpCells = &pCol->maCells;
            maCurPos = mpCells->position(maStartPos.Row());
        }

        SCROW nCurRow = GetRow();
        SCROW nLastRow;
        // Skip all filtered or hidden rows, depending on mnSubTotalFlags
        if ( ( ( mnSubTotalFlags & SubtotalFlags::IgnoreFiltered ) &&
               mrDoc.RowFiltered( nCurRow, mnTab, nullptr, &nLastRow ) ) ||
             ( ( mnSubTotalFlags & SubtotalFlags::IgnoreHidden ) &&
               mrDoc.RowHidden( nCurRow, mnTab, nullptr, &nLastRow ) ) )
        {
            maCurPos = mpCells->position(maCurPos.first, nLastRow+1);
            continue;
        }

        switch (maCurPos.first->type)
        {
            case sc::element_type_numeric:
            {
                bNumValid = false;
                rValue = sc::numeric_block::at(*maCurPos.first->data, maCurPos.second);
                rErr = FormulaError::NONE;
                if (bCalcAsShown)
                {
                    ScAttrArray_IterGetNumberFormat(nNumFormat, pAttrArray,
                        nAttrEndRow, pCol->pAttrArray.get(), nCurRow, mrDoc, pContext);
                    rValue = mrDoc.RoundValueAsShown(rValue, nNumFormat, pContext);
                }
                return true; // Found it!
            }
            break;
            case sc::element_type_formula:
            {
                ScFormulaCell& rCell = *sc::formula_block::at(*maCurPos.first->data, maCurPos.second);
                if ( ( mnSubTotalFlags & SubtotalFlags::IgnoreNestedStAg ) && rCell.IsSubTotal() )
                {
                    // Skip subtotal formula cells.
                    IncPos();
                    break;
                }

                if (rCell.GetErrorOrValue(rErr, rValue))
                {
                    if ( rErr != FormulaError::NONE && ( mnSubTotalFlags & SubtotalFlags::IgnoreErrVal ) )
                    {
                        IncPos();
                        break;
                    }
                    bNumValid = false;
                    return true; // Found it!
                }
                else if (bTextAsZero)
                {
                    rValue = 0.0;
                    bNumValid = false;
                    return true;
                }
                IncPos();
            }
            break;
            case sc::element_type_string :
            case sc::element_type_edittext :
            {
                if (bTextAsZero)
                {
                    rErr = FormulaError::NONE;
                    rValue = 0.0;
                    nNumFmtType = SvNumFormatType::NUMBER;
                    nNumFmtIndex = 0;
                    bNumValid = true;
                    return true;
                }
                IncBlock();
            }
            break;
            case sc::element_type_empty:
            default:
                // Skip the whole block.
                IncBlock();
        }
    }
}

void ScValueIterator::GetCurNumFmtInfo( const ScInterpreterContext& rContext, SvNumFormatType& nType, sal_uInt32& nIndex )
{
    if (!bNumValid && mnTab < mrDoc.GetTableCount())
    {
        SCROW nCurRow = GetRow();
        const ScColumn* pCol = &(mrDoc.maTabs[mnTab])->aCol[mnCol];
        nNumFmtIndex = pCol->GetNumberFormat(rContext, nCurRow);
        nNumFmtType = rContext.GetNumberFormatType( nNumFmtIndex );
        bNumValid = true;
    }

    nType = nNumFmtType;
    nIndex = nNumFmtIndex;
}

bool ScValueIterator::GetFirst(double& rValue, FormulaError& rErr)
{
    mnCol = maStartPos.Col();
    mnTab = maStartPos.Tab();

    ScTable* pTab = mrDoc.FetchTable(mnTab);
    if (!pTab)
        return false;

    nNumFormat = 0; // Initialized in GetNumberFormat
    pAttrArray = nullptr;
    nAttrEndRow = 0;

    auto nCol = maStartPos.Col();
    if (nCol < pTab->GetAllocatedColumnsCount())
    {
        mpCells = &pTab->aCol[nCol].maCells;
        maCurPos = mpCells->position(maStartPos.Row());
    }
    else
        mpCells = nullptr;
    return GetThis(rValue, rErr);
}

bool ScValueIterator::GetNext(double& rValue, FormulaError& rErr)
{
    IncPos();
    return GetThis(rValue, rErr);
}

ScDBQueryDataIterator::DataAccess::DataAccess()
{
}

ScDBQueryDataIterator::DataAccess::~DataAccess()
{
}

const sc::CellStoreType* ScDBQueryDataIterator::GetColumnCellStore(ScDocument& rDoc, SCTAB nTab, SCCOL nCol)
{
    ScTable* pTab = rDoc.FetchTable(nTab);
    if (!pTab)
        return nullptr;
    if (nCol >= pTab->GetAllocatedColumnsCount())
        return nullptr;
    return &pTab->aCol[nCol].maCells;
}

const ScAttrArray* ScDBQueryDataIterator::GetAttrArrayByCol(ScDocument& rDoc, SCTAB nTab, SCCOL nCol)
{
    assert(nTab < rDoc.GetTableCount() && "index out of bounds, FIX IT");
    ScColumn* pCol = &rDoc.maTabs[nTab]->aCol[nCol];
    return pCol->pAttrArray.get();
}

bool ScDBQueryDataIterator::IsQueryValid(
    ScDocument& rDoc, const ScQueryParam& rParam, SCTAB nTab, SCROW nRow, const ScRefCellValue* pCell)
{
    assert(nTab < rDoc.GetTableCount() && "index out of bounds, FIX IT");
    ScQueryEvaluator queryEvaluator(rDoc, *rDoc.maTabs[nTab], rParam);
    return queryEvaluator.ValidQuery(nRow, pCell);
}

ScDBQueryDataIterator::DataAccessInternal::DataAccessInternal(ScDBQueryParamInternal* pParam, ScDocument& rDoc, const ScInterpreterContext& rContext)
    : mpCells(nullptr)
    , mpParam(pParam)
    , mrDoc(rDoc)
    , mrContext(rContext)
    , pAttrArray(nullptr)
    , nNumFormat(0) // Initialized in GetNumberFormat
    , nNumFmtIndex(0)
    , nCol(mpParam->mnField)
    , nRow(mpParam->nRow1)
    , nAttrEndRow(0)
    , nTab(mpParam->nTab)
    , nNumFmtType(SvNumFormatType::ALL)
    , bCalcAsShown(rDoc.GetDocOptions().IsCalcAsShown())
{
    SCSIZE i;
    SCSIZE nCount = mpParam->GetEntryCount();
    for (i=0; (i<nCount) && (mpParam->GetEntry(i).bDoQuery); i++)
    {
        ScQueryEntry& rEntry = mpParam->GetEntry(i);
        ScQueryEntry::QueryItemsType& rItems = rEntry.GetQueryItems();
        rItems.resize(1);
        ScQueryEntry::Item& rItem = rItems.front();
        sal_uInt32 nIndex = 0;
        bool bNumber = mrDoc.GetFormatTable()->IsNumberFormat(
            rItem.maString.getString(), nIndex, rItem.mfVal);
        rItem.meType = bNumber ? ScQueryEntry::ByValue : ScQueryEntry::ByString;
    }
}

ScDBQueryDataIterator::DataAccessInternal::~DataAccessInternal()
{
}

bool ScDBQueryDataIterator::DataAccessInternal::getCurrent(Value& rValue)
{
    // Start with the current row position, and find the first row position
    // that satisfies the query.

    // If the query starts in the same column as the result vector we can
    // prefetch the cell which saves us one fetch in the success case.
    SCCOLROW nFirstQueryField = mpParam->GetEntry(0).nField;
    ScRefCellValue aCell;

    while (true)
    {
        if (maCurPos.first == mpCells->end() || nRow > mpParam->nRow2)
        {
            // Bottom of the range reached. Bail out.
            rValue.mnError = FormulaError::NONE;
            return false;
        }

        if (maCurPos.first->type == sc::element_type_empty)
        {
            // Skip the whole empty block.
            incBlock();
            continue;
        }

        ScRefCellValue* pCell = nullptr;
        if (nCol == static_cast<SCCOL>(nFirstQueryField))
        {
            aCell = sc::toRefCell(maCurPos.first, maCurPos.second);
            pCell = &aCell;
        }

        if (ScDBQueryDataIterator::IsQueryValid(mrDoc, *mpParam, nTab, nRow, pCell))
        {
            if (!pCell)
                aCell = sc::toRefCell(maCurPos.first, maCurPos.second);
            switch (aCell.meType)
            {
                case CELLTYPE_VALUE:
                {
                    rValue.mfValue = aCell.mfValue;
                    rValue.mbIsNumber = true;
                    if ( bCalcAsShown )
                    {
                        const ScAttrArray* pNewAttrArray =
                            ScDBQueryDataIterator::GetAttrArrayByCol(mrDoc, nTab, nCol);
                        ScAttrArray_IterGetNumberFormat( nNumFormat, pAttrArray,
                            nAttrEndRow, pNewAttrArray, nRow, mrDoc );
                        rValue.mfValue = mrDoc.RoundValueAsShown( rValue.mfValue, nNumFormat );
                    }
                    nNumFmtType = SvNumFormatType::NUMBER;
                    nNumFmtIndex = 0;
                    rValue.mnError = FormulaError::NONE;
                    return true; // Found it!
                }

                case CELLTYPE_FORMULA:
                {
                    if (aCell.mpFormula->IsValue())
                    {
                        rValue.mfValue = aCell.mpFormula->GetValue();
                        rValue.mbIsNumber = true;
                        mrDoc.GetNumberFormatInfo(
                            mrContext, nNumFmtType, nNumFmtIndex, ScAddress(nCol, nRow, nTab));
                        rValue.mnError = aCell.mpFormula->GetErrCode();
                        return true; // Found it!
                    }
                    else if(mpParam->mbSkipString)
                        incPos();
                    else
                    {
                        rValue.maString = aCell.mpFormula->GetString().getString();
                        rValue.mfValue = 0.0;
                        rValue.mnError = aCell.mpFormula->GetErrCode();
                        rValue.mbIsNumber = false;
                        return true;
                    }
                }
                break;
                case CELLTYPE_STRING:
                case CELLTYPE_EDIT:
                    if (mpParam->mbSkipString)
                        incPos();
                    else
                    {
                        rValue.maString = aCell.getString(&mrDoc);
                        rValue.mfValue = 0.0;
                        rValue.mnError = FormulaError::NONE;
                        rValue.mbIsNumber = false;
                        return true;
                    }
                break;
                default:
                    incPos();
            }
        }
        else
            incPos();
    }
// statement unreachable
}

bool ScDBQueryDataIterator::DataAccessInternal::getFirst(Value& rValue)
{
    if (mpParam->bHasHeader)
        ++nRow;

    mpCells = ScDBQueryDataIterator::GetColumnCellStore(mrDoc, nTab, nCol);
    if (!mpCells)
        return false;

    maCurPos = mpCells->position(nRow);
    return getCurrent(rValue);
}

bool ScDBQueryDataIterator::DataAccessInternal::getNext(Value& rValue)
{
    if (!mpCells || maCurPos.first == mpCells->end())
        return false;

    incPos();
    return getCurrent(rValue);
}

void ScDBQueryDataIterator::DataAccessInternal::incBlock()
{
    ++maCurPos.first;
    maCurPos.second = 0;

    nRow = maCurPos.first->position;
}

void ScDBQueryDataIterator::DataAccessInternal::incPos()
{
    if (maCurPos.second + 1 < maCurPos.first->size)
    {
        // Move within the same block.
        ++maCurPos.second;
        ++nRow;
    }
    else
        // Move to the next block.
        incBlock();
}

ScDBQueryDataIterator::DataAccessMatrix::DataAccessMatrix(ScDBQueryParamMatrix* pParam)
    : mpParam(pParam)
    , mnCurRow(0)
{
    SCSIZE nC, nR;
    mpParam->mpMatrix->GetDimensions(nC, nR);
    mnRows = static_cast<SCROW>(nR);
}

ScDBQueryDataIterator::DataAccessMatrix::~DataAccessMatrix()
{
}

bool ScDBQueryDataIterator::DataAccessMatrix::getCurrent(Value& rValue)
{
    // Starting from row == mnCurRow, get the first row that satisfies all the
    // query parameters.
    for ( ;mnCurRow < mnRows; ++mnCurRow)
    {
        const ScMatrix& rMat = *mpParam->mpMatrix;
        if (rMat.IsEmpty(mpParam->mnField, mnCurRow))
            // Don't take empty values into account.
            continue;

        bool bIsStrVal = rMat.IsStringOrEmpty(mpParam->mnField, mnCurRow);
        if (bIsStrVal && mpParam->mbSkipString)
            continue;

        if (isValidQuery(mnCurRow, rMat))
        {
            rValue.maString = rMat.GetString(mpParam->mnField, mnCurRow).getString();
            rValue.mfValue = rMat.GetDouble(mpParam->mnField, mnCurRow);
            rValue.mbIsNumber = !bIsStrVal;
            rValue.mnError = FormulaError::NONE;
            return true;
        }
    }
    return false;
}

bool ScDBQueryDataIterator::DataAccessMatrix::getFirst(Value& rValue)
{
    mnCurRow = mpParam->bHasHeader ? 1 : 0;
    return getCurrent(rValue);
}

bool ScDBQueryDataIterator::DataAccessMatrix::getNext(Value& rValue)
{
    ++mnCurRow;
    return getCurrent(rValue);
}

namespace {

bool isQueryByValue(const ScQueryEntry::Item& rItem, const ScMatrix& rMat, SCSIZE nCol, SCSIZE nRow)
{
    if (rItem.meType == ScQueryEntry::ByString)
        return false;

    if (!rMat.IsValueOrEmpty(nCol, nRow))
        return false;

    return true;
}

bool isQueryByString(const ScQueryEntry& rEntry, const ScQueryEntry::Item& rItem, const ScMatrix& rMat, SCSIZE nCol, SCSIZE nRow)
{
    switch (rEntry.eOp)
    {
        case SC_EQUAL:
        case SC_NOT_EQUAL:
        case SC_CONTAINS:
        case SC_DOES_NOT_CONTAIN:
        case SC_BEGINS_WITH:
        case SC_ENDS_WITH:
        case SC_DOES_NOT_BEGIN_WITH:
        case SC_DOES_NOT_END_WITH:
            return true;
        default:
            ;
    }

    return rItem.meType == ScQueryEntry::ByString && rMat.IsStringOrEmpty(nCol, nRow);
}

}

bool ScDBQueryDataIterator::DataAccessMatrix::isValidQuery(SCROW nRow, const ScMatrix& rMat) const
{
    SCSIZE nEntryCount = mpParam->GetEntryCount();
    vector<bool> aResults;
    aResults.reserve(nEntryCount);

    const CollatorWrapper& rCollator = ScGlobal::GetCollator(mpParam->bCaseSens);

    for (SCSIZE i = 0; i < nEntryCount; ++i)
    {
        const ScQueryEntry& rEntry = mpParam->GetEntry(i);
        const ScQueryEntry::Item& rItem = rEntry.GetQueryItem();
        if (!rEntry.bDoQuery)
            continue;

        switch (rEntry.eOp)
        {
            case SC_EQUAL:
            case SC_LESS:
            case SC_GREATER:
            case SC_LESS_EQUAL:
            case SC_GREATER_EQUAL:
            case SC_NOT_EQUAL:
                break;
            default:
                // Only the above operators are supported.
                SAL_WARN("sc.core", "Unsupported operator " << rEntry.eOp
                    << " in ScDBQueryDataIterator::DataAccessMatrix::isValidQuery()");
                continue;
        }

        bool bValid = false;

        SCSIZE nField = static_cast<SCSIZE>(rEntry.nField);
        if (isQueryByValue(rItem, rMat, nField, nRow))
        {
            // By value
            double fMatVal = rMat.GetDouble(nField, nRow);
            bool bEqual = approxEqual(fMatVal, rItem.mfVal);
            switch (rEntry.eOp)
            {
                case SC_EQUAL:
                    bValid = bEqual;
                break;
                case SC_LESS:
                    bValid = (fMatVal < rItem.mfVal) && !bEqual;
                break;
                case SC_GREATER:
                    bValid = (fMatVal > rItem.mfVal) && !bEqual;
                break;
                case SC_LESS_EQUAL:
                    bValid = (fMatVal < rItem.mfVal) || bEqual;
                break;
                case SC_GREATER_EQUAL:
                    bValid = (fMatVal > rItem.mfVal) || bEqual;
                break;
                case SC_NOT_EQUAL:
                    bValid = !bEqual;
                break;
                default:
                    ;
            }
        }
        else if (isQueryByString(rEntry, rItem, rMat, nField, nRow))
        {
            // By string
            do
            {
                // Equality check first.
                svl::SharedString aMatStr = rMat.GetString(nField, nRow);
                svl::SharedString aQueryStr = rEntry.GetQueryItem().maString;
                bool bDone = false;
                rtl_uString* p1 = mpParam->bCaseSens ? aMatStr.getData() : aMatStr.getDataIgnoreCase();
                rtl_uString* p2 = mpParam->bCaseSens ? aQueryStr.getData() : aQueryStr.getDataIgnoreCase();
                switch (rEntry.eOp)
                {
                    case SC_EQUAL:
                        bValid = (p1 == p2);
                        bDone = true;
                    break;
                    case SC_NOT_EQUAL:
                        bValid = (p1 != p2);
                        bDone = true;
                    break;
                    default:
                        ;
                }

                if (bDone)
                    break;

                // Unequality check using collator.
                sal_Int32 nCompare = rCollator.compareString(aMatStr.getString(), aQueryStr.getString());
                switch (rEntry.eOp)
                {
                    case SC_LESS :
                        bValid = (nCompare < 0);
                    break;
                    case SC_GREATER :
                        bValid = (nCompare > 0);
                    break;
                    case SC_LESS_EQUAL :
                        bValid = (nCompare <= 0);
                    break;
                    case SC_GREATER_EQUAL :
                        bValid = (nCompare >= 0);
                    break;
                    default:
                        ;
                }
            }
            while (false);
        }

        if (aResults.empty())
            // First query entry.
            aResults.push_back(bValid);
        else if (rEntry.eConnect == SC_AND)
        {
            // For AND op, tuck the result into the last result value.
            size_t n = aResults.size();
            aResults[n-1] = aResults[n-1] && bValid;
        }
        else
            // For OR op, store its own result.
            aResults.push_back(bValid);
    }

    // Row is valid as long as there is at least one result being true.
    return std::find(aResults.begin(), aResults.end(), true) != aResults.end();
}

ScDBQueryDataIterator::Value::Value()
    : mfValue(std::numeric_limits<double>::quiet_NaN())
    , mnError(FormulaError::NONE), mbIsNumber(true)
{
}

ScDBQueryDataIterator::ScDBQueryDataIterator(ScDocument& rDocument, const ScInterpreterContext& rContext, std::unique_ptr<ScDBQueryParamBase> pParam) :
    mpParam (std::move(pParam))
{
    switch (mpParam->GetType())
    {
        case ScDBQueryParamBase::INTERNAL:
        {
            ScDBQueryParamInternal* p = static_cast<ScDBQueryParamInternal*>(mpParam.get());
            mpData.reset(new DataAccessInternal(p, rDocument, rContext));
        }
        break;
        case ScDBQueryParamBase::MATRIX:
        {
            ScDBQueryParamMatrix* p = static_cast<ScDBQueryParamMatrix*>(mpParam.get());
            mpData.reset(new DataAccessMatrix(p));
        }
    }
}

bool ScDBQueryDataIterator::GetFirst(Value& rValue)
{
    return mpData->getFirst(rValue);
}

bool ScDBQueryDataIterator::GetNext(Value& rValue)
{
    return mpData->getNext(rValue);
}

ScFormulaGroupIterator::ScFormulaGroupIterator( ScDocument& rDoc ) :
    mrDoc(rDoc),
    mnTab(0),
    mnCol(0),
    mnIndex(0)
{
    ScTable *pTab = mrDoc.FetchTable(mnTab);
    ScColumn *pCol = pTab ? pTab->FetchColumn(mnCol) : nullptr;
    if (pCol)
    {
        mbNullCol = false;
        maEntries = pCol->GetFormulaGroupEntries();
    }
    else
        mbNullCol = true;
}

sc::FormulaGroupEntry* ScFormulaGroupIterator::first()
{
    return next();
}

sc::FormulaGroupEntry* ScFormulaGroupIterator::next()
{
    if (mnIndex >= maEntries.size() || mbNullCol)
    {
        while (mnIndex >= maEntries.size() || mbNullCol)
        {
            mnIndex = 0;
            mnCol++;
            if (mnCol > mrDoc.MaxCol())
            {
                mnCol = 0;
                mnTab++;
                if (mnTab >= mrDoc.GetTableCount())
                    return nullptr;
            }
            ScTable *pTab = mrDoc.FetchTable(mnTab);
            ScColumn *pCol = pTab ? pTab->FetchColumn(mnCol) : nullptr;
            if (pCol)
            {
                mbNullCol = false;
                maEntries = pCol->GetFormulaGroupEntries();
            }
            else
                mbNullCol = true;
        }
    }

    return &maEntries[mnIndex++];
}

ScCellIterator::ScCellIterator( ScDocument& rDoc, const ScRange& rRange, SubtotalFlags nSubTotalFlags ) :
    mrDoc(rDoc),
    maStartPos(rRange.aStart),
    maEndPos(rRange.aEnd),
    mnSubTotalFlags(nSubTotalFlags)
{
    init();
}

void ScCellIterator::incBlock()
{
    ++maCurColPos.first;
    maCurColPos.second = 0;

    maCurPos.SetRow(maCurColPos.first->position);
}

void ScCellIterator::incPos()
{
    if (maCurColPos.second + 1 < maCurColPos.first->size)
    {
        // Move within the same block.
        ++maCurColPos.second;
        maCurPos.IncRow();
    }
    else
        // Move to the next block.
        incBlock();
}

void ScCellIterator::setPos(size_t nPos)
{
    maCurColPos = getColumn()->maCells.position(maCurColPos.first, nPos);
    maCurPos.SetRow(nPos);
}

const ScColumn* ScCellIterator::getColumn() const
{
    return &mrDoc.maTabs[maCurPos.Tab()]->aCol[maCurPos.Col()];
}

void ScCellIterator::init()
{
    SCTAB nDocMaxTab = mrDoc.GetTableCount() - 1;

    PutInOrder(maStartPos, maEndPos);

    if (!mrDoc.ValidCol(maStartPos.Col())) maStartPos.SetCol(mrDoc.MaxCol());
    if (!mrDoc.ValidCol(maEndPos.Col())) maEndPos.SetCol(mrDoc.MaxCol());
    if (!mrDoc.ValidRow(maStartPos.Row())) maStartPos.SetRow(mrDoc.MaxRow());
    if (!mrDoc.ValidRow(maEndPos.Row())) maEndPos.SetRow(mrDoc.MaxRow());
    if (!ValidTab(maStartPos.Tab(), nDocMaxTab)) maStartPos.SetTab(nDocMaxTab);
    if (!ValidTab(maEndPos.Tab(), nDocMaxTab)) maEndPos.SetTab(nDocMaxTab);

    while (maEndPos.Tab() > 0 && !mrDoc.maTabs[maEndPos.Tab()])
        maEndPos.IncTab(-1); // Only the tables in use

    if (maStartPos.Tab() > maEndPos.Tab())
        maStartPos.SetTab(maEndPos.Tab());

    if (!mrDoc.maTabs[maStartPos.Tab()])
    {
        assert(!"Table not found");
        maStartPos = ScAddress(mrDoc.MaxCol()+1, mrDoc.MaxRow()+1, MAXTAB+1); // -> Abort on GetFirst.
    }
    else
    {
        maStartPos.SetCol(mrDoc.maTabs[maStartPos.Tab()]->ClampToAllocatedColumns(maStartPos.Col()));
    }

    maCurPos = maStartPos;
}

bool ScCellIterator::getCurrent()
{
    const ScColumn* pCol = getColumn();

    while (true)
    {
        bool bNextColumn = maCurColPos.first == pCol->maCells.end();
        if (!bNextColumn)
        {
            if (maCurPos.Row() > maEndPos.Row())
                bNextColumn = true;
        }

        if (bNextColumn)
        {
            // Move to the next column.
            maCurPos.SetRow(maStartPos.Row());
            do
            {
                maCurPos.IncCol();
                if (maCurPos.Col() >= mrDoc.GetAllocatedColumnsCount(maCurPos.Tab())
                    || maCurPos.Col() > maEndPos.Col())
                {
                    maCurPos.SetCol(maStartPos.Col());
                    maCurPos.IncTab();
                    if (maCurPos.Tab() > maEndPos.Tab())
                    {
                        maCurCell.clear();
                        return false; // Over and out
                    }
                }
                pCol = getColumn();
            }
            while (pCol->IsEmptyData());

            maCurColPos = pCol->maCells.position(maCurPos.Row());
        }

        if (maCurColPos.first->type == sc::element_type_empty)
        {
            incBlock();
            continue;
        }

        SCROW nLastRow;
        // Skip all filtered or hidden rows, depending on mSubTotalFlags
        if ( ( ( mnSubTotalFlags & SubtotalFlags::IgnoreFiltered ) &&
               pCol->GetDoc().RowFiltered(maCurPos.Row(), maCurPos.Tab(), nullptr, &nLastRow) ) ||
             ( ( mnSubTotalFlags & SubtotalFlags::IgnoreHidden ) &&
               pCol->GetDoc().RowHidden(maCurPos.Row(), maCurPos.Tab(), nullptr, &nLastRow) ) )
        {
            setPos(nLastRow+1);
            continue;
        }

        if (maCurColPos.first->type == sc::element_type_formula)
        {
            if ( mnSubTotalFlags != SubtotalFlags::NONE )
            {
                ScFormulaCell* pCell = sc::formula_block::at(*maCurColPos.first->data, maCurColPos.second);
                // Skip formula cells with Subtotal formulae or errors, depending on mnSubTotalFlags
                if ( ( ( mnSubTotalFlags & SubtotalFlags::IgnoreNestedStAg ) && pCell->IsSubTotal() ) ||
                     ( ( mnSubTotalFlags & SubtotalFlags::IgnoreErrVal ) && pCell->GetErrCode() != FormulaError::NONE ) )
                {
                    incPos();
                    continue;
                }
            }
        }

        maCurCell = sc::toRefCell(maCurColPos.first, maCurColPos.second);
        return true;
    }
    return false;
}

OUString ScCellIterator::getString() const
{
    return maCurCell.getString(&mrDoc);
}

ScCellValue ScCellIterator::getCellValue() const
{
    ScCellValue aRet;
    aRet.meType = maCurCell.meType;

    switch (maCurCell.meType)
    {
        case CELLTYPE_STRING:
            aRet.mpString = new svl::SharedString(*maCurCell.mpString);
        break;
        case CELLTYPE_EDIT:
            aRet.mpEditText = maCurCell.mpEditText->Clone().release();
        break;
        case CELLTYPE_VALUE:
            aRet.mfValue = maCurCell.mfValue;
        break;
        case CELLTYPE_FORMULA:
            aRet.mpFormula = maCurCell.mpFormula->Clone();
        break;
        default:
            ;
    }

    return aRet;
}

bool ScCellIterator::hasString() const
{
    return maCurCell.hasString();
}

bool ScCellIterator::isEmpty() const
{
    return maCurCell.isEmpty();
}

bool ScCellIterator::equalsWithoutFormat( const ScAddress& rPos ) const
{
    ScRefCellValue aOther(mrDoc, rPos);
    return maCurCell.equalsWithoutFormat(aOther);
}

bool ScCellIterator::first()
{
    if (!ValidTab(maCurPos.Tab()))
        return false;

    maCurPos = maStartPos;
    const ScColumn* pCol = getColumn();

    maCurColPos = pCol->maCells.position(maCurPos.Row());
    return getCurrent();
}

bool ScCellIterator::next()
{
    incPos();
    return getCurrent();
}

ScQueryCellIterator::ScQueryCellIterator(ScDocument& rDocument, const ScInterpreterContext& rContext, SCTAB nTable,
             const ScQueryParam& rParam, bool bMod ) :
    maParam(rParam),
    rDoc( rDocument ),
    mrContext( rContext ),
    nTab( nTable),
    nStopOnMismatch( nStopOnMismatchDisabled ),
    nTestEqualCondition( nTestEqualConditionDisabled ),
    bAdvanceQuery( false ),
    bIgnoreMismatchOnLeadingStrings( false )
{
    nCol = maParam.nCol1;
    nRow = maParam.nRow1;
    SCSIZE i;
    if (!bMod) // Or else it's already inserted
        return;

    SCSIZE nCount = maParam.GetEntryCount();
    for (i = 0; (i < nCount) && (maParam.GetEntry(i).bDoQuery); ++i)
    {
        ScQueryEntry& rEntry = maParam.GetEntry(i);
        ScQueryEntry::Item& rItem = rEntry.GetQueryItem();
        sal_uInt32 nIndex = 0;
        bool bNumber = mrContext.GetFormatTable()->IsNumberFormat(
            rItem.maString.getString(), nIndex, rItem.mfVal);
        rItem.meType = bNumber ? ScQueryEntry::ByValue : ScQueryEntry::ByString;
    }
}

void ScQueryCellIterator::InitPos()
{
    nRow = maParam.nRow1;
    if (maParam.bHasHeader && maParam.bByRow)
        ++nRow;
    const ScColumn& rCol = rDoc.maTabs[nTab]->CreateColumnIfNotExists(nCol);
    maCurPos = rCol.maCells.position(nRow);
}

void ScQueryCellIterator::IncPos()
{
    if (maCurPos.second + 1 < maCurPos.first->size)
    {
        // Move within the same block.
        ++maCurPos.second;
        ++nRow;
    }
    else
        // Move to the next block.
        IncBlock();
}

void ScQueryCellIterator::IncBlock()
{
    ++maCurPos.first;
    maCurPos.second = 0;

    nRow = maCurPos.first->position;
}

bool ScQueryCellIterator::GetThis()
{
    assert(nTab < rDoc.GetTableCount() && "index out of bounds, FIX IT");
    const ScQueryEntry& rEntry = maParam.GetEntry(0);
    const ScQueryEntry::Item& rItem = rEntry.GetQueryItem();

    SCCOLROW nFirstQueryField = rEntry.nField;
    bool bAllStringIgnore = bIgnoreMismatchOnLeadingStrings &&
        rItem.meType != ScQueryEntry::ByString;
    bool bFirstStringIgnore = bIgnoreMismatchOnLeadingStrings &&
        !maParam.bHasHeader && rItem.meType == ScQueryEntry::ByString &&
        ((maParam.bByRow && nRow == maParam.nRow1) ||
         (!maParam.bByRow && nCol == maParam.nCol1));
    bool bTestEqualCondition = false;
    ScQueryEvaluator queryEvaluator(rDoc, *rDoc.maTabs[nTab], maParam, &mrContext,
        (nTestEqualCondition ? &bTestEqualCondition : nullptr));

    ScColumn* pCol = &(rDoc.maTabs[nTab])->aCol[nCol];
    while (true)
    {
        bool bNextColumn = maCurPos.first == pCol->maCells.end();
        if (!bNextColumn)
        {
            if (nRow > maParam.nRow2)
                bNextColumn = true;
        }

        if (bNextColumn)
        {
            do
            {
                ++nCol;
                if (nCol > maParam.nCol2 || nCol >= rDoc.maTabs[nTab]->GetAllocatedColumnsCount())
                    return false; // Over and out
                if ( bAdvanceQuery )
                {
                    AdvanceQueryParamEntryField();
                    nFirstQueryField = rEntry.nField;
                }
                pCol = &(rDoc.maTabs[nTab])->aCol[nCol];
            }
            while (!rItem.mbMatchEmpty && pCol->IsEmptyData());

            InitPos();

            bFirstStringIgnore = bIgnoreMismatchOnLeadingStrings &&
                !maParam.bHasHeader && rItem.meType == ScQueryEntry::ByString &&
                maParam.bByRow;
        }

        if (maCurPos.first->type == sc::element_type_empty)
        {
            if (rItem.mbMatchEmpty && rEntry.GetQueryItems().size() == 1)
            {
                // This shortcut, instead of determining if any SC_OR query
                // exists or this query is SC_AND'ed (which wouldn't make
                // sense, but..) and evaluating them in ValidQuery(), is
                // possible only because the interpreter is the only caller
                // that sets mbMatchEmpty and there is only one item in those
                // cases.
                // XXX this would have to be reworked if other filters used it
                // in different manners and evaluation would have to be done in
                // ValidQuery().
                return true;
            }
            else
            {
                IncBlock();
                continue;
            }
        }

        ScRefCellValue aCell = sc::toRefCell(maCurPos.first, maCurPos.second);

        if (bAllStringIgnore && aCell.hasString())
            IncPos();
        else
        {
            if ( queryEvaluator.ValidQuery( nRow,
                    (nCol == static_cast<SCCOL>(nFirstQueryField) ? &aCell : nullptr)))
            {
                if ( nTestEqualCondition && bTestEqualCondition )
                    nTestEqualCondition |= nTestEqualConditionMatched;
                return !aCell.isEmpty(); // Found it!
            }
            else if ( nStopOnMismatch )
            {
                // Yes, even a mismatch may have a fulfilled equal
                // condition if regular expressions were involved and
                // SC_LESS_EQUAL or SC_GREATER_EQUAL were queried.
                if ( nTestEqualCondition && bTestEqualCondition )
                {
                    nTestEqualCondition |= nTestEqualConditionMatched;
                    nStopOnMismatch |= nStopOnMismatchOccurred;
                    return false;
                }
                bool bStop;
                if (bFirstStringIgnore)
                {
                    if (aCell.hasString())
                    {
                        IncPos();
                        bStop = false;
                    }
                    else
                        bStop = true;
                }
                else
                    bStop = true;
                if (bStop)
                {
                    nStopOnMismatch |= nStopOnMismatchOccurred;
                    return false;
                }
            }
            else
                IncPos();
        }
        bFirstStringIgnore = false;
    }
}

bool ScQueryCellIterator::GetFirst()
{
    assert(nTab < rDoc.GetTableCount() && "index out of bounds, FIX IT");
    nCol = maParam.nCol1;
    InitPos();
    return GetThis();
}

bool ScQueryCellIterator::GetNext()
{
    IncPos();
    if ( nStopOnMismatch )
        nStopOnMismatch = nStopOnMismatchEnabled;
    if ( nTestEqualCondition )
        nTestEqualCondition = nTestEqualConditionEnabled;
    return GetThis();
}

void ScQueryCellIterator::AdvanceQueryParamEntryField()
{
    SCSIZE nEntries = maParam.GetEntryCount();
    for ( SCSIZE j = 0; j < nEntries; j++  )
    {
        ScQueryEntry& rEntry = maParam.GetEntry( j );
        if ( rEntry.bDoQuery )
        {
            if ( rEntry.nField < rDoc.MaxCol() )
                rEntry.nField++;
            else
            {
                assert(!"AdvanceQueryParamEntryField: ++rEntry.nField > MAXCOL");
            }
        }
        else
            break;  // for
    }
}

bool ScQueryCellIterator::FindEqualOrSortedLastInRange( SCCOL& nFoundCol,
        SCROW& nFoundRow )
{
    // Set and automatically reset mpParam->mbRangeLookup when returning. We
    // could use comphelper::FlagRestorationGuard, but really, that one is
    // overengineered for this simple purpose here.
    struct BoolResetter
    {
        bool& mr;
        bool  mb;
        BoolResetter( bool& r, bool b ) : mr(r), mb(r) { r = b; }
        ~BoolResetter() { mr = mb; }
    } aRangeLookupResetter( maParam.mbRangeLookup, true);

    nFoundCol = rDoc.MaxCol()+1;
    nFoundRow = rDoc.MaxRow()+1;
    SetStopOnMismatch( true ); // assume sorted keys
    SetTestEqualCondition( true );
    bIgnoreMismatchOnLeadingStrings = true;
    bool bLiteral = maParam.eSearchType == utl::SearchParam::SearchType::Normal &&
        maParam.GetEntry(0).GetQueryItem().meType == ScQueryEntry::ByString;
    bool bBinary = maParam.bByRow &&
        (bLiteral || maParam.GetEntry(0).GetQueryItem().meType == ScQueryEntry::ByValue) &&
        (maParam.GetEntry(0).eOp == SC_LESS_EQUAL || maParam.GetEntry(0).eOp == SC_GREATER_EQUAL);
    bool bFound = false;
    if (bBinary)
    {
        if (BinarySearch())
        {
            // BinarySearch() already positions correctly and only needs real
            // query comparisons afterwards, skip the verification check below.
            maParam.mbRangeLookup = false;
            bFound = GetThis();
        }
    }
    else
    {
        bFound = GetFirst();
    }
    if (bFound)
    {
        // First equal entry or last smaller than (greater than) entry.
        PositionType aPosSave;
        bool bNext = false;
        do
        {
            nFoundCol = GetCol();
            nFoundRow = GetRow();
            aPosSave = maCurPos;
            if (IsEqualConditionFulfilled())
                break;
            bNext = GetNext();
        }
        while (bNext);

        // There may be no pNext but equal condition fulfilled if regular
        // expressions are involved. Keep the found entry and proceed.
        if (!bNext && !IsEqualConditionFulfilled())
        {
            // Step back to last in range and adjust position markers for
            // GetNumberFormat() or similar.
            SCCOL nColDiff = nCol - nFoundCol;
            nCol = nFoundCol;
            nRow = nFoundRow;
            maCurPos = aPosSave;
            if (maParam.mbRangeLookup)
            {
                // Verify that the found entry does not only fulfill the range
                // lookup but also the real query, i.e. not numeric was found
                // if query is ByString and vice versa.
                maParam.mbRangeLookup = false;
                // Step back the last field advance if GetNext() did one.
                if (bAdvanceQuery && nColDiff)
                {
                    SCSIZE nEntries = maParam.GetEntryCount();
                    for (SCSIZE j=0; j < nEntries; ++j)
                    {
                        ScQueryEntry& rEntry = maParam.GetEntry( j );
                        if (rEntry.bDoQuery)
                        {
                            if (rEntry.nField - nColDiff >= 0)
                                rEntry.nField -= nColDiff;
                            else
                            {
                                assert(!"FindEqualOrSortedLastInRange: rEntry.nField -= nColDiff < 0");
                            }
                        }
                        else
                            break;  // for
                    }
                }
                // Check it.
                if (!GetThis())
                {
                    nFoundCol = rDoc.MaxCol()+1;
                    nFoundRow = rDoc.MaxRow()+1;
                }
            }
        }
    }
    if ( IsEqualConditionFulfilled() )
    {
        // Position on last equal entry.
        SCSIZE nEntries = maParam.GetEntryCount();
        for ( SCSIZE j = 0; j < nEntries; j++  )
        {
            ScQueryEntry& rEntry = maParam.GetEntry( j );
            if ( rEntry.bDoQuery )
            {
                switch ( rEntry.eOp )
                {
                    case SC_LESS_EQUAL :
                    case SC_GREATER_EQUAL :
                        rEntry.eOp = SC_EQUAL;
                    break;
                    default:
                    {
                        // added to avoid warnings
                    }
                }
            }
            else
                break;  // for
        }
        PositionType aPosSave;
        bIgnoreMismatchOnLeadingStrings = false;
        SetTestEqualCondition( false );
        do
        {
            nFoundCol = GetCol();
            nFoundRow = GetRow();
            aPosSave = maCurPos;
        } while (GetNext());

        // Step back conditions are the same as above
        nCol = nFoundCol;
        nRow = nFoundRow;
        maCurPos = aPosSave;
        return true;
    }
    if ( (maParam.eSearchType != utl::SearchParam::SearchType::Normal) &&
            StoppedOnMismatch() )
    {
        // Assume found entry to be the last value less than respectively
        // greater than the query. But keep on searching for an equal match.
        SCSIZE nEntries = maParam.GetEntryCount();
        for ( SCSIZE j = 0; j < nEntries; j++  )
        {
            ScQueryEntry& rEntry = maParam.GetEntry( j );
            if ( rEntry.bDoQuery )
            {
                switch ( rEntry.eOp )
                {
                    case SC_LESS_EQUAL :
                    case SC_GREATER_EQUAL :
                        rEntry.eOp = SC_EQUAL;
                    break;
                    default:
                    {
                        // added to avoid warnings
                    }
                }
            }
            else
                break;  // for
        }
        SetStopOnMismatch( false );
        SetTestEqualCondition( false );
        if (GetNext())
        {
            // Last of a consecutive area, avoid searching the entire parameter
            // range as it is a real performance bottleneck in case of regular
            // expressions.
            PositionType aPosSave;
            do
            {
                nFoundCol = GetCol();
                nFoundRow = GetRow();
                aPosSave = maCurPos;
                SetStopOnMismatch( true );
            } while (GetNext());
            nCol = nFoundCol;
            nRow = nFoundRow;
            maCurPos = aPosSave;
        }
    }
    return (nFoundCol <= rDoc.MaxCol()) && (nFoundRow <= rDoc.MaxRow());
}

ScCountIfCellIterator::ScCountIfCellIterator(ScDocument& rDocument, const ScInterpreterContext& rContext, SCTAB nTable,
             const ScQueryParam& rParam ) :
    maParam(rParam),
    rDoc( rDocument ),
    mrContext( rContext ),
    nTab( nTable)
{
    maParam.nCol1 = rDoc.maTabs[nTable]->ClampToAllocatedColumns(maParam.nCol1);
    maParam.nCol2 = rDoc.maTabs[nTable]->ClampToAllocatedColumns(maParam.nCol2);
    nCol = maParam.nCol1;
    nRow = maParam.nRow1;
}

void ScCountIfCellIterator::InitPos()
{
    nRow = maParam.nRow1;
    if (maParam.bHasHeader && maParam.bByRow)
        ++nRow;
    ScColumn* pCol = &(rDoc.maTabs[nTab])->aCol[nCol];
    maCurPos = pCol->maCells.position(nRow);
}

void ScCountIfCellIterator::IncPos()
{
    if (maCurPos.second + 1 < maCurPos.first->size)
    {
        // Move within the same block.
        ++maCurPos.second;
        ++nRow;
    }
    else
        // Move to the next block.
        IncBlock();
}

void ScCountIfCellIterator::IncBlock()
{
    ++maCurPos.first;
    maCurPos.second = 0;

    nRow = maCurPos.first->position;
}

int ScCountIfCellIterator::GetCount()
{
    assert(nTab < rDoc.GetTableCount() && "try to access index out of bounds, FIX IT");
    nCol = maParam.nCol1;
    InitPos();

    const ScQueryEntry& rEntry = maParam.GetEntry(0);
    const ScQueryEntry::Item& rItem = rEntry.GetQueryItem();
    const bool bSingleQueryItem = rEntry.GetQueryItems().size() == 1;
    int count = 0;
    ScQueryEvaluator queryEvaluator(rDoc, *rDoc.maTabs[nTab], maParam, &mrContext);

    ScColumn* pCol = &(rDoc.maTabs[nTab])->aCol[nCol];
    while (true)
    {
        bool bNextColumn = maCurPos.first == pCol->maCells.end();
        if (!bNextColumn)
        {
            if (nRow > maParam.nRow2)
                bNextColumn = true;
        }

        if (bNextColumn)
        {
            do
            {
                ++nCol;
                if (nCol > maParam.nCol2 || nCol >= rDoc.maTabs[nTab]->GetAllocatedColumnsCount())
                    return count; // Over and out
                AdvanceQueryParamEntryField();
                pCol = &(rDoc.maTabs[nTab])->aCol[nCol];
            }
            while (!rItem.mbMatchEmpty && pCol->IsEmptyData());

            InitPos();
        }

        if (maCurPos.first->type == sc::element_type_empty)
        {
            if (rItem.mbMatchEmpty && bSingleQueryItem)
            {
                // This shortcut, instead of determining if any SC_OR query
                // exists or this query is SC_AND'ed (which wouldn't make
                // sense, but..) and evaluating them in ValidQuery(), is
                // possible only because the interpreter is the only caller
                // that sets mbMatchEmpty and there is only one item in those
                // cases.
                // XXX this would have to be reworked if other filters used it
                // in different manners and evaluation would have to be done in
                // ValidQuery().
                count++;
                IncPos();
                continue;
            }
            else
            {
                IncBlock();
                continue;
            }
        }

        ScRefCellValue aCell = sc::toRefCell(maCurPos.first, maCurPos.second);

        if ( queryEvaluator.ValidQuery( nRow,
                (nCol == static_cast<SCCOL>(rEntry.nField) ? &aCell : nullptr)))
        {
            if (aCell.isEmpty())
                return count;
            count++;
            IncPos();
            continue;
        }
        else
            IncPos();
    }
    return count;
}

void ScCountIfCellIterator::AdvanceQueryParamEntryField()
{
    SCSIZE nEntries = maParam.GetEntryCount();
    for ( SCSIZE j = 0; j < nEntries; j++  )
    {
        ScQueryEntry& rEntry = maParam.GetEntry( j );
        if ( rEntry.bDoQuery )
        {
            if ( rEntry.nField < rDoc.MaxCol() )
                rEntry.nField++;
            else
            {
                OSL_FAIL( "AdvanceQueryParamEntryField: ++rEntry.nField > MAXCOL" );
            }
        }
        else
            break;  // for
    }
}

namespace {

/**
 * This class sequentially indexes non-empty cells in order, from the top of
 * the block where the start row position is, to the bottom of the block
 * where the end row position is.  It skips all empty blocks that may be
 * present in between.
 *
 * The index value is an offset from the first element of the first block
 * disregarding all empty cell blocks.
 */
class NonEmptyCellIndexer
{
    typedef std::map<size_t, sc::CellStoreType::const_iterator> BlockMapType;

    BlockMapType maBlockMap;

    const sc::CellStoreType& mrCells;

    size_t mnLowIndex;
    size_t mnHighIndex;

    bool mbValid;

public:

    typedef std::pair<ScRefCellValue, SCROW> CellType;

    /**
     * @param rCells cell storage container
     * @param nStartRow logical start row position
     * @param nEndRow logical end row position, inclusive.
     * @param bSkipTopStrBlock when true, skip all leading string cells.
     */
    NonEmptyCellIndexer(
        const sc::CellStoreType& rCells, SCROW nStartRow, SCROW nEndRow, bool bSkipTopStrBlock ) :
        mrCells(rCells), mnLowIndex(0), mnHighIndex(0), mbValid(true)
    {
        if (nEndRow < nStartRow)
        {
            mbValid = false;
            return;
        }

        // Find the low position.

        sc::CellStoreType::const_position_type aLoPos = mrCells.position(nStartRow);
        if (aLoPos.first->type == sc::element_type_empty)
            incBlock(aLoPos);

        if (aLoPos.first == rCells.end())
        {
            mbValid = false;
            return;
        }

        if (bSkipTopStrBlock)
        {
            // Skip all leading string or empty blocks.
            while (aLoPos.first->type == sc::element_type_string ||
                   aLoPos.first->type == sc::element_type_edittext ||
                   aLoPos.first->type == sc::element_type_empty)
            {
                incBlock(aLoPos);
                if (aLoPos.first == rCells.end())
                {
                    mbValid = false;
                    return;
                }
            }
        }

        SCROW nFirstRow = aLoPos.first->position;
        SCROW nLastRow = aLoPos.first->position + aLoPos.first->size - 1;

        if (nFirstRow > nEndRow)
        {
            // Both start and end row positions are within the leading skipped
            // blocks.
            mbValid = false;
            return;
        }

        // Calculate the index of the low position.
        if (nFirstRow < nStartRow)
            mnLowIndex = nStartRow - nFirstRow;
        else
        {
            // Start row is within the skipped block(s). Set it to the first
            // element of the low block.
            mnLowIndex = 0;
        }

        if (nEndRow < nLastRow)
        {
            assert(nEndRow >= nFirstRow);
            mnHighIndex = nEndRow - nFirstRow;

            maBlockMap.emplace(aLoPos.first->size, aLoPos.first);
            return;
        }

        // Find the high position.

        sc::CellStoreType::const_position_type aHiPos = mrCells.position(aLoPos.first, nEndRow);
        if (aHiPos.first->type == sc::element_type_empty)
        {
            // Move to the last position of the previous block.
            decBlock(aHiPos);

            // Check the row position of the end of the previous block, and make sure it's valid.
            SCROW nBlockEndRow = aHiPos.first->position + aHiPos.first->size - 1;
            if (nBlockEndRow < nStartRow)
            {
                mbValid = false;
                return;
            }
        }

        // Tag the start and end blocks, and all blocks in between in order
        // but skip all empty blocks.

        size_t nPos = 0;
        sc::CellStoreType::const_iterator itBlk = aLoPos.first;
        while (itBlk != aHiPos.first)
        {
            if (itBlk->type == sc::element_type_empty)
            {
                ++itBlk;
                continue;
            }

            nPos += itBlk->size;
            maBlockMap.emplace(nPos, itBlk);
            ++itBlk;

            if (itBlk->type == sc::element_type_empty)
                ++itBlk;

            assert(itBlk != mrCells.end());
        }

        assert(itBlk == aHiPos.first);
        nPos += itBlk->size;
        maBlockMap.emplace(nPos, itBlk);

        // Calculate the high index.
        BlockMapType::const_reverse_iterator ri = maBlockMap.rbegin();
        mnHighIndex = ri->first;
        mnHighIndex -= ri->second->size;
        mnHighIndex += aHiPos.second;
    }

    sc::CellStoreType::const_position_type getPosition( size_t nIndex ) const
    {
        assert(mbValid);
        assert(mnLowIndex <= nIndex);
        assert(nIndex <= mnHighIndex);

        sc::CellStoreType::const_position_type aRet(mrCells.end(), 0);

        BlockMapType::const_iterator it = maBlockMap.upper_bound(nIndex);
        if (it == maBlockMap.end())
            return aRet;

        sc::CellStoreType::const_iterator itBlk = it->second;
        size_t nBlkIndex = it->first - itBlk->size; // index of the first element of the block.
        assert(nBlkIndex <= nIndex);
        assert(nIndex < it->first);

        size_t nOffset = nIndex - nBlkIndex;
        aRet.first = itBlk;
        aRet.second = nOffset;
        return aRet;
    }

    CellType getCell( size_t nIndex ) const
    {
        std::pair<ScRefCellValue, SCROW> aRet;
        aRet.second = -1;

        sc::CellStoreType::const_position_type aPos = getPosition(nIndex);
        if (aPos.first == mrCells.end())
            return aRet;

        aRet.first = sc::toRefCell(aPos.first, aPos.second);
        aRet.second = aPos.first->position + aPos.second;
        return aRet;
    }

    size_t getLowIndex() const { return mnLowIndex; }

    size_t getHighIndex() const { return mnHighIndex; }

    bool isValid() const { return mbValid; }
};

}

bool ScQueryCellIterator::BinarySearch()
{
    // TODO: This will be extremely slow with mdds::multi_type_vector.

    assert(nTab < rDoc.GetTableCount() && "index out of bounds, FIX IT");
    nCol = maParam.nCol1;

    if (nCol >= rDoc.maTabs[nTab]->GetAllocatedColumnsCount())
        return false;

    ScColumn* pCol = &(rDoc.maTabs[nTab])->aCol[nCol];
    if (pCol->IsEmptyData())
        return false;

    CollatorWrapper& rCollator = ScGlobal::GetCollator(maParam.bCaseSens);
    SvNumberFormatter& rFormatter = *(mrContext.GetFormatTable());
    const ScQueryEntry& rEntry = maParam.GetEntry(0);
    const ScQueryEntry::Item& rItem = rEntry.GetQueryItem();
    bool bLessEqual = rEntry.eOp == SC_LESS_EQUAL;
    bool bByString = rItem.meType == ScQueryEntry::ByString;
    bool bAllStringIgnore = bIgnoreMismatchOnLeadingStrings && !bByString;
    bool bFirstStringIgnore = bIgnoreMismatchOnLeadingStrings &&
        !maParam.bHasHeader && bByString;

    nRow = maParam.nRow1;
    if (maParam.bHasHeader)
        ++nRow;

    ScRefCellValue aCell;
    if (bFirstStringIgnore)
    {
        sc::CellStoreType::const_position_type aPos = pCol->maCells.position(nRow);
        if (aPos.first->type == sc::element_type_string || aPos.first->type == sc::element_type_edittext)
        {
            aCell = sc::toRefCell(aPos.first, aPos.second);
            sal_uInt32 nFormat = pCol->GetNumberFormat(mrContext, nRow);
            OUString aCellStr = ScCellFormat::GetInputString(aCell, nFormat, rFormatter, rDoc);
            sal_Int32 nTmp = rCollator.compareString(aCellStr, rEntry.GetQueryItem().maString.getString());
            if ((rEntry.eOp == SC_LESS_EQUAL && nTmp > 0) ||
                    (rEntry.eOp == SC_GREATER_EQUAL && nTmp < 0) ||
                    (rEntry.eOp == SC_EQUAL && nTmp != 0))
                ++nRow;
        }
    }

    NonEmptyCellIndexer aIndexer(pCol->maCells, nRow, maParam.nRow2, bAllStringIgnore);
    if (!aIndexer.isValid())
        return false;

    size_t nLo = aIndexer.getLowIndex();
    size_t nHi = aIndexer.getHighIndex();
    NonEmptyCellIndexer::CellType aCellData;

    // Bookkeeping values for breaking up the binary search in case the data
    // range isn't strictly sorted.
    size_t nLastInRange = nLo;
    size_t nFirstLastInRange = nLastInRange;
    double fLastInRangeValue = bLessEqual ?
        -(::std::numeric_limits<double>::max()) :
            ::std::numeric_limits<double>::max();
    OUString aLastInRangeString;
    if (!bLessEqual)
        aLastInRangeString = OUString(u'\xFFFF');

    aCellData = aIndexer.getCell(nLastInRange);
    aCell = aCellData.first;
    if (aCell.hasString())
    {
        sal_uInt32 nFormat = pCol->GetNumberFormat(mrContext, aCellData.second);
        OUString aStr = ScCellFormat::GetInputString(aCell, nFormat, rFormatter, rDoc);
        aLastInRangeString = aStr;
    }
    else
    {
        switch (aCell.meType)
        {
            case CELLTYPE_VALUE :
                fLastInRangeValue = aCell.mfValue;
            break;
            case CELLTYPE_FORMULA :
                fLastInRangeValue = aCell.mpFormula->GetValue();
            break;
            default:
            {
                // added to avoid warnings
            }
        }
    }

    sal_Int32 nRes = 0;
    bool bFound = false;
    bool bDone = false;
    while (nLo <= nHi && !bDone)
    {
        size_t nMid = (nLo+nHi)/2;
        size_t i = nMid;

        aCellData = aIndexer.getCell(i);
        aCell = aCellData.first;
        bool bStr = aCell.hasString();
        nRes = 0;

        // compares are content<query:-1, content>query:1
        // Cell value comparison similar to ScTable::ValidQuery()
        if (!bStr && !bByString)
        {
            double nCellVal;
            switch (aCell.meType)
            {
                case CELLTYPE_VALUE :
                case CELLTYPE_FORMULA :
                    nCellVal = aCell.getValue();
                break;
                default:
                    nCellVal = 0.0;
            }
            if ((nCellVal < rItem.mfVal) && !::rtl::math::approxEqual(
                        nCellVal, rItem.mfVal))
            {
                nRes = -1;
                if (bLessEqual)
                {
                    if (fLastInRangeValue < nCellVal)
                    {
                        fLastInRangeValue = nCellVal;
                        nLastInRange = i;
                    }
                    else if (fLastInRangeValue > nCellVal)
                    {
                        // not strictly sorted, continue with GetThis()
                        nLastInRange = nFirstLastInRange;
                        bDone = true;
                    }
                }
            }
            else if ((nCellVal > rItem.mfVal) && !::rtl::math::approxEqual(
                        nCellVal, rItem.mfVal))
            {
                nRes = 1;
                if (!bLessEqual)
                {
                    if (fLastInRangeValue > nCellVal)
                    {
                        fLastInRangeValue = nCellVal;
                        nLastInRange = i;
                    }
                    else if (fLastInRangeValue < nCellVal)
                    {
                        // not strictly sorted, continue with GetThis()
                        nLastInRange = nFirstLastInRange;
                        bDone = true;
                    }
                }
            }
        }
        else if (bStr && bByString)
        {
            sal_uInt32 nFormat = pCol->GetNumberFormat(mrContext, aCellData.second);
            OUString aCellStr = ScCellFormat::GetInputString(aCell, nFormat, rFormatter, rDoc);

            nRes = rCollator.compareString(aCellStr, rEntry.GetQueryItem().maString.getString());
            if (nRes < 0 && bLessEqual)
            {
                sal_Int32 nTmp = rCollator.compareString( aLastInRangeString,
                        aCellStr);
                if (nTmp < 0)
                {
                    aLastInRangeString = aCellStr;
                    nLastInRange = i;
                }
                else if (nTmp > 0)
                {
                    // not strictly sorted, continue with GetThis()
                    nLastInRange = nFirstLastInRange;
                    bDone = true;
                }
            }
            else if (nRes > 0 && !bLessEqual)
            {
                sal_Int32 nTmp = rCollator.compareString( aLastInRangeString,
                        aCellStr);
                if (nTmp > 0)
                {
                    aLastInRangeString = aCellStr;
                    nLastInRange = i;
                }
                else if (nTmp < 0)
                {
                    // not strictly sorted, continue with GetThis()
                    nLastInRange = nFirstLastInRange;
                    bDone = true;
                }
            }
        }
        else if (!bStr && bByString)
        {
            nRes = -1; // numeric < string
            if (bLessEqual)
                nLastInRange = i;
        }
        else // if (bStr && !bByString)
        {
            nRes = 1; // string > numeric
            if (!bLessEqual)
                nLastInRange = i;
        }
        if (nRes < 0)
        {
            if (bLessEqual)
                nLo = nMid + 1;
            else // assumed to be SC_GREATER_EQUAL
            {
                if (nMid > 0)
                    nHi = nMid - 1;
                else
                    bDone = true;
            }
        }
        else if (nRes > 0)
        {
            if (bLessEqual)
            {
                if (nMid > 0)
                    nHi = nMid - 1;
                else
                    bDone = true;
            }
            else // assumed to be SC_GREATER_EQUAL
                nLo = nMid + 1;
        }
        else
        {
            nLo = i;
            bDone = bFound = true;
        }
    }

    if (!bFound)
    {
        // If all hits didn't result in a moving limit there's something
        // strange, e.g. data range not properly sorted, or only identical
        // values encountered, which doesn't mean there aren't any others in
        // between... leave it to GetThis(). The condition for this would be
        // if (nLastInRange == nFirstLastInRange) nLo = nFirstLastInRange;
        // Else, in case no exact match was found, we step back for a
        // subsequent GetThis() to find the last in range. Effectively this is
        // --nLo with nLastInRange == nLo-1. Both conditions combined yield:
        nLo = nLastInRange;
    }

    aCellData = aIndexer.getCell(nLo);
    if (nLo <= nHi && aCellData.second <= maParam.nRow2)
    {
        nRow = aCellData.second;
        maCurPos = aIndexer.getPosition(nLo);
        return true;
    }
    else
    {
        nRow = maParam.nRow2 + 1;
        // Set current position to the last possible row.
        maCurPos.first = pCol->maCells.end();
        --maCurPos.first;
        maCurPos.second = maCurPos.first->size - 1;
        return false;
    }
}

ScHorizontalCellIterator::ScHorizontalCellIterator(ScDocument& rDocument, SCTAB nTable,
                                    SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2 ) :
    rDoc( rDocument ),
    mnTab( nTable ),
    nStartCol( nCol1 ),
    nEndCol( nCol2 ),
    nStartRow( nRow1 ),
    nEndRow( nRow2 ),
    mnCol( nCol1 ),
    mnRow( nRow1 ),
    mbMore( false )
{
    assert(mnTab < rDoc.GetTableCount() && "index out of bounds, FIX IT");

    nEndCol = rDoc.maTabs[mnTab]->ClampToAllocatedColumns(nEndCol);
    if (nEndCol < nStartCol) // E.g., somewhere completely outside allocated area
        nEndCol = nStartCol - 1; // Empty

    maColPositions.reserve( nEndCol-nStartCol+1 );

    SetTab( mnTab );
}

ScHorizontalCellIterator::~ScHorizontalCellIterator()
{
}

void ScHorizontalCellIterator::SetTab( SCTAB nTabP )
{
    mbMore = false;
    mnTab = nTabP;
    mnRow = nStartRow;
    mnCol = nStartCol;
    maColPositions.resize(0);

    // Set the start position in each column.
    for (SCCOL i = nStartCol; i <= nEndCol; ++i)
    {
        ScColumn* pCol = &rDoc.maTabs[mnTab]->aCol[i];
        ColParam aParam;
        aParam.maPos = pCol->maCells.position(nStartRow).first;
        aParam.maEnd = pCol->maCells.end();
        aParam.mnCol = i;

        // find first non-empty element.
        while (aParam.maPos != aParam.maEnd) {
            if (aParam.maPos->type == sc::element_type_empty)
                ++aParam.maPos;
            else
            {
                maColPositions.push_back( aParam );
                break;
            }
        }
    }

    if (maColPositions.empty())
        return;

    maColPos = maColPositions.begin();
    mbMore = true;
    SkipInvalid();
}

ScRefCellValue* ScHorizontalCellIterator::GetNext( SCCOL& rCol, SCROW& rRow )
{
    if (!mbMore)
    {
        debugiter("no more !\n");
        return nullptr;
    }

    // Return the current non-empty cell, and move the cursor to the next one.
    ColParam& r = *maColPos;

    rCol = mnCol = r.mnCol;
    rRow = mnRow;
    debugiter("return col %d row %d\n", (int)rCol, (int)rRow);

    size_t nOffset = static_cast<size_t>(mnRow) - r.maPos->position;
    maCurCell = sc::toRefCell(r.maPos, nOffset);
    Advance();
    debugiter("advance to: col %d row %d\n", (int)maColPos->mnCol, (int)mnRow);

    return &maCurCell;
}

bool ScHorizontalCellIterator::GetPos( SCCOL& rCol, SCROW& rRow )
{
    rCol = mnCol;
    rRow = mnRow;
    return mbMore;
}

// Skip any invalid / empty cells across the current row,
// we only advance the cursor if the current entry is invalid.
// if we return true we have a valid cursor (or hit the end)
bool ScHorizontalCellIterator::SkipInvalidInRow()
{
    assert (mbMore);
    assert (maColPos != maColPositions.end());

    // Find the next non-empty cell in the current row.
    while( maColPos != maColPositions.end() )
    {
        ColParam& r = *maColPos;
        assert (r.maPos != r.maEnd);

        size_t nRow = static_cast<size_t>(mnRow);

        if (nRow >= r.maPos->position)
        {
            if (nRow < r.maPos->position + r.maPos->size)
            {
                mnCol = maColPos->mnCol;
                debugiter("found valid cell at column %d, row %d\n",
                          (int)mnCol, (int)mnRow);
                assert(r.maPos->type != sc::element_type_empty);
                return true;
            }
            else
            {
                bool bMoreBlocksInColumn = false;
                // This block is behind the current row position. Advance the block.
                for (++r.maPos; r.maPos != r.maEnd; ++r.maPos)
                {
                    if (nRow < r.maPos->position + r.maPos->size &&
                        r.maPos->type != sc::element_type_empty)
                    {
                        bMoreBlocksInColumn = true;
                        break;
                    }
                }
                if (!bMoreBlocksInColumn)
                {
                    debugiter("remove column %d at row %d\n",
                              (int)maColPos->mnCol, (int)nRow);
                    maColPos = maColPositions.erase(maColPos);
                    if (maColPositions.empty())
                    {
                        debugiter("no more columns\n");
                        mbMore = false;
                    }
                }
                else
                {
                    debugiter("advanced column %d to block starting row %d, retrying\n",
                              (int)maColPos->mnCol, r.maPos->position);
                }
            }
        }
        else
        {
            debugiter("skip empty cells at column %d, row %d\n",
                      (int)maColPos->mnCol, (int)nRow);
            ++maColPos;
        }
    }

    // No more columns with anything interesting in them ?
    if (maColPositions.empty())
    {
        debugiter("no more live columns left - done\n");
        mbMore = false;
        return true;
    }

    return false;
}

/// Find the next row that has some real content in one of its columns.
SCROW ScHorizontalCellIterator::FindNextNonEmptyRow()
{
    size_t nNextRow = rDoc.MaxRow()+1;

    for (const ColParam& r : maColPositions)
    {
        assert(o3tl::make_unsigned(mnRow) <= r.maPos->position);
        nNextRow = std::min (nNextRow, static_cast<size_t>(r.maPos->position));
    }

    SCROW nRow = std::max(static_cast<SCROW>(nNextRow), mnRow);
    debugiter("Next non empty row is %d\n", (int) nRow);
    return nRow;
}

void ScHorizontalCellIterator::Advance()
{
    assert (mbMore);
    assert (maColPos != maColPositions.end());

    ++maColPos;

    SkipInvalid();
}

void ScHorizontalCellIterator::SkipInvalid()
{
    if (maColPos == maColPositions.end() ||
        !SkipInvalidInRow())
    {
        mnRow++;

        if (mnRow > nEndRow)
        {
            mbMore = false;
            return;
        }

        maColPos = maColPositions.begin();
        debugiter("moving to next row\n");
        if (SkipInvalidInRow())
        {
            debugiter("moved to valid cell in next row (or end)\n");
            return;
        }

        mnRow = FindNextNonEmptyRow();
        maColPos = maColPositions.begin();
        bool bCorrect = SkipInvalidInRow();
        assert (bCorrect); (void) bCorrect;
    }

    if (mnRow > nEndRow)
        mbMore = false;
}

ScHorizontalValueIterator::ScHorizontalValueIterator( ScDocument& rDocument,
        const ScRange& rRange ) :
    rDoc( rDocument ),
    nEndTab( rRange.aEnd.Tab() ),
    bCalcAsShown( rDocument.GetDocOptions().IsCalcAsShown() )
{
    SCCOL nStartCol = rRange.aStart.Col();
    SCROW nStartRow = rRange.aStart.Row();
    SCTAB nStartTab = rRange.aStart.Tab();
    SCCOL nEndCol = rRange.aEnd.Col();
    SCROW nEndRow = rRange.aEnd.Row();
    PutInOrder( nStartCol, nEndCol);
    PutInOrder( nStartRow, nEndRow);
    PutInOrder( nStartTab, nEndTab );

    if (!rDoc.ValidCol(nStartCol)) nStartCol = rDoc.MaxCol();
    if (!rDoc.ValidCol(nEndCol)) nEndCol = rDoc.MaxCol();
    if (!rDoc.ValidRow(nStartRow)) nStartRow = rDoc.MaxRow();
    if (!rDoc.ValidRow(nEndRow)) nEndRow = rDoc.MaxRow();
    if (!ValidTab(nStartTab)) nStartTab = MAXTAB;
    if (!ValidTab(nEndTab)) nEndTab = MAXTAB;

    nCurCol = nStartCol;
    nCurRow = nStartRow;
    nCurTab = nStartTab;

    nNumFormat = 0; // Will be initialized in GetNumberFormat()
    pAttrArray = nullptr;
    nAttrEndRow = 0;

    pCellIter.reset( new ScHorizontalCellIterator( rDoc, nStartTab, nStartCol,
            nStartRow, nEndCol, nEndRow ) );
}

ScHorizontalValueIterator::~ScHorizontalValueIterator()
{
}

bool ScHorizontalValueIterator::GetNext( double& rValue, FormulaError& rErr )
{
    bool bFound = false;
    while ( !bFound )
    {
        ScRefCellValue* pCell = pCellIter->GetNext( nCurCol, nCurRow );
        while ( !pCell )
        {
            if ( nCurTab < nEndTab )
            {
                pCellIter->SetTab( ++nCurTab);
                pCell = pCellIter->GetNext( nCurCol, nCurRow );
            }
            else
                return false;
        }
        switch (pCell->meType)
        {
            case CELLTYPE_VALUE:
                {
                    rValue = pCell->mfValue;
                    rErr = FormulaError::NONE;
                    if ( bCalcAsShown )
                    {
                        ScColumn* pCol = &rDoc.maTabs[nCurTab]->aCol[nCurCol];
                        ScAttrArray_IterGetNumberFormat( nNumFormat, pAttrArray,
                                nAttrEndRow, pCol->pAttrArray.get(), nCurRow, rDoc );
                        rValue = rDoc.RoundValueAsShown( rValue, nNumFormat );
                    }
                    bFound = true;
                }
                break;
            case CELLTYPE_FORMULA:
                {
                    rErr = pCell->mpFormula->GetErrCode();
                    if (rErr != FormulaError::NONE || pCell->mpFormula->IsValue())
                    {
                        rValue = pCell->mpFormula->GetValue();
                        bFound = true;
                    }
                }
                break;
            case CELLTYPE_STRING :
            case CELLTYPE_EDIT :
                break;
            default: ;   // nothing
        }
    }
    return bFound;
}

ScHorizontalAttrIterator::ScHorizontalAttrIterator( ScDocument& rDocument, SCTAB nTable,
                            SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2 ) :
    rDoc( rDocument ),
    nTab( nTable ),
    nStartCol( nCol1 ),
    nStartRow( nRow1 ),
    nEndCol( nCol2 ),
    nEndRow( nRow2 )
{
    assert(nTab < rDoc.GetTableCount() && "index out of bounds, FIX IT");
    assert(rDoc.maTabs[nTab]);

    nEndCol = rDoc.maTabs[nTab]->ClampToAllocatedColumns(nEndCol);

    nRow = nStartRow;
    nCol = nStartCol;
    bRowEmpty = false;

    pIndices.reset( new SCSIZE[nEndCol-nStartCol+1] );
    pNextEnd.reset( new SCROW[nEndCol-nStartCol+1] );
    pHorizEnd.reset( new SCCOL[nEndCol-nStartCol+1] );
    ppPatterns.reset( new const ScPatternAttr*[nEndCol-nStartCol+1] );

    InitForNextRow(true);
}

ScHorizontalAttrIterator::~ScHorizontalAttrIterator()
{
}

void ScHorizontalAttrIterator::InitForNextRow(bool bInitialization)
{
    bool bEmpty = true;
    nMinNextEnd = rDoc.MaxRow();
    SCCOL nThisHead = 0;

    for (SCCOL i=nStartCol; i<=nEndCol; i++)
    {
        SCCOL nPos = i - nStartCol;
        if ( bInitialization || pNextEnd[nPos] < nRow )
        {
            const ScAttrArray* pArray = rDoc.maTabs[nTab]->aCol[i].pAttrArray.get();
            assert(pArray);

            SCSIZE nIndex;
            if (bInitialization)
            {
                if ( pArray->Count() )
                    pArray->Search( nStartRow, nIndex );
                else
                    nIndex = 0;
                pIndices[nPos] = nIndex;
                pHorizEnd[nPos] = rDoc.MaxCol()+1; // only for assert()
            }
            else
                nIndex = ++pIndices[nPos];

            if ( !nIndex && !pArray->Count() )
            {
                pNextEnd[nPos] = rDoc.MaxRow();
                assert( pNextEnd[nPos] >= nRow && "Sequence out of order" );
                ppPatterns[nPos] = nullptr;
            }
            else if ( nIndex < pArray->Count() )
            {
                const ScPatternAttr* pPattern = pArray->mvData[nIndex].pPattern;
                SCROW nThisEnd = pArray->mvData[nIndex].nEndRow;

                if ( IsDefaultItem( pPattern ) )
                    pPattern = nullptr;
                else
                    bEmpty = false; // Found attributes

                pNextEnd[nPos] = nThisEnd;
                assert( pNextEnd[nPos] >= nRow && "Sequence out of order" );
                ppPatterns[nPos] = pPattern;
            }
            else
            {
                assert(!"AttrArray does not range to MAXROW");
                pNextEnd[nPos] = rDoc.MaxRow();
                ppPatterns[nPos] = nullptr;
            }
        }
        else if ( ppPatterns[nPos] )
            bEmpty = false; // Area not at the end yet

        if ( nMinNextEnd > pNextEnd[nPos] )
            nMinNextEnd = pNextEnd[nPos];

        // store positions of ScHorizontalAttrIterator elements (minimizing expensive ScPatternAttr comparisons)
        if (i > nStartCol && ppPatterns[nThisHead] != ppPatterns[nPos])
        {
           pHorizEnd[nThisHead] = i - 1;
           nThisHead = nPos; // start position of the next horizontal group
        }
    }

    if (bEmpty)
        nRow = nMinNextEnd; // Skip until end of next section
    else
        pHorizEnd[nThisHead] = nEndCol; // set the end position of the last horizontal group, too
    bRowEmpty = bEmpty;
}

bool ScHorizontalAttrIterator::InitForNextAttr()
{
    if ( !ppPatterns[nCol-nStartCol] ) // Skip default items
    {
        assert( pHorizEnd[nCol-nStartCol] < rDoc.MaxCol()+1 && "missing stored data" );
        nCol = pHorizEnd[nCol-nStartCol] + 1;
        if ( nCol > nEndCol )
            return false;
    }

    return true;
}

const ScPatternAttr* ScHorizontalAttrIterator::GetNext( SCCOL& rCol1, SCCOL& rCol2, SCROW& rRow )
{
    assert(nTab < rDoc.GetTableCount() && "index out of bounds, FIX IT");
    for (;;)
    {
        if ( !bRowEmpty && nCol <= nEndCol && InitForNextAttr() )
        {
            const ScPatternAttr* pPat = ppPatterns[nCol-nStartCol];
            rRow = nRow;
            rCol1 = nCol;
            assert( pHorizEnd[nCol-nStartCol] < rDoc.MaxCol()+1 && "missing stored data" );
            nCol = pHorizEnd[nCol-nStartCol];
            rCol2 = nCol;
            ++nCol; // Count up for next call
            return pPat; // Found it!
        }

        // Next row
        ++nRow;
        if ( nRow > nEndRow ) // Already at the end?
            return nullptr; // Found nothing
        nCol = nStartCol; // Start at the left again

        if ( bRowEmpty || nRow > nMinNextEnd )
            InitForNextRow(false);
    }
}

static bool IsGreater( SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2 )
{
    return ( nRow1 > nRow2 ) || ( nRow1 == nRow2 && nCol1 > nCol2 );
}

ScUsedAreaIterator::ScUsedAreaIterator( ScDocument& rDocument, SCTAB nTable,
                            SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2 )
    : aCellIter( rDocument, nTable, nCol1, nRow1, nCol2, nRow2 )
    , aAttrIter( rDocument, nTable, nCol1, nRow1, nCol2, nRow2 )
    , nNextCol( nCol1 )
    , nNextRow( nRow1 )
    , nCellCol( 0 )
    , nCellRow( 0 )
    , nAttrCol1( 0 )
    , nAttrCol2( 0 )
    , nAttrRow( 0 )
    , nFoundStartCol( 0 )
    , nFoundEndCol( 0 )
    , nFoundRow( 0 )
    , pFoundPattern( nullptr )
{
    pCell    = aCellIter.GetNext( nCellCol, nCellRow );
    pPattern = aAttrIter.GetNext( nAttrCol1, nAttrCol2, nAttrRow );
}

ScUsedAreaIterator::~ScUsedAreaIterator()
{
}

bool ScUsedAreaIterator::GetNext()
{
    //  Forward iterators
    if ( pCell && IsGreater( nNextCol, nNextRow, nCellCol, nCellRow ) )
        pCell = aCellIter.GetNext( nCellCol, nCellRow );

    while (pCell && pCell->isEmpty())
        pCell = aCellIter.GetNext( nCellCol, nCellRow );

    if ( pPattern && IsGreater( nNextCol, nNextRow, nAttrCol2, nAttrRow ) )
        pPattern = aAttrIter.GetNext( nAttrCol1, nAttrCol2, nAttrRow );

    if ( pPattern && nAttrRow == nNextRow && nAttrCol1 < nNextCol )
        nAttrCol1 = nNextCol;

    // Find next area
    bool bFound = true;
    bool bUseCell = false;

    if ( pCell && pPattern )
    {
        if ( IsGreater( nCellCol, nCellRow, nAttrCol1, nAttrRow ) ) // Only attributes at the beginning?
        {
            maFoundCell.clear();
            pFoundPattern = pPattern;
            nFoundRow = nAttrRow;
            nFoundStartCol = nAttrCol1;
            if ( nCellRow == nAttrRow && nCellCol <= nAttrCol2 ) // Area also contains cell?
                nFoundEndCol = nCellCol - 1; // Only until right before the cell
            else
                nFoundEndCol = nAttrCol2; // Everything
        }
        else
        {
            bUseCell = true;
            if ( nAttrRow == nCellRow && nAttrCol1 == nCellCol ) // Attributes on the cell?
                pFoundPattern = pPattern;
            else
                pFoundPattern = nullptr;
        }
    }
    else if ( pCell ) // Just a cell -> take over right away
    {
        pFoundPattern = nullptr;
        bUseCell = true; // Cell position
    }
    else if ( pPattern ) // Just attributes -> take over right away
    {
        maFoundCell.clear();
        pFoundPattern = pPattern;
        nFoundRow = nAttrRow;
        nFoundStartCol = nAttrCol1;
        nFoundEndCol = nAttrCol2;
    }
    else // Nothing
        bFound = false;

    if ( bUseCell ) // Cell position
    {
        if (pCell)
            maFoundCell = *pCell;
        else
            maFoundCell.clear();

        nFoundRow = nCellRow;
        nFoundStartCol = nFoundEndCol = nCellCol;
    }

    if (bFound)
    {
        nNextRow = nFoundRow;
        nNextCol = nFoundEndCol + 1;
    }

    return bFound;
}

ScDocAttrIterator::ScDocAttrIterator(ScDocument& rDocument, SCTAB nTable,
                                    SCCOL nCol1, SCROW nRow1,
                                    SCCOL nCol2, SCROW nRow2) :
    rDoc( rDocument ),
    nTab( nTable ),
    nEndCol( nCol2 ),
    nStartRow( nRow1 ),
    nEndRow( nRow2 ),
    nCol( nCol1 )
{
    if ( ValidTab(nTab) && nTab < rDoc.GetTableCount() && rDoc.maTabs[nTab]
        && nCol < rDoc.maTabs[nTab]->GetAllocatedColumnsCount())
    {
        nEndCol = rDoc.maTabs[nTab]->ClampToAllocatedColumns(nEndCol);
        pColIter = rDoc.maTabs[nTab]->aCol[nCol].CreateAttrIterator( nStartRow, nEndRow );
    }
}

ScDocAttrIterator::~ScDocAttrIterator()
{
}

const ScPatternAttr* ScDocAttrIterator::GetNext( SCCOL& rCol, SCROW& rRow1, SCROW& rRow2 )
{
    while ( pColIter )
    {
        const ScPatternAttr* pPattern = pColIter->Next( rRow1, rRow2 );
        if ( pPattern )
        {
            rCol = nCol;
            return pPattern;
        }

        ++nCol;
        if ( nCol <= nEndCol )
            pColIter = rDoc.maTabs[nTab]->aCol[nCol].CreateAttrIterator( nStartRow, nEndRow );
        else
            pColIter.reset();
    }
    return nullptr;  // Nothing anymore
}

ScDocRowHeightUpdater::TabRanges::TabRanges(SCTAB nTab, SCROW nMaxRow) :
    mnTab(nTab), maRanges(nMaxRow)
{
}

ScDocRowHeightUpdater::ScDocRowHeightUpdater(ScDocument& rDoc, OutputDevice* pOutDev, double fPPTX, double fPPTY, const vector<TabRanges>* pTabRangesArray) :
    mrDoc(rDoc), mpOutDev(pOutDev), mfPPTX(fPPTX), mfPPTY(fPPTY), mpTabRangesArray(pTabRangesArray)
{
}

void ScDocRowHeightUpdater::update()
{
    if (!mpTabRangesArray || mpTabRangesArray->empty())
    {
        // No ranges defined. Update all rows in all tables.
        updateAll();
        return;
    }

    sal_uInt64 nCellCount = 0;
    for (const auto& rTabRanges : *mpTabRangesArray)
    {
        const SCTAB nTab = rTabRanges.mnTab;
        if (!ValidTab(nTab) || nTab >= mrDoc.GetTableCount() || !mrDoc.maTabs[nTab])
            continue;

        ScFlatBoolRowSegments::RangeData aData;
        ScFlatBoolRowSegments::RangeIterator aRangeItr(rTabRanges.maRanges);
        for (bool bFound = aRangeItr.getFirst(aData); bFound; bFound = aRangeItr.getNext(aData))
        {
            if (!aData.mbValue)
                continue;

            nCellCount += mrDoc.maTabs[nTab]->GetWeightedCount(aData.mnRow1, aData.mnRow2);
        }
    }

    ScProgress aProgress(mrDoc.GetDocumentShell(), ScResId(STR_PROGRESS_HEIGHTING), nCellCount, true);

    Fraction aZoom(1, 1);
    sal_uInt64 nProgressStart = 0;
    for (const auto& rTabRanges : *mpTabRangesArray)
    {
        const SCTAB nTab = rTabRanges.mnTab;
        if (!ValidTab(nTab) || nTab >= mrDoc.GetTableCount() || !mrDoc.maTabs[nTab])
            continue;

        sc::RowHeightContext aCxt(mrDoc.MaxRow(), mfPPTX, mfPPTY, aZoom, aZoom, mpOutDev);
        ScFlatBoolRowSegments::RangeData aData;
        ScFlatBoolRowSegments::RangeIterator aRangeItr(rTabRanges.maRanges);
        for (bool bFound = aRangeItr.getFirst(aData); bFound; bFound = aRangeItr.getNext(aData))
        {
            if (!aData.mbValue)
                continue;

            mrDoc.maTabs[nTab]->SetOptimalHeight(
                aCxt, aData.mnRow1, aData.mnRow2, true, &aProgress, nProgressStart);

            nProgressStart += mrDoc.maTabs[nTab]->GetWeightedCount(aData.mnRow1, aData.mnRow2);
        }
    }
}

void ScDocRowHeightUpdater::updateAll()
{
    sal_uInt64 nCellCount = 0;
    for (SCTAB nTab = 0; nTab < mrDoc.GetTableCount(); ++nTab)
    {
        if (!ValidTab(nTab) || !mrDoc.maTabs[nTab])
            continue;

        nCellCount += mrDoc.maTabs[nTab]->GetWeightedCount();
    }

    ScProgress aProgress(mrDoc.GetDocumentShell(), ScResId(STR_PROGRESS_HEIGHTING), nCellCount, true);

    Fraction aZoom(1, 1);
    sc::RowHeightContext aCxt(mrDoc.MaxRow(), mfPPTX, mfPPTY, aZoom, aZoom, mpOutDev);
    sal_uInt64 nProgressStart = 0;
    for (SCTAB nTab = 0; nTab < mrDoc.GetTableCount(); ++nTab)
    {
        if (!ValidTab(nTab) || !mrDoc.maTabs[nTab])
            continue;

        mrDoc.maTabs[nTab]->SetOptimalHeight(aCxt, 0, mrDoc.MaxRow(), true, &aProgress, nProgressStart);
        nProgressStart += mrDoc.maTabs[nTab]->GetWeightedCount();
    }
}

ScAttrRectIterator::ScAttrRectIterator(ScDocument& rDocument, SCTAB nTable,
                                    SCCOL nCol1, SCROW nRow1,
                                    SCCOL nCol2, SCROW nRow2) :
    rDoc( rDocument ),
    nTab( nTable ),
    nEndCol( nCol2 ),
    nStartRow( nRow1 ),
    nEndRow( nRow2 ),
    nIterStartCol( nCol1 ),
    nIterEndCol( nCol1 )
{
    if ( ValidTab(nTab) && nTab < rDoc.GetTableCount() && rDoc.maTabs[nTab]
        && nCol1 < rDoc.maTabs[nTab]->GetAllocatedColumnsCount())
    {
        nEndCol = rDoc.maTabs[nTab]->ClampToAllocatedColumns(nEndCol);
        pColIter = rDoc.maTabs[nTab]->aCol[nIterStartCol].CreateAttrIterator( nStartRow, nEndRow );
        while ( nIterEndCol < nEndCol &&
                rDoc.maTabs[nTab]->aCol[nIterEndCol].IsAllAttrEqual(
                    rDoc.maTabs[nTab]->aCol[nIterEndCol+1], nStartRow, nEndRow ) )
            ++nIterEndCol;
    }
    else
        pColIter = nullptr;
}

ScAttrRectIterator::~ScAttrRectIterator()
{
}

void ScAttrRectIterator::DataChanged()
{
    if (pColIter)
    {
        SCROW nNextRow = pColIter->GetNextRow();
        pColIter = rDoc.maTabs[nTab]->aCol[nIterStartCol].CreateAttrIterator( nNextRow, nEndRow );
    }
}

const ScPatternAttr* ScAttrRectIterator::GetNext( SCCOL& rCol1, SCCOL& rCol2,
                                                    SCROW& rRow1, SCROW& rRow2 )
{
    while ( pColIter )
    {
        const ScPatternAttr* pPattern = pColIter->Next( rRow1, rRow2 );
        if ( pPattern )
        {
            rCol1 = nIterStartCol;
            rCol2 = nIterEndCol;
            return pPattern;
        }

        nIterStartCol = nIterEndCol+1;
        if ( nIterStartCol <= nEndCol )
        {
            nIterEndCol = nIterStartCol;
            pColIter = rDoc.maTabs[nTab]->aCol[nIterStartCol].CreateAttrIterator( nStartRow, nEndRow );
            while ( nIterEndCol < nEndCol &&
                    rDoc.maTabs[nTab]->aCol[nIterEndCol].IsAllAttrEqual(
                        rDoc.maTabs[nTab]->aCol[nIterEndCol+1], nStartRow, nEndRow ) )
                ++nIterEndCol;
        }
        else
            pColIter.reset();
    }
    return nullptr; // Nothing anymore
}

ScRowBreakIterator::ScRowBreakIterator(set<SCROW>& rBreaks) :
    mrBreaks(rBreaks),
    maItr(rBreaks.begin()), maEnd(rBreaks.end())
{
}

SCROW ScRowBreakIterator::first()
{
    maItr = mrBreaks.begin();
    return maItr == maEnd ? NOT_FOUND : *maItr;
}

SCROW ScRowBreakIterator::next()
{
    ++maItr;
    return maItr == maEnd ? NOT_FOUND : *maItr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
