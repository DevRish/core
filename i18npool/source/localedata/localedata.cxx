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
#include <mutex>
#include <string_view>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/text/HoriOrientation.hpp>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <localedata.hxx>
#include <i18nlangtag/mslangid.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <sal/macros.h>

namespace com::sun::star::uno { class XComponentContext; }

using namespace com::sun::star::i18n;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star;

typedef sal_Unicode**   (* MyFunc_Type)( sal_Int16&);
typedef sal_Unicode const *** (* MyFunc_Type2)( sal_Int16&, sal_Int16& );
typedef sal_Unicode const **** (* MyFunc_Type3)( sal_Int16&, sal_Int16&, sal_Int16& );
typedef sal_Unicode const * const * (* MyFunc_FormatCode)( sal_Int16&, sal_Unicode const *&, sal_Unicode const *& );

#ifndef DISABLE_DYNLOADING

static const char *lcl_DATA_EN = "localedata_en";
static const char *lcl_DATA_ES = "localedata_es";
static const char *lcl_DATA_EURO = "localedata_euro";
static const char *lcl_DATA_OTHERS = "localedata_others";

const struct {
    const char* pLocale;
    const char* pLib;
} aLibTable[] = {
    { "en_US",  lcl_DATA_EN },
    { "en_AU",  lcl_DATA_EN },
    { "en_BZ",  lcl_DATA_EN },
    { "en_CA",  lcl_DATA_EN },
    { "en_GB",  lcl_DATA_EN },
    { "en_IE",  lcl_DATA_EN },
    { "en_JM",  lcl_DATA_EN },
    { "en_NZ",  lcl_DATA_EN },
    { "en_PH",  lcl_DATA_EN },
    { "en_TT",  lcl_DATA_EN },
    { "en_ZA",  lcl_DATA_EN },
    { "en_ZW",  lcl_DATA_EN },
    { "en_NA",  lcl_DATA_EN },
    { "en_GH",  lcl_DATA_EN },
    { "en_MW",  lcl_DATA_EN },
    { "en_GM",  lcl_DATA_EN },
    { "en_BW",  lcl_DATA_EN },
    { "en_ZM",  lcl_DATA_EN },
    { "en_LK",  lcl_DATA_EN },
    { "en_NG",  lcl_DATA_EN },
    { "en_KE",  lcl_DATA_EN },
    { "en_DK",  lcl_DATA_EN },

    { "es_ES",  lcl_DATA_ES },
    { "es_AR",  lcl_DATA_ES },
    { "es_BO",  lcl_DATA_ES },
    { "es_CL",  lcl_DATA_ES },
    { "es_CO",  lcl_DATA_ES },
    { "es_CR",  lcl_DATA_ES },
    { "es_DO",  lcl_DATA_ES },
    { "es_EC",  lcl_DATA_ES },
    { "es_GT",  lcl_DATA_ES },
    { "es_HN",  lcl_DATA_ES },
    { "es_MX",  lcl_DATA_ES },
    { "es_NI",  lcl_DATA_ES },
    { "es_PA",  lcl_DATA_ES },
    { "es_PE",  lcl_DATA_ES },
    { "es_PR",  lcl_DATA_ES },
    { "es_PY",  lcl_DATA_ES },
    { "es_SV",  lcl_DATA_ES },
    { "es_UY",  lcl_DATA_ES },
    { "es_VE",  lcl_DATA_ES },
    { "gl_ES",  lcl_DATA_ES },
    { "oc_ES",  lcl_DATA_ES },

    { "de_DE",  lcl_DATA_EURO },
    { "de_AT",  lcl_DATA_EURO },
    { "de_CH",  lcl_DATA_EURO },
    { "de_LI",  lcl_DATA_EURO },
    { "de_LU",  lcl_DATA_EURO },
    { "fr_FR",  lcl_DATA_EURO },
    { "fr_BE",  lcl_DATA_EURO },
    { "fr_CA",  lcl_DATA_EURO },
    { "fr_CH",  lcl_DATA_EURO },
    { "fr_LU",  lcl_DATA_EURO },
    { "fr_MC",  lcl_DATA_EURO },
    { "fr_BF",  lcl_DATA_EURO },
    { "fr_CI",  lcl_DATA_EURO },
    { "fr_ML",  lcl_DATA_EURO },
    { "fr_SN",  lcl_DATA_EURO },
    { "fr_BJ",  lcl_DATA_EURO },
    { "fr_NE",  lcl_DATA_EURO },
    { "fr_TG",  lcl_DATA_EURO },
    { "it_IT",  lcl_DATA_EURO },
    { "it_CH",  lcl_DATA_EURO },
    { "sl_SI",  lcl_DATA_EURO },
    { "sv_SE",  lcl_DATA_EURO },
    { "sv_FI",  lcl_DATA_EURO },
    { "ca_ES",  lcl_DATA_EURO },
    { "ca_ES_valencia",  lcl_DATA_EURO },
    { "cs_CZ",  lcl_DATA_EURO },
    { "sk_SK",  lcl_DATA_EURO },
    { "da_DK",  lcl_DATA_EURO },
    { "el_GR",  lcl_DATA_EURO },
    { "fi_FI",  lcl_DATA_EURO },
    { "is_IS",  lcl_DATA_EURO },
    { "nl_BE",  lcl_DATA_EURO },
    { "nl_NL",  lcl_DATA_EURO },
    { "no_NO",  lcl_DATA_EURO },
    { "nn_NO",  lcl_DATA_EURO },
    { "nb_NO",  lcl_DATA_EURO },
    { "nds_DE", lcl_DATA_EURO },
    { "pl_PL",  lcl_DATA_EURO },
    { "pt_BR",  lcl_DATA_EURO },
    { "pt_PT",  lcl_DATA_EURO },
    { "ru_RU",  lcl_DATA_EURO },
    { "tr_TR",  lcl_DATA_EURO },
    { "tt_RU",  lcl_DATA_EURO },
    { "et_EE",  lcl_DATA_EURO },
    { "vro_EE", lcl_DATA_EURO },
    { "lb_LU",  lcl_DATA_EURO },
    { "lt_LT",  lcl_DATA_EURO },
    { "lv_LV",  lcl_DATA_EURO },
    { "uk_UA",  lcl_DATA_EURO },
    { "ro_RO",  lcl_DATA_EURO },
    { "cy_GB",  lcl_DATA_EURO },
    { "bg_BG",  lcl_DATA_EURO },
    { "sr_Latn_ME",  lcl_DATA_EURO },
    { "sr_Latn_RS",  lcl_DATA_EURO },
    { "sr_Latn_CS",  lcl_DATA_EURO },
    { "sr_ME",  lcl_DATA_EURO },
    { "sr_RS",  lcl_DATA_EURO },
    { "sr_CS",  lcl_DATA_EURO },
    { "hr_HR",  lcl_DATA_EURO },
    { "bs_BA",  lcl_DATA_EURO },
    { "eu_ES",  lcl_DATA_EURO },
    { "fo_FO",  lcl_DATA_EURO },
    { "ga_IE",  lcl_DATA_EURO },
    { "gd_GB",  lcl_DATA_EURO },
    { "ka_GE",  lcl_DATA_EURO },
    { "be_BY",  lcl_DATA_EURO },
    { "kl_GL",  lcl_DATA_EURO },
    { "mk_MK",  lcl_DATA_EURO },
    { "br_FR",  lcl_DATA_EURO },
    { "la_VA",  lcl_DATA_EURO },
    { "cv_RU",  lcl_DATA_EURO },
    { "wa_BE",  lcl_DATA_EURO },
    { "fur_IT", lcl_DATA_EURO },
    { "gsc_FR", lcl_DATA_EURO },
    { "fy_NL",  lcl_DATA_EURO },
    { "oc_FR",  lcl_DATA_EURO },
    { "mt_MT",  lcl_DATA_EURO },
    { "sc_IT",  lcl_DATA_EURO },
    { "ast_ES", lcl_DATA_EURO },
    { "ltg_LV", lcl_DATA_EURO },
    { "hsb_DE", lcl_DATA_EURO },
    { "dsb_DE", lcl_DATA_EURO },
    { "rue_SK", lcl_DATA_EURO },
    { "an_ES",  lcl_DATA_EURO },
    { "myv_RU", lcl_DATA_EURO },
    { "lld_IT", lcl_DATA_EURO },
    { "cu_RU",  lcl_DATA_EURO },
    { "vec_IT", lcl_DATA_EURO },
    { "szl_PL", lcl_DATA_EURO },
    { "lij_IT", lcl_DATA_EURO },

    { "ja_JP",  lcl_DATA_OTHERS },
    { "ko_KR",  lcl_DATA_OTHERS },
    { "zh_CN",  lcl_DATA_OTHERS },
    { "zh_HK",  lcl_DATA_OTHERS },
    { "zh_SG",  lcl_DATA_OTHERS },
    { "zh_TW",  lcl_DATA_OTHERS },
    { "zh_MO",  lcl_DATA_OTHERS },
    { "en_HK",  lcl_DATA_OTHERS },  // needs to be in OTHERS instead of EN because currency inherited from zh_HK

    { "ar_EG",  lcl_DATA_OTHERS },
    { "ar_DZ",  lcl_DATA_OTHERS },
    { "ar_LB",  lcl_DATA_OTHERS },
    { "ar_SA",  lcl_DATA_OTHERS },
    { "ar_TN",  lcl_DATA_OTHERS },
    { "he_IL",  lcl_DATA_OTHERS },
    { "hi_IN",  lcl_DATA_OTHERS },
    { "kn_IN",  lcl_DATA_OTHERS },
    { "ta_IN",  lcl_DATA_OTHERS },
    { "te_IN",  lcl_DATA_OTHERS },
    { "gu_IN",  lcl_DATA_OTHERS },
    { "mr_IN",  lcl_DATA_OTHERS },
    { "pa_IN",  lcl_DATA_OTHERS },
    { "bn_IN",  lcl_DATA_OTHERS },
    { "or_IN",  lcl_DATA_OTHERS },
    { "en_IN",  lcl_DATA_OTHERS },  // keep in OTHERS for IN
    { "ml_IN",  lcl_DATA_OTHERS },
    { "bn_BD",  lcl_DATA_OTHERS },
    { "th_TH",  lcl_DATA_OTHERS },

    { "af_ZA",  lcl_DATA_OTHERS },
    { "hu_HU",  lcl_DATA_OTHERS },
    { "id_ID",  lcl_DATA_OTHERS },
    { "ms_MY",  lcl_DATA_OTHERS },
    { "en_MY",  lcl_DATA_OTHERS },  // needs to be in OTHERS instead of EN because currency inherited from ms_MY
    { "ia",     lcl_DATA_OTHERS },
    { "mn_Cyrl_MN",  lcl_DATA_OTHERS },
    { "az_AZ",  lcl_DATA_OTHERS },
    { "sw_TZ",  lcl_DATA_OTHERS },
    { "km_KH",  lcl_DATA_OTHERS },
    { "lo_LA",  lcl_DATA_OTHERS },
    { "rw_RW",  lcl_DATA_OTHERS },
    { "eo",     lcl_DATA_OTHERS },
    { "dz_BT",  lcl_DATA_OTHERS },
    { "ne_NP",  lcl_DATA_OTHERS },
    { "zu_ZA",  lcl_DATA_OTHERS },
    { "nso_ZA", lcl_DATA_OTHERS },
    { "vi_VN",  lcl_DATA_OTHERS },
    { "tn_ZA",  lcl_DATA_OTHERS },
    { "xh_ZA",  lcl_DATA_OTHERS },
    { "st_ZA",  lcl_DATA_OTHERS },
    { "ss_ZA",  lcl_DATA_OTHERS },
    { "ve_ZA",  lcl_DATA_OTHERS },
    { "nr_ZA",  lcl_DATA_OTHERS },
    { "ts_ZA",  lcl_DATA_OTHERS },
    { "kmr_Latn_TR",  lcl_DATA_OTHERS },
    { "ak_GH",  lcl_DATA_OTHERS },
    { "af_NA",  lcl_DATA_OTHERS },
    { "am_ET",  lcl_DATA_OTHERS },
    { "ti_ER",  lcl_DATA_OTHERS },
    { "tg_TJ",  lcl_DATA_OTHERS },
    { "ky_KG",  lcl_DATA_OTHERS },
    { "kk_KZ",  lcl_DATA_OTHERS },
    { "fa_IR",  lcl_DATA_OTHERS },
    { "ha_Latn_GH",  lcl_DATA_OTHERS },
    { "ee_GH",  lcl_DATA_OTHERS },
    { "sg_CF",  lcl_DATA_OTHERS },
    { "lg_UG",  lcl_DATA_OTHERS },
    { "uz_UZ",  lcl_DATA_OTHERS },
    { "ln_CD",  lcl_DATA_OTHERS },
    { "hy_AM",  lcl_DATA_OTHERS },
    { "hil_PH", lcl_DATA_OTHERS },
    { "so_SO",  lcl_DATA_OTHERS },
    { "gug_PY", lcl_DATA_OTHERS },
    { "tk_TM",  lcl_DATA_OTHERS },
    { "my_MM",  lcl_DATA_OTHERS },
    { "shs_CA", lcl_DATA_OTHERS },
    { "tpi_PG", lcl_DATA_OTHERS },
    { "ar_OM",  lcl_DATA_OTHERS },
    { "ug_CN",  lcl_DATA_OTHERS },
    { "om_ET",  lcl_DATA_OTHERS },
    { "plt_MG", lcl_DATA_OTHERS },
    { "mai_IN", lcl_DATA_OTHERS },
    { "yi_US",  lcl_DATA_OTHERS },
    { "haw_US", lcl_DATA_OTHERS },
    { "lif_NP", lcl_DATA_OTHERS },
    { "ur_PK",  lcl_DATA_OTHERS },
    { "ht_HT",  lcl_DATA_OTHERS },
    { "jbo",    lcl_DATA_OTHERS },
    { "kab_DZ", lcl_DATA_OTHERS },
    { "pt_AO",  lcl_DATA_OTHERS },
    { "pjt_AU", lcl_DATA_OTHERS },
    { "pap_BQ", lcl_DATA_OTHERS },
    { "pap_CW", lcl_DATA_OTHERS },
    { "ebo_CG", lcl_DATA_OTHERS },
    { "tyx_CG", lcl_DATA_OTHERS },
    { "axk_CG", lcl_DATA_OTHERS },
    { "beq_CG", lcl_DATA_OTHERS },
    { "bkw_CG", lcl_DATA_OTHERS },
    { "bvx_CG", lcl_DATA_OTHERS },
    { "dde_CG", lcl_DATA_OTHERS },
    { "iyx_CG", lcl_DATA_OTHERS },
    { "kkw_CG", lcl_DATA_OTHERS },
    { "kng_CG", lcl_DATA_OTHERS },
    { "ldi_CG", lcl_DATA_OTHERS },
    { "mdw_CG", lcl_DATA_OTHERS },
    { "mkw_CG", lcl_DATA_OTHERS },
    { "njx_CG", lcl_DATA_OTHERS },
    { "ngz_CG", lcl_DATA_OTHERS },
    { "njy_CG", lcl_DATA_OTHERS },
    { "puu_CG", lcl_DATA_OTHERS },
    { "sdj_CG", lcl_DATA_OTHERS },
    { "tek_CG", lcl_DATA_OTHERS },
    { "tsa_CG", lcl_DATA_OTHERS },
    { "vif_CG", lcl_DATA_OTHERS },
    { "xku_CG", lcl_DATA_OTHERS },
    { "yom_CG", lcl_DATA_OTHERS },
    { "sid_ET", lcl_DATA_OTHERS },
    { "bo_CN",  lcl_DATA_OTHERS },
    { "bo_IN",  lcl_DATA_OTHERS },
    { "ar_AE",  lcl_DATA_OTHERS },
    { "ar_KW",  lcl_DATA_OTHERS },
    { "bm_ML",  lcl_DATA_OTHERS },
    { "pui_CO", lcl_DATA_OTHERS },
    { "lgr_SB", lcl_DATA_OTHERS },
    { "mos_BF", lcl_DATA_OTHERS },
    { "ny_MW",  lcl_DATA_OTHERS },
    { "ar_BH",  lcl_DATA_OTHERS },
    { "ar_IQ",  lcl_DATA_OTHERS },
    { "ar_JO",  lcl_DATA_OTHERS },
    { "ar_LY",  lcl_DATA_OTHERS },
    { "ar_MA",  lcl_DATA_OTHERS },
    { "ar_QA",  lcl_DATA_OTHERS },
    { "ar_SY",  lcl_DATA_OTHERS },
    { "ar_YE",  lcl_DATA_OTHERS },
    { "ilo_PH", lcl_DATA_OTHERS },
    { "ha_Latn_NG",  lcl_DATA_OTHERS },
    { "min_ID", lcl_DATA_OTHERS },
    { "sun_ID", lcl_DATA_OTHERS },
    { "en_IL",  lcl_DATA_OTHERS },  // needs to be in OTHERS instead of EN because inherits from he_IL
    { "pdc_US", lcl_DATA_OTHERS }
};

#else

#include "localedata_static.hxx"

#endif

const sal_Unicode cUnder = '_';
const sal_Unicode cHyphen = '-';

const sal_Int16 nbOfLocales = SAL_N_ELEMENTS(aLibTable);

namespace i18npool {

// static
Sequence< CalendarItem > LocaleDataImpl::downcastCalendarItems( const Sequence< CalendarItem2 > & rCi )
{
    return comphelper::containerToSequence<CalendarItem>(rCi);
}


// static
Calendar LocaleDataImpl::downcastCalendar( const Calendar2 & rC )
{
    Calendar aCal(
            downcastCalendarItems( rC.Days),
            downcastCalendarItems( rC.Months),
            downcastCalendarItems( rC.Eras),
            rC.StartOfWeek,
            rC.MinimumNumberOfDaysForFirstWeek,
            rC.Default,
            rC.Name
            );
    return aCal;
}


LocaleDataImpl::LocaleDataImpl()
{
}
LocaleDataImpl::~LocaleDataImpl()
{
}


LocaleDataItem SAL_CALL
LocaleDataImpl::getLocaleItem( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getLocaleItem" ));

    if ( func ) {
        sal_Int16 dataItemCount = 0;
        sal_Unicode **dataItem = func(dataItemCount);

        LocaleDataItem item{
                OUString(dataItem[0]),
                OUString(dataItem[1]),
                OUString(dataItem[2]),
                OUString(dataItem[3]),
                OUString(dataItem[4]),
                OUString(dataItem[5]),
                OUString(dataItem[6]),
                OUString(dataItem[7]),
                OUString(dataItem[8]),
                OUString(dataItem[9]),
                OUString(dataItem[10]),
                OUString(dataItem[11]),
                OUString(dataItem[12]),
                OUString(dataItem[13]),
                OUString(dataItem[14]),
                OUString(dataItem[15]),
                OUString(dataItem[16]),
                OUString(dataItem[17])
                };
        return item;
    }
    else {
        LocaleDataItem item1;
        return item1;
    }
}


LocaleDataItem2 SAL_CALL
LocaleDataImpl::getLocaleItem2( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getLocaleItem" ));

    if ( func ) {
        sal_Int16 dataItemCount = 0;
        sal_Unicode **dataItem = func(dataItemCount);

        assert(dataItemCount >= 18);

        LocaleDataItem2 item{
                OUString(dataItem[0]),
                OUString(dataItem[1]),
                OUString(dataItem[2]),
                OUString(dataItem[3]),
                OUString(dataItem[4]),
                OUString(dataItem[5]),
                OUString(dataItem[6]),
                OUString(dataItem[7]),
                OUString(dataItem[8]),
                OUString(dataItem[9]),
                OUString(dataItem[10]),
                OUString(dataItem[11]),
                OUString(dataItem[12]),
                OUString(dataItem[13]),
                OUString(dataItem[14]),
                OUString(dataItem[15]),
                OUString(dataItem[16]),
                OUString(dataItem[17]),
                dataItemCount >= 19 ? OUString(dataItem[18]) : OUString()
                };
        return item;
    }
    else {
        LocaleDataItem2 item1;
        return item1;
    }
}

#ifndef DISABLE_DYNLOADING

extern "C" { static void thisModule() {} }

#endif

namespace
{

// implement the lookup table as a safe static object
class lcl_LookupTableHelper
{
public:
    lcl_LookupTableHelper();
    ~lcl_LookupTableHelper();

    oslGenericFunction getFunctionSymbolByName(
            const OUString& localeName, const char* pFunction,
            std::optional<LocaleDataLookupTableItem>& rOutCachedItem );

private:
    std::mutex maMutex;
    ::std::vector< LocaleDataLookupTableItem >  maLookupTable;
};

// from instance.hxx: Helper base class for a late-initialized
// (default-constructed) static variable, implementing the double-checked
// locking pattern correctly.
// usage:  lcl_LookupTableHelper & rLookupTable = lcl_LookupTableStatic::get();
// retrieves the singleton lookup table instance
lcl_LookupTableHelper& lcl_LookupTableStatic()
{
    static lcl_LookupTableHelper SINGLETON;
    return SINGLETON;
}

lcl_LookupTableHelper::lcl_LookupTableHelper()
{
}

lcl_LookupTableHelper::~lcl_LookupTableHelper()
{
    for ( const LocaleDataLookupTableItem& item : maLookupTable ) {
        delete item.module;
    }
}

oslGenericFunction lcl_LookupTableHelper::getFunctionSymbolByName(
        const OUString& localeName, const char* pFunction,
        std::optional<LocaleDataLookupTableItem>& rOutCachedItem )
{
    OUString aFallback;
    bool bFallback = (localeName.indexOf( cUnder) < 0);
    if (bFallback)
    {
        Locale aLocale;
        aLocale.Language = localeName;
        Locale aFbLocale = MsLangId::getFallbackLocale( aLocale);
        if (aFbLocale == aLocale)
            bFallback = false;  // may be a "language-only-locale" like Interlingua (ia)
        else
            aFallback = LocaleDataImpl::getFirstLocaleServiceName( aFbLocale);
    }

    for (const auto & i : aLibTable)
    {
        if (localeName.equalsAscii(i.pLocale) ||
                (bFallback && aFallback.equalsAscii(i.pLocale)))
        {
#ifndef DISABLE_DYNLOADING
            {
                std::unique_lock aGuard( maMutex );
                for (LocaleDataLookupTableItem & rCurrent : maLookupTable)
                {
                    if (rCurrent.dllName == i.pLib)
                    {
                        rOutCachedItem.emplace( rCurrent );
                        rOutCachedItem->localeName = i.pLocale;
                        OString sSymbolName = OString::Concat(pFunction) + "_" +
                                rOutCachedItem->localeName;
                        return rOutCachedItem->module->getFunctionSymbol(
                                sSymbolName.getStr());
                    }
                }
            }
            // Library not loaded, load it and add it to the list.
#ifdef SAL_DLLPREFIX
            OString sModuleName =    // mostly "lib*.so"
                OString::Concat(SAL_DLLPREFIX) + i.pLib + SAL_DLLEXTENSION;
#else
            OString sModuleName =    // mostly "*.dll"
                OString::Concat(i.pLib) + SAL_DLLEXTENSION;
#endif
            std::unique_ptr<osl::Module> module(new osl::Module());
            if ( module->loadRelative(&thisModule, sModuleName.getStr()) )
            {
                std::unique_lock aGuard( maMutex );
                auto pTmpModule = module.get();
                maLookupTable.emplace_back(i.pLib, module.release(), i.pLocale);
                rOutCachedItem.emplace( maLookupTable.back() );
                OString sSymbolName = OString::Concat(pFunction) + "_" + rOutCachedItem->localeName;
                return pTmpModule->getFunctionSymbol(sSymbolName.getStr());
            }
            else
                module.reset();
#else
            (void) rOutCachedItem;

            if( strcmp(pFunction, "getAllCalendars") == 0 )
                return i.getAllCalendars;
            else if( strcmp(pFunction, "getAllCurrencies") == 0 )
                return i.getAllCurrencies;
            else if( strcmp(pFunction, "getAllFormats0") == 0 )
                return i.getAllFormats0;
            else if( strcmp(pFunction, "getBreakIteratorRules") == 0 )
                return i.getBreakIteratorRules;
            else if( strcmp(pFunction, "getCollationOptions") == 0 )
                return i.getCollationOptions;
            else if( strcmp(pFunction, "getCollatorImplementation") == 0 )
                return i.getCollatorImplementation;
            else if( strcmp(pFunction, "getContinuousNumberingLevels") == 0 )
                return i.getContinuousNumberingLevels;
            else if( strcmp(pFunction, "getDateAcceptancePatterns") == 0 )
                return i.getDateAcceptancePatterns;
            else if( strcmp(pFunction, "getFollowPageWords") == 0 )
                return i.getFollowPageWords;
            else if( strcmp(pFunction, "getForbiddenCharacters") == 0 )
                return i.getForbiddenCharacters;
            else if( strcmp(pFunction, "getIndexAlgorithm") == 0 )
                return i.getIndexAlgorithm;
            else if( strcmp(pFunction, "getLCInfo") == 0 )
                return i.getLCInfo;
            else if( strcmp(pFunction, "getLocaleItem") == 0 )
                return i.getLocaleItem;
            else if( strcmp(pFunction, "getOutlineNumberingLevels") == 0 )
                return i.getOutlineNumberingLevels;
            else if( strcmp(pFunction, "getReservedWords") == 0 )
                return i.getReservedWords;
            else if( strcmp(pFunction, "getSearchOptions") == 0 )
                return i.getSearchOptions;
            else if( strcmp(pFunction, "getTransliterations") == 0 )
                return i.getTransliterations;
            else if( strcmp(pFunction, "getUnicodeScripts") == 0 )
                return i.getUnicodeScripts;
            else if( strcmp(pFunction, "getAllFormats1") == 0 )
                return i.getAllFormats1;
#endif
        }
    }
    return nullptr;
}

} // anonymous namespace


// REF values equal offsets of counts within getAllCalendars() data structure!
#define REF_DAYS         0
#define REF_MONTHS       1
#define REF_GMONTHS      2
#define REF_PMONTHS      3
#define REF_ERAS         4
#define REF_OFFSET_COUNT 5

Sequence< CalendarItem2 > &LocaleDataImpl::getCalendarItemByName(const OUString& name,
        const Locale& rLocale, const Sequence< Calendar2 >& calendarsSeq, sal_Int16 item)
{
    if (ref_name != name) {
        OUString aLocStr, id;
        sal_Int32 nLastUnder = name.lastIndexOf( cUnder);
        SAL_WARN_IF( nLastUnder < 1, "i18npool",
                "LocaleDataImpl::getCalendarItemByName - no '_' or first in name can't be right: " << name);
        if (nLastUnder >= 0)
        {
            aLocStr = name.copy( 0, nLastUnder);
            if (nLastUnder + 1 < name.getLength())
                id = name.copy( nLastUnder + 1);
        }
        Locale loc( LanguageTag::convertToLocale( aLocStr.replace( cUnder, cHyphen)));
        Sequence < Calendar2 > cals;
        if (loc == rLocale) {
            cals = calendarsSeq;
        } else {
            cals = getAllCalendars2(loc);
        }
        auto pCal = std::find_if(std::cbegin(cals), std::cend(cals),
            [&id](const Calendar2& rCal) { return id == rCal.Name; });
        if (pCal != std::cend(cals))
            ref_cal = *pCal;
        else {
            // Referred locale not found, return name for en_US locale.
            cals = getAllCalendars2( Locale("en", "US", OUString()) );
            if (!cals.hasElements())
                throw RuntimeException();
            ref_cal = cals.getConstArray()[0];
        }
        ref_name = name;
    }
    switch (item)
    {
        case REF_DAYS:
            return ref_cal.Days;
        case REF_MONTHS:
            return ref_cal.Months;
        case REF_GMONTHS:
            return ref_cal.GenitiveMonths;
        case REF_PMONTHS:
            return ref_cal.PartitiveMonths;
        default:
            OSL_FAIL( "LocaleDataImpl::getCalendarItemByName: unhandled REF_* case");
            [[fallthrough]];
        case REF_ERAS:
            return ref_cal.Eras;
    }
}

Sequence< CalendarItem2 > LocaleDataImpl::getCalendarItems(
        sal_Unicode const * const * const allCalendars, sal_Int16 & rnOffset,
        const sal_Int16 nWhichItem, const sal_Int16 nCalendar,
        const Locale & rLocale, const Sequence< Calendar2 > & calendarsSeq )
{
    Sequence< CalendarItem2 > aItems;
    if ( allCalendars[rnOffset] == std::u16string_view(u"ref") )
    {
        aItems = getCalendarItemByName( OUString( allCalendars[rnOffset+1]), rLocale, calendarsSeq, nWhichItem);
        rnOffset += 2;
    }
    else
    {
        const sal_Int32 nSize = allCalendars[nWhichItem][nCalendar];
        aItems.realloc( nSize);
        switch (nWhichItem)
        {
            case REF_DAYS:
            case REF_MONTHS:
            case REF_GMONTHS:
            case REF_PMONTHS:
                for (CalendarItem2& rItem : asNonConstRange(aItems))
                {
                    rItem = CalendarItem2{ OUString(allCalendars[rnOffset]),
                            OUString(allCalendars[rnOffset+1]),
                            OUString(allCalendars[rnOffset+2]), OUString(allCalendars[rnOffset+3])};
                    rnOffset += 4;
                }
                break;
            case REF_ERAS:
                // Absent narrow name.
                for (CalendarItem2& rItem : asNonConstRange(aItems))
                {
                    rItem = CalendarItem2{ OUString(allCalendars[rnOffset]),
                            OUString(allCalendars[rnOffset+1]),
                            OUString(allCalendars[rnOffset+2]), OUString()};
                    rnOffset += 3;
                }
                break;
            default:
                OSL_FAIL( "LocaleDataImpl::getCalendarItems: unhandled REF_* case");
        }
    }
    return aItems;
}

Sequence< Calendar2 > SAL_CALL
LocaleDataImpl::getAllCalendars2( const Locale& rLocale )
{

    sal_Unicode const * const * allCalendars = nullptr;

    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getAllCalendars" ));

    if ( func ) {
        sal_Int16 calendarsCount = 0;
        allCalendars = func(calendarsCount);

        Sequence< Calendar2 > calendarsSeq(calendarsCount);
        auto calendarsSeqRange = asNonConstRange(calendarsSeq);
        sal_Int16 offset = REF_OFFSET_COUNT;
        for(sal_Int16 i = 0; i < calendarsCount; i++) {
            OUString calendarID(allCalendars[offset]);
            offset++;
            bool defaultCalendar = allCalendars[offset][0] != 0;
            offset++;
            Sequence< CalendarItem2 > days = getCalendarItems( allCalendars, offset, REF_DAYS, i,
                    rLocale, calendarsSeq);
            Sequence< CalendarItem2 > months = getCalendarItems( allCalendars, offset, REF_MONTHS, i,
                    rLocale, calendarsSeq);
            Sequence< CalendarItem2 > gmonths = getCalendarItems( allCalendars, offset, REF_GMONTHS, i,
                    rLocale, calendarsSeq);
            Sequence< CalendarItem2 > pmonths = getCalendarItems( allCalendars, offset, REF_PMONTHS, i,
                    rLocale, calendarsSeq);
            Sequence< CalendarItem2 > eras = getCalendarItems( allCalendars, offset, REF_ERAS, i,
                    rLocale, calendarsSeq);
            OUString startOfWeekDay(allCalendars[offset]);
            offset++;
            sal_Int16 minimalDaysInFirstWeek = allCalendars[offset][0];
            offset++;
            Calendar2 aCalendar(days, months, gmonths, pmonths, eras, startOfWeekDay,
                    minimalDaysInFirstWeek, defaultCalendar, calendarID);
            calendarsSeqRange[i] = aCalendar;
        }
        return calendarsSeq;
    }
    else {
        return {};
    }
}


Sequence< Calendar > SAL_CALL
LocaleDataImpl::getAllCalendars( const Locale& rLocale )
{
    const Sequence< Calendar2 > aCal2( getAllCalendars2( rLocale));
    std::vector<Calendar> aCal1;
    aCal1.reserve(aCal2.getLength());
    std::transform(aCal2.begin(), aCal2.end(), std::back_inserter(aCal1),
        [](const Calendar2& rCal2) { return downcastCalendar(rCal2); });
    return comphelper::containerToSequence(aCal1);
}


Sequence< Currency2 > SAL_CALL
LocaleDataImpl::getAllCurrencies2( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getAllCurrencies" ));

    if ( func ) {
        sal_Int16 currencyCount = 0;
        sal_Unicode **allCurrencies = func(currencyCount);

        Sequence< Currency2 > seq(currencyCount);
        auto seqRange = asNonConstRange(seq);
        for(int i = 0, nOff = 0; i < currencyCount; i++, nOff += 8 ) {
            Currency2 cur(
                    OUString(allCurrencies[nOff]), // string ID
                    OUString(allCurrencies[nOff+1]), // string Symbol
                    OUString(allCurrencies[nOff+2]), // string BankSymbol
                    OUString(allCurrencies[nOff+3]), // string Name
                    allCurrencies[nOff+4][0] != 0, // boolean Default
                    allCurrencies[nOff+5][0] != 0, // boolean UsedInCompatibleFormatCodes
                    allCurrencies[nOff+6][0],   // short DecimalPlaces
                    allCurrencies[nOff+7][0] != 0 // boolean LegacyOnly
                    );
            seqRange[i] = cur;
        }
        return seq;
    }
    else {
        return {};
    }
}


Sequence< Currency > SAL_CALL
LocaleDataImpl::getAllCurrencies( const Locale& rLocale )
{
    return comphelper::containerToSequence<Currency>(getAllCurrencies2(rLocale));
}


Sequence< FormatElement > SAL_CALL
LocaleDataImpl::getAllFormats( const Locale& rLocale )
{
    const int SECTIONS = 2;
    struct FormatSection
    {
        MyFunc_FormatCode         func;
        sal_Unicode const        *from;
        sal_Unicode const        *to;
        sal_Unicode const *const *formatArray;
        sal_Int16                 formatCount;

        FormatSection() : func(nullptr), from(nullptr), to(nullptr), formatArray(nullptr), formatCount(0) {}
        sal_Int16 getFunc( LocaleDataImpl& rLocaleData, const Locale& rL, const char* pName )
        {
            func = reinterpret_cast<MyFunc_FormatCode>( rLocaleData.getFunctionSymbol( rL, pName));
            if (func)
                formatArray = func( formatCount, from, to);
            return formatCount;
        }
    } section[SECTIONS];

    sal_Int32 formatCount;
    formatCount  = section[0].getFunc( *this, rLocale, "getAllFormats0");
    formatCount += section[1].getFunc( *this, rLocale, "getAllFormats1");

    Sequence< FormatElement > seq(formatCount);
    auto seqRange = asNonConstRange(seq);
    sal_Int32 f = 0;
    for (const FormatSection & s : section)
    {
        sal_Unicode const * const * const formatArray = s.formatArray;
        if ( formatArray )
        {
            for (int i = 0, nOff = 0; i < s.formatCount; ++i, nOff += 7, ++f)
            {
                FormatElement elem(
                        OUString(formatArray[nOff]).replaceAll(s.from, s.to),
                        OUString(formatArray[nOff + 1]),
                        OUString(formatArray[nOff + 2]),
                        OUString(formatArray[nOff + 3]),
                        OUString(formatArray[nOff + 4]),
                        formatArray[nOff + 5][0],
                        formatArray[nOff + 6][0] != 0);
                seqRange[f] = elem;
            }
        }
    }
    return seq;
}


Sequence< OUString > SAL_CALL
LocaleDataImpl::getDateAcceptancePatterns( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getDateAcceptancePatterns" ));

    if (func)
    {
        sal_Int16 patternsCount = 0;
        sal_Unicode **patternsArray = func( patternsCount );
        Sequence< OUString > seq( patternsCount );
        auto seqRange = asNonConstRange(seq);
        for (sal_Int16 i = 0; i < patternsCount; ++i)
        {
            seqRange[i] = OUString( patternsArray[i] );
        }
        return seq;
    }
    else
    {
        return {};
    }
}


#define COLLATOR_OFFSET_ALGO    0
#define COLLATOR_OFFSET_DEFAULT 1
#define COLLATOR_OFFSET_RULE    2
#define COLLATOR_ELEMENTS       3

OUString
LocaleDataImpl::getCollatorRuleByAlgorithm( const Locale& rLocale, std::u16string_view algorithm )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getCollatorImplementation" ));
    if ( func ) {
        sal_Int16 collatorCount = 0;
        sal_Unicode **collatorArray = func(collatorCount);
        for(sal_Int16 i = 0; i < collatorCount; i++)
            if (algorithm == collatorArray[i * COLLATOR_ELEMENTS + COLLATOR_OFFSET_ALGO])
                return OUString(collatorArray[i * COLLATOR_ELEMENTS + COLLATOR_OFFSET_RULE]);
    }
    return OUString();
}


Sequence< Implementation > SAL_CALL
LocaleDataImpl::getCollatorImplementations( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getCollatorImplementation" ));

    if ( func ) {
        sal_Int16 collatorCount = 0;
        sal_Unicode **collatorArray = func(collatorCount);
        Sequence< Implementation > seq(collatorCount);
        auto seqRange = asNonConstRange(seq);
        for(sal_Int16 i = 0; i < collatorCount; i++) {
            Implementation impl(
                    OUString(collatorArray[i * COLLATOR_ELEMENTS + COLLATOR_OFFSET_ALGO]),
                    collatorArray[i * COLLATOR_ELEMENTS + COLLATOR_OFFSET_DEFAULT][0] != 0);
            seqRange[i] = impl;
        }
        return seq;
    }
    else {
        return {};
    }
}

Sequence< OUString > SAL_CALL
LocaleDataImpl::getCollationOptions( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getCollationOptions" ));

    if ( func ) {
        sal_Int16 optionsCount = 0;
        sal_Unicode **optionsArray = func(optionsCount);
        Sequence< OUString > seq(optionsCount);
        auto seqRange = asNonConstRange(seq);
        for(sal_Int16 i = 0; i < optionsCount; i++) {
            seqRange[i] = OUString( optionsArray[i] );
        }
        return seq;
    }
    else {
        return {};
    }
}

Sequence< OUString > SAL_CALL
LocaleDataImpl::getSearchOptions( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getSearchOptions" ));

    if ( func ) {
        sal_Int16 optionsCount = 0;
        sal_Unicode **optionsArray = func(optionsCount);
        Sequence< OUString > seq(optionsCount);
        auto seqRange = asNonConstRange(seq);
        for(sal_Int16 i = 0; i < optionsCount; i++) {
            seqRange[i] = OUString( optionsArray[i] );
        }
        return seq;
    }
    else {
        return {};
    }
}

sal_Unicode **
LocaleDataImpl::getIndexArray(const Locale& rLocale, sal_Int16& indexCount)
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getIndexAlgorithm" ));

    if (func)
        return func(indexCount);
    return nullptr;
}

Sequence< OUString >
LocaleDataImpl::getIndexAlgorithm( const Locale& rLocale )
{
    sal_Int16 indexCount = 0;
    sal_Unicode **indexArray = getIndexArray(rLocale, indexCount);

    if ( indexArray ) {
        Sequence< OUString > seq(indexCount);
        auto seqRange = asNonConstRange(seq);
        for(sal_Int16 i = 0; i < indexCount; i++) {
            seqRange[i] = indexArray[i*5];
        }
        return seq;
    }
    else {
        return {};
    }
}

OUString
LocaleDataImpl::getDefaultIndexAlgorithm( const Locale& rLocale )
{
    sal_Int16 indexCount = 0;
    sal_Unicode **indexArray = getIndexArray(rLocale, indexCount);

    if ( indexArray ) {
        for(sal_Int16 i = 0; i < indexCount; i++) {
            if (indexArray[i*5 + 3][0])
                return OUString(indexArray[i*5]);
        }
    }
    return OUString();
}

bool
LocaleDataImpl::hasPhonetic( const Locale& rLocale )
{
    sal_Int16 indexCount = 0;
    sal_Unicode **indexArray = getIndexArray(rLocale, indexCount);

    if ( indexArray ) {
        for(sal_Int16 i = 0; i < indexCount; i++) {
            if (indexArray[i*5 + 4][0])
                return true;
        }
    }
    return false;
}

sal_Unicode **
LocaleDataImpl::getIndexArrayForAlgorithm(const Locale& rLocale, std::u16string_view algorithm)
{
    sal_Int16 indexCount = 0;
    sal_Unicode **indexArray = getIndexArray(rLocale, indexCount);
    if ( indexArray ) {
        for(sal_Int16 i = 0; i < indexCount; i++) {
            if (algorithm == indexArray[i*5])
                return indexArray+i*5;
        }
    }
    return nullptr;
}

bool
LocaleDataImpl::isPhonetic( const Locale& rLocale, std::u16string_view algorithm )
{
    sal_Unicode **indexArray = getIndexArrayForAlgorithm(rLocale, algorithm);
    return indexArray && indexArray[4][0];
}

OUString
LocaleDataImpl::getIndexKeysByAlgorithm( const Locale& rLocale, std::u16string_view algorithm )
{
    sal_Unicode **indexArray = getIndexArrayForAlgorithm(rLocale, algorithm);
    return indexArray ? (OUString::Concat(u"0-9") + indexArray[2]) : OUString();
}

OUString
LocaleDataImpl::getIndexModuleByAlgorithm( const Locale& rLocale, std::u16string_view algorithm )
{
    sal_Unicode **indexArray = getIndexArrayForAlgorithm(rLocale, algorithm);
    return indexArray ? OUString(indexArray[1]) : OUString();
}

Sequence< UnicodeScript >
LocaleDataImpl::getUnicodeScripts( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getUnicodeScripts" ));

    if ( func ) {
        sal_Int16 scriptCount = 0;
        sal_Unicode **scriptArray = func(scriptCount);
        Sequence< UnicodeScript > seq(scriptCount);
        auto seqRange = asNonConstRange(seq);
        for(sal_Int16 i = 0; i < scriptCount; i++) {
            seqRange[i] = UnicodeScript( OUString(scriptArray[i]).toInt32() );
        }
        return seq;
    }
    else {
        return {};
    }
}

Sequence< OUString >
LocaleDataImpl::getFollowPageWords( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getFollowPageWords" ));

    if ( func ) {
        sal_Int16 wordCount = 0;
        sal_Unicode **wordArray = func(wordCount);
        Sequence< OUString > seq(wordCount);
        auto seqRange = asNonConstRange(seq);
        for(sal_Int16 i = 0; i < wordCount; i++) {
            seqRange[i] = OUString(wordArray[i]);
        }
        return seq;
    }
    else {
        return {};
    }
}

Sequence< OUString > SAL_CALL
LocaleDataImpl::getTransliterations( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getTransliterations" ));

    if ( func ) {
        sal_Int16 transliterationsCount = 0;
        sal_Unicode **transliterationsArray = func(transliterationsCount);

        Sequence< OUString > seq(transliterationsCount);
        auto seqRange = asNonConstRange(seq);
        for(int i = 0; i < transliterationsCount; i++) {
            OUString  elem(transliterationsArray[i]);
            seqRange[i] = elem;
        }
        return seq;
    }
    else {
        return {};
    }


}


LanguageCountryInfo SAL_CALL
LocaleDataImpl::getLanguageCountryInfo( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getLCInfo" ));

    if ( func ) {
        sal_Int16 LCInfoCount = 0;
        sal_Unicode **LCInfoArray = func(LCInfoCount);
        LanguageCountryInfo info{OUString(LCInfoArray[0]),
                OUString(LCInfoArray[1]),
                OUString(LCInfoArray[2]),
                OUString(LCInfoArray[3]),
                OUString(LCInfoArray[4])};
        return info;
    }
    else {
        LanguageCountryInfo info1;
        return info1;
    }

}


ForbiddenCharacters SAL_CALL
LocaleDataImpl::getForbiddenCharacters( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getForbiddenCharacters" ));

    if ( func ) {
        sal_Int16 LCForbiddenCharactersCount = 0;
        sal_Unicode **LCForbiddenCharactersArray = func(LCForbiddenCharactersCount);
        ForbiddenCharacters chars{
            OUString(LCForbiddenCharactersArray[0]), OUString(LCForbiddenCharactersArray[1])};
        return chars;
    }
    else {
        ForbiddenCharacters chars1;
        return chars1;
    }
}

OUString
LocaleDataImpl::getHangingCharacters( const Locale& rLocale )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getForbiddenCharacters" ));

    if ( func ) {
        sal_Int16 LCForbiddenCharactersCount = 0;
        sal_Unicode **LCForbiddenCharactersArray = func(LCForbiddenCharactersCount);
        return OUString(LCForbiddenCharactersArray[2]);
    }

    return OUString();
}

Sequence< OUString >
LocaleDataImpl::getBreakIteratorRules( const Locale& rLocale  )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getBreakIteratorRules" ));

    if ( func ) {
        sal_Int16 LCBreakIteratorRuleCount = 0;
        sal_Unicode **LCBreakIteratorRulesArray = func(LCBreakIteratorRuleCount);
        Sequence< OUString > seq(LCBreakIteratorRuleCount);
        auto seqRange = asNonConstRange(seq);
        for(int i = 0; i < LCBreakIteratorRuleCount; i++) {
            OUString  elem(LCBreakIteratorRulesArray[i]);
            seqRange[i] = elem;
        }
        return seq;
    }
    else {
        return {};
    }
}


Sequence< OUString > SAL_CALL
LocaleDataImpl::getReservedWord( const Locale& rLocale  )
{
    MyFunc_Type func = reinterpret_cast<MyFunc_Type>(getFunctionSymbol( rLocale, "getReservedWords" ));

    if ( func ) {
        sal_Int16 LCReservedWordsCount = 0;
        sal_Unicode **LCReservedWordsArray = func(LCReservedWordsCount);
        Sequence< OUString > seq(LCReservedWordsCount);
        auto seqRange = asNonConstRange(seq);
        for(int i = 0; i < LCReservedWordsCount; i++) {
            OUString  elem(LCReservedWordsArray[i]);
            seqRange[i] = elem;
        }
        return seq;
    }
    else {
        return {};
    }
}


Sequence< Sequence<beans::PropertyValue> >
LocaleDataImpl::getContinuousNumberingLevels( const lang::Locale& rLocale )
{
    // load symbol
    MyFunc_Type2 func = reinterpret_cast<MyFunc_Type2>(getFunctionSymbol( rLocale, "getContinuousNumberingLevels" ));

    if ( func )
    {
        // invoke function
        sal_Int16 nStyles;
        sal_Int16 nAttributes;
        sal_Unicode const *** p0 = func( nStyles, nAttributes );

        // allocate memory for nAttributes attributes for each of the nStyles styles.
        Sequence< Sequence<beans::PropertyValue> > pv( nStyles );
        auto pvRange = asNonConstRange(pv);
        for( auto& i : pvRange ) {
            i = Sequence<beans::PropertyValue>( nAttributes );
        }

        sal_Unicode const *** pStyle = p0;
        for( int i=0;  i<nStyles;  i++ ) {
            sal_Unicode const ** pAttribute = pStyle[i];
            auto pvElementRange = asNonConstRange(pvRange[i]);
            for( int j=0;  j<nAttributes;  j++ ) { // prefix, numberingtype, ...
                sal_Unicode const * pString = pAttribute[j];
                beans::PropertyValue& rVal = pvElementRange[j];
                OUString sVal;
                if( pString ) {
                    if( 0 != j && 2 != j )
                        sVal = pString;
                    else if( *pString )
                        sVal = OUString( pString, 1 );
                }

                switch( j )
                {
                    case 0:
                        rVal.Name = "Prefix";
                        rVal.Value <<= sVal;
                        break;
                    case 1:
                        rVal.Name = "NumberingType";
                        rVal.Value <<= static_cast<sal_Int16>(sVal.toInt32());
                        break;
                    case 2:
                        rVal.Name = "Suffix";
                        rVal.Value <<= sVal;
                        break;
                    case 3:
                        rVal.Name = "Transliteration";
                        rVal.Value <<= sVal;
                        break;
                    case 4:
                        rVal.Name = "NatNum";
                        rVal.Value <<= static_cast<sal_Int16>(sVal.toInt32());
                        break;
                    default:
                        OSL_ASSERT(false);
                }
            }
        }
        return pv;
    }

    return Sequence< Sequence<beans::PropertyValue> >();
}

// OutlineNumbering helper class

namespace {

struct OutlineNumberingLevel_Impl
{
    OUString        sPrefix;
    sal_Int16       nNumType; //css::style::NumberingType
    OUString        sSuffix;
    sal_Unicode     cBulletChar;
    OUString        sBulletFontName;
    sal_Int16       nParentNumbering;
    sal_Int32       nLeftMargin;
    sal_Int32       nSymbolTextDistance;
    sal_Int32       nFirstLineOffset;
    OUString        sTransliteration;
    sal_Int32       nNatNum;
};

class OutlineNumbering : public cppu::WeakImplHelper < container::XIndexAccess >
{
    // OutlineNumbering helper class

    std::unique_ptr<const OutlineNumberingLevel_Impl[]> m_pOutlineLevels;
    sal_Int16                         m_nCount;
public:
    OutlineNumbering(std::unique_ptr<const OutlineNumberingLevel_Impl[]> pOutlineLevels, int nLevels);

    //XIndexAccess
    virtual sal_Int32 SAL_CALL getCount(  ) override;
    virtual Any SAL_CALL getByIndex( sal_Int32 Index ) override;

    //XElementAccess
    virtual Type SAL_CALL getElementType(  ) override;
    virtual sal_Bool SAL_CALL hasElements(  ) override;
};

}

Sequence< Reference<container::XIndexAccess> >
LocaleDataImpl::getOutlineNumberingLevels( const lang::Locale& rLocale )
{
    // load symbol
    MyFunc_Type3 func = reinterpret_cast<MyFunc_Type3>(getFunctionSymbol( rLocale, "getOutlineNumberingLevels" ));

    if ( func )
    {
        int i;
        // invoke function
        sal_Int16 nStyles;
        sal_Int16 nLevels;
        sal_Int16 nAttributes;
        sal_Unicode const **** p0 = func( nStyles, nLevels, nAttributes );

        Sequence< Reference<container::XIndexAccess> > aRet( nStyles );
        auto aRetRange = asNonConstRange(aRet);
        sal_Unicode const **** pStyle = p0;
        for( i=0;  i<nStyles;  i++ )
        {
            int j;

            std::unique_ptr<OutlineNumberingLevel_Impl[]> level(new OutlineNumberingLevel_Impl[ nLevels+1 ]);
            sal_Unicode const *** pLevel = pStyle[i];
            for( j = 0;  j < nLevels;  j++ )
            {
                sal_Unicode const ** pAttribute = pLevel[j];
                for( int k=0; k<nAttributes; k++ )
                {
                    OUString tmp( pAttribute[k] );
                    switch( k )
                    {
                        case 0: level[j].sPrefix             = tmp;             break;
                        case 1: level[j].nNumType            = sal::static_int_cast<sal_Int16>(tmp.toInt32()); break;
                        case 2: level[j].sSuffix             = tmp;             break;
                        case 3: level[j].cBulletChar         = sal::static_int_cast<sal_Unicode>(tmp.toUInt32(16)); break; // base 16
                        case 4: level[j].sBulletFontName     = tmp;             break;
                        case 5: level[j].nParentNumbering    = sal::static_int_cast<sal_Int16>(tmp.toInt32()); break;
                        case 6: level[j].nLeftMargin         = tmp.toInt32();   break;
                        case 7: level[j].nSymbolTextDistance = tmp.toInt32();   break;
                        case 8: level[j].nFirstLineOffset    = tmp.toInt32();   break;
                        case 9: break;
                        case 10: level[j].sTransliteration = tmp; break;
                        case 11: level[j].nNatNum    = tmp.toInt32();   break;
                        default:
                                 OSL_ASSERT(false);
                    }
                }
            }
            level[j].sPrefix.clear();
            level[j].nNumType            = 0;
            level[j].sSuffix.clear();
            level[j].cBulletChar         = 0;
            level[j].sBulletFontName.clear();
            level[j].nParentNumbering    = 0;
            level[j].nLeftMargin         = 0;
            level[j].nSymbolTextDistance = 0;
            level[j].nFirstLineOffset    = 0;
            level[j].sTransliteration.clear();
            level[j].nNatNum             = 0;
            aRetRange[i] = new OutlineNumbering( std::move(level), nLevels );
        }
        return aRet;
    }
    else {
        return {};
    }
}

// helper functions

oslGenericFunction LocaleDataImpl::getFunctionSymbol( const Locale& rLocale, const char* pFunction )
{
    lcl_LookupTableHelper & rLookupTable = lcl_LookupTableStatic();

    if (moCachedItem && moCachedItem->equals(rLocale))
    {
        OString sSymbolName = OString::Concat(pFunction) + "_" +
                moCachedItem->localeName;
        return moCachedItem->module->getFunctionSymbol(sSymbolName.getStr());
    }

    oslGenericFunction pSymbol = nullptr;
    std::optional<LocaleDataLookupTableItem> oCachedItem;

    // Load function with name <func>_<lang>_<country> or <func>_<bcp47> and
    // fallbacks.
    pSymbol = rLookupTable.getFunctionSymbolByName( LocaleDataImpl::getFirstLocaleServiceName( rLocale),
            pFunction, oCachedItem);
    if (!pSymbol)
    {
        ::std::vector< OUString > aFallbacks( LocaleDataImpl::getFallbackLocaleServiceNames( rLocale));
        for (const auto& rFallback : aFallbacks)
        {
            pSymbol = rLookupTable.getFunctionSymbolByName(rFallback, pFunction, oCachedItem);
            if (pSymbol)
                break;
        }
    }
    if (!pSymbol)
    {
        // load default function with name <func>_en_US
        pSymbol = rLookupTable.getFunctionSymbolByName("en_US", pFunction, oCachedItem);
    }

    if (!pSymbol)
        // Appropriate symbol could not be found.  Give up.
        throw RuntimeException();

    if (oCachedItem)
        moCachedItem = std::move(oCachedItem);
    if (moCachedItem)
        moCachedItem->aLocale = rLocale;

    return pSymbol;
}

Sequence< Locale > SAL_CALL
LocaleDataImpl::getAllInstalledLocaleNames()
{
    Sequence< lang::Locale > seq( nbOfLocales );
    auto seqRange = asNonConstRange(seq);
    sal_Int16 nInstalled = 0;

    for(const auto & i : aLibTable) {
        OUString name = OUString::createFromAscii( i.pLocale );

        // Check if the locale is really available and not just in the table,
        // don't allow fall backs.
        std::optional<LocaleDataLookupTableItem> oCachedItem;
        if (lcl_LookupTableStatic().getFunctionSymbolByName( name, "getLocaleItem", oCachedItem )) {
            if( oCachedItem )
                moCachedItem = std::move( oCachedItem );
            seqRange[nInstalled++] = LanguageTag::convertToLocale( name.replace( cUnder, cHyphen), false);
        }
    }
    if ( nInstalled < nbOfLocales )
        seq.realloc( nInstalled );          // reflect reality

    return seq;
}

using namespace ::com::sun::star::container;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::text;

OutlineNumbering::OutlineNumbering(std::unique_ptr<const OutlineNumberingLevel_Impl[]> pOutlnLevels, int nLevels) :
    m_pOutlineLevels(std::move(pOutlnLevels)),
    m_nCount(sal::static_int_cast<sal_Int16>(nLevels))
{
}

sal_Int32 OutlineNumbering::getCount(  )
{
    return m_nCount;
}

Any OutlineNumbering::getByIndex( sal_Int32 nIndex )
{
    if(nIndex < 0 || nIndex >= m_nCount)
        throw IndexOutOfBoundsException();
    const OutlineNumberingLevel_Impl* pTemp = m_pOutlineLevels.get();
    pTemp += nIndex;
    Any aRet;

    Sequence<PropertyValue> aOutlineNumbering(12);
    PropertyValue* pValues = aOutlineNumbering.getArray();
    pValues[0].Name = "Prefix";
    pValues[0].Value <<= pTemp->sPrefix;
    pValues[1].Name = "NumberingType";
    pValues[1].Value <<= pTemp->nNumType;
    pValues[2].Name = "Suffix";
    pValues[2].Value <<= pTemp->sSuffix;
    pValues[3].Name = "BulletChar";
    pValues[3].Value <<= OUString(&pTemp->cBulletChar, 1);
    pValues[4].Name = "BulletFontName";
    pValues[4].Value <<= pTemp->sBulletFontName;
    pValues[5].Name = "ParentNumbering";
    pValues[5].Value <<= pTemp->nParentNumbering;
    pValues[6].Name = "LeftMargin";
    pValues[6].Value <<= pTemp->nLeftMargin;
    pValues[7].Name = "SymbolTextDistance";
    pValues[7].Value <<= pTemp->nSymbolTextDistance;
    pValues[8].Name = "FirstLineOffset";
    pValues[8].Value <<= pTemp->nFirstLineOffset;
    pValues[9].Name = "Adjust";
    pValues[9].Value <<= sal_Int16(HoriOrientation::LEFT);
    pValues[10].Name = "Transliteration";
    pValues[10].Value <<= pTemp->sTransliteration;
    pValues[11].Name = "NatNum";
    pValues[11].Value <<= pTemp->nNatNum;
    aRet <<= aOutlineNumbering;
    return aRet;
}

Type OutlineNumbering::getElementType(  )
{
    return cppu::UnoType<Sequence<PropertyValue>>::get();
}

sal_Bool OutlineNumbering::hasElements(  )
{
    return m_nCount > 0;
}

OUString SAL_CALL
LocaleDataImpl::getImplementationName()
{
    return "com.sun.star.i18n.LocaleDataImpl";
}

sal_Bool SAL_CALL LocaleDataImpl::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

Sequence< OUString > SAL_CALL
LocaleDataImpl::getSupportedServiceNames()
{
    Sequence< OUString > aRet {
        "com.sun.star.i18n.LocaleData",
        "com.sun.star.i18n.LocaleData2"
    };
    return aRet;
}

// static
OUString LocaleDataImpl::getFirstLocaleServiceName( const css::lang::Locale & rLocale )
{
    if (rLocale.Language == I18NLANGTAG_QLT)
        return rLocale.Variant.replace( cHyphen, cUnder);
    else if (!rLocale.Country.isEmpty())
        return rLocale.Language + "_" + rLocale.Country;
    else
        return rLocale.Language;
}

// static
::std::vector< OUString > LocaleDataImpl::getFallbackLocaleServiceNames( const css::lang::Locale & rLocale )
{
    ::std::vector< OUString > aVec;
    if (rLocale.Language == I18NLANGTAG_QLT)
    {
        aVec = LanguageTag( rLocale).getFallbackStrings( false);
        for (auto& rItem : aVec)
        {
            rItem = rItem.replace(cHyphen, cUnder);
        }
    }
    else if (!rLocale.Country.isEmpty())
    {
        aVec.push_back( rLocale.Language);
    }
    // else nothing, language-only was the first
    return aVec;
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_i18n_LocaleDataImpl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new i18npool::LocaleDataImpl());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
