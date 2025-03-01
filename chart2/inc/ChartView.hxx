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
#pragma once

#include <chartview/ExplicitValueProvider.hxx>
#include <cppuhelper/implbase.hxx>
#include <comphelper/multicontainer2.hxx>

#include <svl/lstner.hxx>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/datatransfer/XTransferable.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/qa/XDumper.hpp>
#include <com/sun/star/util/XModeChangeBroadcaster.hpp>
#include <com/sun/star/util/XModifyListener.hpp>
#include <com/sun/star/util/XUpdatable2.hpp>
#include <rtl/ref.hxx>
#include <svx/unopage.hxx>
#include <svx/unoshape.hxx>

#include <vector>
#include <memory>

#include <vcl/timer.hxx>
#include <sfx2/xmldump.hxx>

namespace com::sun::star::drawing { class XDrawPage; }
namespace com::sun::star::drawing { class XShapes; }
namespace com::sun::star::io { class XOutputStream; }
namespace com::sun::star::uno { class XComponentContext; }
namespace com::sun::star::util { class XUpdatable2; }

class SdrPage;

namespace chart {

class VCoordinateSystem;
class DrawModelWrapper;
class VDataSeries;
struct CreateShapeParam2D;

struct TimeBasedInfo
{
    TimeBasedInfo():
        bTimeBased(false),
        nFrame(0) {}

    bool bTimeBased;
    size_t nFrame;
    Timer maTimer { "chart2 TimeBasedInfo" };

    // only valid when we are in the time based mode
    std::vector< std::vector< VDataSeries* > > m_aDataSeriesList;
};

/**
 * The ChartView is responsible to manage the generation of Drawing Objects
 * for visualization on a given OutputDevice. The ChartModel is responsible
 * to notify changes to the view. The view than changes to state dirty. The
 * view can be updated with call 'update'.
 *
 * The View is not responsible to handle single user events (that is instead
 * done by the ChartWindow).
 */
class OOO_DLLPUBLIC_CHARTVIEW ChartView final : public ::cppu::WeakImplHelper<
    css::lang::XInitialization
        ,css::lang::XServiceInfo
        ,css::datatransfer::XTransferable
        ,css::lang::XUnoTunnel
        ,css::util::XModifyListener
        ,css::util::XModeChangeBroadcaster
        ,css::util::XUpdatable2
        ,css::beans::XPropertySet
        ,css::lang::XMultiServiceFactory
        ,css::qa::XDumper
        >
        , public ExplicitValueProvider
        , private SfxListener
        , public sfx2::XmlDump
{
private:
    void init();

public:
    ChartView() = delete;
    ChartView(css::uno::Reference< css::uno::XComponentContext > const & xContext,
               ChartModel& rModel);

    virtual ~ChartView() override;

    // ___lang::XServiceInfo___
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // ___lang::XInitialization___
    virtual void SAL_CALL initialize( const css::uno::Sequence< css::uno::Any >& aArguments ) override;

    // ___ExplicitValueProvider___
    virtual bool getExplicitValuesForAxis(
        css::uno::Reference< css::chart2::XAxis > xAxis
        , ExplicitScaleData&  rExplicitScale
        , ExplicitIncrementData& rExplicitIncrement ) override;
    virtual rtl::Reference< SvxShape >
        getShapeForCID( const OUString& rObjectCID ) override;

    virtual css::awt::Rectangle getRectangleOfObject( const OUString& rObjectCID, bool bSnapRect=false ) override;

    virtual css::awt::Rectangle getDiagramRectangleExcludingAxes() override;

    std::shared_ptr< DrawModelWrapper > getDrawModelWrapper() override;

    // ___XTransferable___
    virtual css::uno::Any SAL_CALL getTransferData( const css::datatransfer::DataFlavor& aFlavor ) override;
    virtual css::uno::Sequence< css::datatransfer::DataFlavor > SAL_CALL getTransferDataFlavors(  ) override;
    virtual sal_Bool SAL_CALL isDataFlavorSupported( const css::datatransfer::DataFlavor& aFlavor ) override;

    // css::util::XEventListener (base of XCloseListener and XModifyListener)
    virtual void SAL_CALL
        disposing( const css::lang::EventObject& Source ) override;

    // css::util::XModifyListener
    virtual void SAL_CALL modified(
        const css::lang::EventObject& aEvent ) override;

    //SfxListener
    virtual void Notify( SfxBroadcaster& rBC, const SfxHint& rHint ) override;

    // css::util::XModeChangeBroadcaster

    virtual void SAL_CALL addModeChangeListener( const css::uno::Reference< css::util::XModeChangeListener >& _rxListener ) override;
    virtual void SAL_CALL removeModeChangeListener( const css::uno::Reference< css::util::XModeChangeListener >& _rxListener ) override;
    virtual void SAL_CALL addModeChangeApproveListener( const css::uno::Reference< css::util::XModeChangeApproveListener >& _rxListener ) override;
    virtual void SAL_CALL removeModeChangeApproveListener( const css::uno::Reference< css::util::XModeChangeApproveListener >& _rxListener ) override;

    // css::util::XUpdatable
    virtual void SAL_CALL update() override;

    // util::XUpdatable2
    virtual void SAL_CALL updateSoft() override;
    virtual void SAL_CALL updateHard() override;

    // css::beans::XPropertySet
    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) override;
    virtual void SAL_CALL setPropertyValue( const OUString& aPropertyName, const css::uno::Any& aValue ) override;
    virtual css::uno::Any SAL_CALL getPropertyValue( const OUString& PropertyName ) override;
    virtual void SAL_CALL addPropertyChangeListener( const OUString& aPropertyName, const css::uno::Reference< css::beans::XPropertyChangeListener >& xListener ) override;
    virtual void SAL_CALL removePropertyChangeListener( const OUString& aPropertyName, const css::uno::Reference< css::beans::XPropertyChangeListener >& aListener ) override;
    virtual void SAL_CALL addVetoableChangeListener( const OUString& PropertyName, const css::uno::Reference< css::beans::XVetoableChangeListener >& aListener ) override;
    virtual void SAL_CALL removeVetoableChangeListener( const OUString& PropertyName, const css::uno::Reference< css::beans::XVetoableChangeListener >& aListener ) override;

    // css::lang::XMultiServiceFactory
    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL createInstance( const OUString& aServiceSpecifier ) override;
    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL createInstanceWithArguments(
        const OUString& ServiceSpecifier, const css::uno::Sequence< css::uno::Any >& Arguments ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getAvailableServiceNames() override;

    // for ExplicitValueProvider
    // ____ XUnoTunnel ___
    virtual ::sal_Int64 SAL_CALL getSomething( const css::uno::Sequence< ::sal_Int8 >& aIdentifier ) override;

    // XDumper
    virtual OUString SAL_CALL dump() override;

    void setViewDirty();

    /// See sfx2::XmlDump::dumpAsXml().
    void dumpAsXml(xmlTextWriterPtr pWriter) const override;

private: //methods
    void createShapes();
    void createShapes2D( const css::awt::Size& rPageSize );
    bool createAxisTitleShapes2D( CreateShapeParam2D& rParam, const css::awt::Size& rPageSize, bool bHasRelativeSize );
    void getMetaFile( const css::uno::Reference< css::io::XOutputStream >& xOutStream
                      , bool bUseHighContrast );
    SdrPage* getSdrPage();

    void impl_deleteCoordinateSystems();
    void impl_notifyModeChangeListener( const OUString& rNewMode );

    void impl_refreshAddIn();

    void impl_updateView( bool bCheckLockedCtrler = true );

    css::awt::Rectangle impl_createDiagramAndContent( const CreateShapeParam2D& rParam, const css::awt::Size& rPageSize );

    DECL_LINK( UpdateTimeBased, Timer*, void );

private: //member
    ::osl::Mutex m_aMutex;

    css::uno::Reference< css::uno::XComponentContext>
            m_xCC;

    chart::ChartModel& mrChartModel;

    css::uno::Reference< css::lang::XMultiServiceFactory>
            m_xShapeFactory;
    rtl::Reference<SvxDrawPage>
            m_xDrawPage;
    rtl::Reference<SvxShapeGroupAnyD>
            mxRootShape;

    css::uno::Reference< css::uno::XInterface > m_xDashTable;
    css::uno::Reference< css::uno::XInterface > m_xGradientTable;
    css::uno::Reference< css::uno::XInterface > m_xHatchTable;
    css::uno::Reference< css::uno::XInterface > m_xBitmapTable;
    css::uno::Reference< css::uno::XInterface > m_xTransGradientTable;
    css::uno::Reference< css::uno::XInterface > m_xMarkerTable;

    std::shared_ptr< DrawModelWrapper > m_pDrawModelWrapper;

    std::vector< std::unique_ptr<VCoordinateSystem> > m_aVCooSysList;

    comphelper::OMultiTypeInterfaceContainerHelper2
                        m_aListenerContainer;

    bool m_bViewDirty; //states whether the view needs to be rebuild
    bool m_bInViewUpdate;
    bool m_bViewUpdatePending;
    bool m_bRefreshAddIn;

    //better performance for big data
    css::awt::Size m_aPageResolution;
    bool m_bPointsWereSkipped;

    //#i75867# poor quality of ole's alternative view with 3D scenes and zoomfactors besides 100%
    sal_Int32 m_nScaleXNumerator;
    sal_Int32 m_nScaleXDenominator;
    sal_Int32 m_nScaleYNumerator;
    sal_Int32 m_nScaleYDenominator;

    bool m_bSdrViewIsInEditMode;

    css::awt::Rectangle m_aResultingDiagramRectangleExcludingAxes;

    TimeBasedInfo maTimeBased;
    osl::Mutex maTimeMutex;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
