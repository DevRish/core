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

#include <BaseCoordinateSystem.hxx>
#include <PropertyHelper.hxx>
#include <UserDefinedProperties.hxx>
#include <CloneHelper.hxx>
#include <ModifyListenerHelper.hxx>
#include <Axis.hxx>
#include <ChartType.hxx>
#include <com/sun/star/chart2/AxisType.hpp>
#include <com/sun/star/container/NoSuchElementException.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <o3tl/safeint.hxx>
#include <tools/diagnose_ex.h>

#include <algorithm>

#include <com/sun/star/beans/PropertyAttribute.hpp>

using namespace ::com::sun::star;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::beans::Property;

namespace
{
enum
{
    PROP_COORDINATESYSTEM_SWAPXANDYAXIS
};

void lcl_AddPropertiesToVector(
    std::vector< Property > & rOutProperties )
{
    rOutProperties.emplace_back( "SwapXAndYAxis",
                  PROP_COORDINATESYSTEM_SWAPXANDYAXIS,
                  cppu::UnoType<bool>::get(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID );
}

struct StaticCooSysDefaults_Initializer
{
    ::chart::tPropertyValueMap* operator()()
    {
        static ::chart::tPropertyValueMap aStaticDefaults;
        ::chart::PropertyHelper::setPropertyValueDefault( aStaticDefaults, PROP_COORDINATESYSTEM_SWAPXANDYAXIS, false );
        return &aStaticDefaults;
    }
};

struct StaticCooSysDefaults : public rtl::StaticAggregate< ::chart::tPropertyValueMap, StaticCooSysDefaults_Initializer >
{
};

struct StaticCooSysInfoHelper_Initializer
{
    ::cppu::OPropertyArrayHelper* operator()()
    {
        static ::cppu::OPropertyArrayHelper aPropHelper( lcl_GetPropertySequence() );
        return &aPropHelper;
    }

private:
    static Sequence< Property > lcl_GetPropertySequence()
    {
        std::vector< css::beans::Property > aProperties;
        lcl_AddPropertiesToVector( aProperties );
        ::chart::UserDefinedProperties::AddPropertiesToVector( aProperties );

        std::sort( aProperties.begin(), aProperties.end(),
                     ::chart::PropertyNameLess() );

        return comphelper::containerToSequence( aProperties );
    }
};

struct StaticCooSysInfoHelper : public rtl::StaticAggregate< ::cppu::OPropertyArrayHelper, StaticCooSysInfoHelper_Initializer >
{
};

struct StaticCooSysInfo_Initializer
{
    uno::Reference< beans::XPropertySetInfo >* operator()()
    {
        static uno::Reference< beans::XPropertySetInfo > xPropertySetInfo(
            ::cppu::OPropertySetHelper::createPropertySetInfo(*StaticCooSysInfoHelper::get() ) );
        return &xPropertySetInfo;
    }
};

struct StaticCooSysInfo : public rtl::StaticAggregate< uno::Reference< beans::XPropertySetInfo >, StaticCooSysInfo_Initializer >
{
};

} // anonymous namespace

namespace chart
{

BaseCoordinateSystem::BaseCoordinateSystem(
    sal_Int32 nDimensionCount /* = 2 */ ) :
        ::property::OPropertySet( m_aMutex ),
        m_xModifyEventForwarder( new ModifyEventForwarder() ),
        m_nDimensionCount( nDimensionCount )
 {
    m_aAllAxis.resize( m_nDimensionCount );
    for( sal_Int32 nN=0; nN<m_nDimensionCount; nN++ )
    {
        m_aAllAxis[nN].resize( 1 );
        rtl::Reference< Axis > xAxis( new Axis );
        m_aAllAxis[nN][0] = xAxis;

        ModifyListenerHelper::addListenerToAllElements( m_aAllAxis[nN], m_xModifyEventForwarder );
        chart2::ScaleData aScaleData( xAxis->getScaleData() );
        if(nN==0)
        {
            aScaleData.AxisType = chart2::AxisType::CATEGORY;
        }
        else if( nN==1)
        {
            aScaleData.AxisType = chart2::AxisType::REALNUMBER;
        }
        else if( nN==2)
        {
            aScaleData.AxisType = chart2::AxisType::SERIES;
        }
        xAxis->setScaleData( aScaleData );
    }

    setFastPropertyValue_NoBroadcast( PROP_COORDINATESYSTEM_SWAPXANDYAXIS, uno::Any( false ));
}

// explicit
BaseCoordinateSystem::BaseCoordinateSystem(
    const BaseCoordinateSystem & rSource ) :
        impl::BaseCoordinateSystem_Base(rSource),
        ::property::OPropertySet( rSource, m_aMutex ),
    m_xModifyEventForwarder( new ModifyEventForwarder() ),
    m_nDimensionCount( rSource.m_nDimensionCount )
{
    m_aAllAxis.resize(rSource.m_aAllAxis.size());
    tAxisVecVecType::size_type nN=0;
    for( nN=0; nN<m_aAllAxis.size(); nN++ )
        CloneHelper::CloneRefVector( rSource.m_aAllAxis[nN], m_aAllAxis[nN] );
    for (const auto & rxChartType : rSource.m_aChartTypes)
        m_aChartTypes.push_back(rxChartType->cloneChartType());

    for( nN=0; nN<m_aAllAxis.size(); nN++ )
        ModifyListenerHelper::addListenerToAllElements( m_aAllAxis[nN], m_xModifyEventForwarder );
    for (const auto & rxChartType : m_aChartTypes)
        rxChartType->addModifyListener( m_xModifyEventForwarder );
}

BaseCoordinateSystem::~BaseCoordinateSystem()
{
    try
    {
        for(const tAxisVecVecType::value_type & i : m_aAllAxis)
            ModifyListenerHelper::removeListenerFromAllElements( i, m_xModifyEventForwarder );
        for (const auto & rxChartType : m_aChartTypes)
            rxChartType->removeModifyListener( m_xModifyEventForwarder );
    }
    catch( const uno::Exception & )
    {
        DBG_UNHANDLED_EXCEPTION("chart2" );
    }
}

// ____ XCoordinateSystem ____
sal_Int32 SAL_CALL BaseCoordinateSystem::getDimension()
{
    return m_nDimensionCount;
}

void SAL_CALL BaseCoordinateSystem::setAxisByDimension(
    sal_Int32 nDimensionIndex,
    const Reference< chart2::XAxis >& xAxis,
    sal_Int32 nIndex )
{
    if( nDimensionIndex < 0 || nDimensionIndex >= getDimension() )
        throw lang::IndexOutOfBoundsException();

    if( nIndex < 0 )
        throw lang::IndexOutOfBoundsException();

    if( m_aAllAxis[ nDimensionIndex ].size() < o3tl::make_unsigned( nIndex+1 ))
    {
        m_aAllAxis[ nDimensionIndex ].resize( nIndex+1 );
        m_aAllAxis[ nDimensionIndex ][nIndex] = nullptr;
    }

    rtl::Reference< Axis > xOldAxis( m_aAllAxis[ nDimensionIndex ][nIndex] );
    if( xOldAxis.is())
        ModifyListenerHelper::removeListener( xOldAxis, m_xModifyEventForwarder );
    m_aAllAxis[ nDimensionIndex ][nIndex] = dynamic_cast<Axis*>(xAxis.get());
    if( xAxis.is())
        ModifyListenerHelper::addListener( xAxis, m_xModifyEventForwarder );
    fireModifyEvent();
}

void BaseCoordinateSystem::setAxisByDimension(
    sal_Int32 nDimensionIndex,
    const rtl::Reference< Axis >& xAxis,
    sal_Int32 nIndex )
{
    if( nDimensionIndex < 0 || nDimensionIndex >= getDimension() )
        throw lang::IndexOutOfBoundsException();

    if( nIndex < 0 )
        throw lang::IndexOutOfBoundsException();

    if( m_aAllAxis[ nDimensionIndex ].size() < o3tl::make_unsigned( nIndex+1 ))
    {
        m_aAllAxis[ nDimensionIndex ].resize( nIndex+1 );
        m_aAllAxis[ nDimensionIndex ][nIndex] = nullptr;
    }

    rtl::Reference< Axis > xOldAxis( m_aAllAxis[ nDimensionIndex ][nIndex] );
    if( xOldAxis.is())
        ModifyListenerHelper::removeListener( xOldAxis, m_xModifyEventForwarder );
    m_aAllAxis[ nDimensionIndex ][nIndex] = xAxis;
    if( xAxis.is())
        ModifyListenerHelper::addListener( xAxis, m_xModifyEventForwarder );
    fireModifyEvent();
}

Reference< chart2::XAxis > SAL_CALL BaseCoordinateSystem::getAxisByDimension(
            sal_Int32 nDimensionIndex, sal_Int32 nAxisIndex )
{
    if( nDimensionIndex < 0 || nDimensionIndex >= getDimension() )
        throw lang::IndexOutOfBoundsException();

    OSL_ASSERT( m_aAllAxis.size() == static_cast< size_t >( getDimension()));

    if( nAxisIndex < 0 || nAxisIndex > getMaximumAxisIndexByDimension(nDimensionIndex) )
        throw lang::IndexOutOfBoundsException();

    return m_aAllAxis[ nDimensionIndex ][nAxisIndex];
}

rtl::Reference< Axis > BaseCoordinateSystem::getAxisByDimension2(
            sal_Int32 nDimensionIndex, sal_Int32 nAxisIndex ) const
{
    if( nDimensionIndex < 0 || nDimensionIndex >= m_nDimensionCount )
        throw lang::IndexOutOfBoundsException();

    OSL_ASSERT( m_aAllAxis.size() == static_cast< size_t >( m_nDimensionCount));

    if( nAxisIndex < 0 || nAxisIndex > static_cast<sal_Int32>(m_aAllAxis[ nDimensionIndex ].size()) )
        throw lang::IndexOutOfBoundsException();

    return m_aAllAxis[ nDimensionIndex ][nAxisIndex];
}

sal_Int32 SAL_CALL BaseCoordinateSystem::getMaximumAxisIndexByDimension( sal_Int32 nDimensionIndex )
{
    if( nDimensionIndex < 0 || nDimensionIndex >= getDimension() )
        throw lang::IndexOutOfBoundsException();

    OSL_ASSERT( m_aAllAxis.size() == static_cast< size_t >( getDimension()));

    sal_Int32 nRet = m_aAllAxis[ nDimensionIndex ].size();
    if(nRet)
        nRet-=1;

    return nRet;
}

// ____ XChartTypeContainer ____
void SAL_CALL BaseCoordinateSystem::addChartType( const Reference< chart2::XChartType >& aChartType )
{
    auto pChartType = dynamic_cast<ChartType*>(aChartType.get());
    assert(pChartType);

    if( std::find( m_aChartTypes.begin(), m_aChartTypes.end(), pChartType )
        != m_aChartTypes.end())
        throw lang::IllegalArgumentException("type not found", static_cast<cppu::OWeakObject*>(this), 1);

    m_aChartTypes.push_back( pChartType );
    ModifyListenerHelper::addListener( aChartType, m_xModifyEventForwarder );
    fireModifyEvent();
}

void SAL_CALL BaseCoordinateSystem::removeChartType( const Reference< chart2::XChartType >& aChartType )
{
    auto pChartType = dynamic_cast<ChartType*>(aChartType.get());
    assert(pChartType);
    auto aIt( std::find( m_aChartTypes.begin(), m_aChartTypes.end(), pChartType ));
    if( aIt == m_aChartTypes.end())
        throw container::NoSuchElementException(
            "The given chart type is no element of the container",
            static_cast< uno::XWeak * >( this ));

    m_aChartTypes.erase( aIt );
    ModifyListenerHelper::removeListener( aChartType, m_xModifyEventForwarder );
    fireModifyEvent();
}

Sequence< Reference< chart2::XChartType > > SAL_CALL BaseCoordinateSystem::getChartTypes()
{
    return comphelper::containerToSequence< Reference< chart2::XChartType > >( m_aChartTypes );
}

void SAL_CALL BaseCoordinateSystem::setChartTypes( const Sequence< Reference< chart2::XChartType > >& aChartTypes )
{
    for (auto const & aChartType : m_aChartTypes)
        aChartType->removeModifyListener( m_xModifyEventForwarder );
    m_aChartTypes.clear();
    for (auto const & aChartType : aChartTypes)
    {
        auto pChartType = dynamic_cast<ChartType*>(aChartType.get());
        assert(pChartType);
        m_aChartTypes.push_back(pChartType);
        pChartType->addModifyListener( m_xModifyEventForwarder );
    }
    fireModifyEvent();
}

void BaseCoordinateSystem::setChartTypes( const std::vector< rtl::Reference< ChartType > >& aChartTypes )
{
    for (auto const & aChartType : m_aChartTypes)
        aChartType->removeModifyListener( m_xModifyEventForwarder );
    m_aChartTypes = aChartTypes;
    for (auto const & aChartType : m_aChartTypes)
        aChartType->addModifyListener( m_xModifyEventForwarder );
    fireModifyEvent();
}

// ____ XModifyBroadcaster ____
void SAL_CALL BaseCoordinateSystem::addModifyListener( const Reference< util::XModifyListener >& aListener )
{
    m_xModifyEventForwarder->addModifyListener( aListener );
}

void SAL_CALL BaseCoordinateSystem::removeModifyListener( const Reference< util::XModifyListener >& aListener )
{
    m_xModifyEventForwarder->removeModifyListener( aListener );
}

// ____ XModifyListener ____
void SAL_CALL BaseCoordinateSystem::modified( const lang::EventObject& aEvent )
{
    m_xModifyEventForwarder->modified( aEvent );
}

// ____ XEventListener (base of XModifyListener) ____
void SAL_CALL BaseCoordinateSystem::disposing( const lang::EventObject& /* Source */ )
{
    // nothing
}

// ____ OPropertySet ____
void BaseCoordinateSystem::firePropertyChangeEvent()
{
    fireModifyEvent();
}

void BaseCoordinateSystem::fireModifyEvent()
{
    m_xModifyEventForwarder->modified( lang::EventObject( static_cast< uno::XWeak* >( this )));
}

// ____ OPropertySet ____
void BaseCoordinateSystem::GetDefaultValue( sal_Int32 nHandle, uno::Any& rAny ) const
{
    const tPropertyValueMap& rStaticDefaults = *StaticCooSysDefaults::get();
    tPropertyValueMap::const_iterator aFound( rStaticDefaults.find( nHandle ) );
    if( aFound == rStaticDefaults.end() )
        rAny.clear();
    else
        rAny = (*aFound).second;
}

// ____ OPropertySet ____
::cppu::IPropertyArrayHelper & SAL_CALL BaseCoordinateSystem::getInfoHelper()
{
    return *StaticCooSysInfoHelper::get();
}

// ____ XPropertySet ____
Reference< beans::XPropertySetInfo > SAL_CALL BaseCoordinateSystem::getPropertySetInfo()
{
    return *StaticCooSysInfo::get();
}

using impl::BaseCoordinateSystem_Base;

IMPLEMENT_FORWARD_XINTERFACE2( BaseCoordinateSystem, BaseCoordinateSystem_Base, ::property::OPropertySet )
IMPLEMENT_FORWARD_XTYPEPROVIDER2( BaseCoordinateSystem, BaseCoordinateSystem_Base, ::property::OPropertySet )

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
