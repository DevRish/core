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

#include <sal/config.h>
#include <sal/log.hxx>

#include <algorithm>
#include <map>
#include <vector>

#include "vclhelperbufferdevice.hxx"
#include <basegfx/range/b2drange.hxx>
#include <vcl/bitmapex.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <tools/stream.hxx>
#include <vcl/timer.hxx>
#include <cppuhelper/basemutex.hxx>
#include <vcl/lazydelete.hxx>
#include <vcl/dibtools.hxx>
#include <vcl/skia/SkiaHelper.hxx>

// buffered VDev usage

namespace
{
class VDevBuffer : public Timer, protected cppu::BaseMutex
{
private:
    struct Entry
    {
        VclPtr<VirtualDevice> buf;
        bool isTransparent = false;
        Entry(const VclPtr<VirtualDevice>& vdev, bool bTransparent)
            : buf(vdev)
            , isTransparent(bTransparent)
        {
        }
    };

    // available buffers
    std::vector<Entry> maFreeBuffers;

    // allocated/used buffers (remembered to allow deleting them in destructor)
    std::vector<Entry> maUsedBuffers;

    // remember what outputdevice was the template passed to VirtualDevice::Create
    // so we can test if that OutputDevice was disposed before reusing a
    // virtualdevice because that isn't safe to do at least for Gtk2
    std::map<VclPtr<VirtualDevice>, VclPtr<OutputDevice>> maDeviceTemplates;

    static bool isSizeSuitable(const VclPtr<VirtualDevice>& device, const Size& size);

public:
    VDevBuffer();
    virtual ~VDevBuffer() override;

    VclPtr<VirtualDevice> alloc(OutputDevice& rOutDev, const Size& rSizePixel, bool bTransparent);
    void free(VirtualDevice& rDevice);

    // Timer virtuals
    virtual void Invoke() override;
};

VDevBuffer::VDevBuffer()
    : Timer("drawinglayer::VDevBuffer via Invoke()")
{
    SetTimeout(10L * 1000L); // ten seconds
}

VDevBuffer::~VDevBuffer()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    Stop();

    while (!maFreeBuffers.empty())
    {
        maFreeBuffers.back().buf.disposeAndClear();
        maFreeBuffers.pop_back();
    }

    while (!maUsedBuffers.empty())
    {
        maUsedBuffers.back().buf.disposeAndClear();
        maUsedBuffers.pop_back();
    }
}

bool VDevBuffer::isSizeSuitable(const VclPtr<VirtualDevice>& device, const Size& rSizePixel)
{
    if (device->GetOutputWidthPixel() >= rSizePixel.getWidth()
        && device->GetOutputHeightPixel() >= rSizePixel.getHeight())
    {
        bool requireSmall = false;
#if defined(UNX)
        // HACK: See the small size handling in SvpSalVirtualDevice::CreateSurface().
        // Make sure to not reuse a larger device when a small one should be preferred.
        if (device->GetRenderBackendName() == "svp")
            requireSmall = true;
#endif
        // The same for Skia, see renderMethodToUseForSize().
        if (SkiaHelper::isVCLSkiaEnabled())
            requireSmall = true;
        if (requireSmall)
        {
            if (rSizePixel.getWidth() <= 32 && rSizePixel.getHeight() <= 32
                && (device->GetOutputWidthPixel() > 32 || device->GetOutputHeightPixel() > 32))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

VclPtr<VirtualDevice> VDevBuffer::alloc(OutputDevice& rOutDev, const Size& rSizePixel,
                                        bool bTransparent)
{
    ::osl::MutexGuard aGuard(m_aMutex);
    VclPtr<VirtualDevice> pRetval;

    sal_Int32 nBits = rOutDev.GetBitCount();

    bool bOkay(false);
    if (!maFreeBuffers.empty())
    {
        auto aFound(maFreeBuffers.end());

        for (auto a = maFreeBuffers.begin(); a != maFreeBuffers.end(); ++a)
        {
            assert(a->buf && "Empty pointer in VDevBuffer (!)");

            if (nBits == a->buf->GetBitCount() && bTransparent == a->isTransparent)
            {
                // candidate is valid due to bit depth
                if (aFound != maFreeBuffers.end())
                {
                    // already found
                    if (bOkay)
                    {
                        // found is valid
                        const bool bCandidateOkay = isSizeSuitable(a->buf, rSizePixel);

                        if (bCandidateOkay)
                        {
                            // found and candidate are valid
                            const sal_uLong aSquare(aFound->buf->GetOutputWidthPixel()
                                                    * aFound->buf->GetOutputHeightPixel());
                            const sal_uLong aCandidateSquare(a->buf->GetOutputWidthPixel()
                                                             * a->buf->GetOutputHeightPixel());

                            if (aCandidateSquare < aSquare)
                            {
                                // candidate is valid and smaller, use it
                                aFound = a;
                            }
                        }
                        else
                        {
                            // found is valid, candidate is not. Keep found
                        }
                    }
                    else
                    {
                        // found is invalid, use candidate
                        aFound = a;
                        bOkay = isSizeSuitable(aFound->buf, rSizePixel);
                    }
                }
                else
                {
                    // none yet, use candidate
                    aFound = a;
                    bOkay = isSizeSuitable(aFound->buf, rSizePixel);
                }
            }
        }

        if (aFound != maFreeBuffers.end())
        {
            pRetval = aFound->buf;
            maFreeBuffers.erase(aFound);
        }
    }

    if (pRetval)
    {
        // found a suitable cached virtual device, but the
        // outputdevice it was based on has been disposed,
        // drop it and create a new one instead as reusing
        // such devices is unsafe under at least Gtk2
        if (maDeviceTemplates[pRetval]->isDisposed())
        {
            maDeviceTemplates.erase(pRetval);
            pRetval.disposeAndClear();
        }
        else
        {
            if (bOkay)
            {
                pRetval->Erase(pRetval->PixelToLogic(
                    tools::Rectangle(0, 0, rSizePixel.getWidth(), rSizePixel.getHeight())));
            }
            else
            {
                pRetval->SetOutputSizePixel(rSizePixel, true);
            }
        }
    }

    // no success yet, create new buffer
    if (!pRetval)
    {
        pRetval = VclPtr<VirtualDevice>::Create(rOutDev, DeviceFormat::DEFAULT,
                                                bTransparent ? DeviceFormat::DEFAULT
                                                             : DeviceFormat::NONE);
        maDeviceTemplates[pRetval] = &rOutDev;
        pRetval->SetOutputSizePixel(rSizePixel, true);
    }
    else
    {
        // reused, reset some values
        pRetval->SetMapMode();
        pRetval->SetRasterOp(RasterOp::OverPaint);
    }

    // remember allocated buffer
    maUsedBuffers.emplace_back(pRetval, bTransparent);

    return pRetval;
}

void VDevBuffer::free(VirtualDevice& rDevice)
{
    ::osl::MutexGuard aGuard(m_aMutex);
    const auto aUsedFound
        = std::find_if(maUsedBuffers.begin(), maUsedBuffers.end(),
                       [&rDevice](const Entry& el) { return el.buf == &rDevice; });
    SAL_WARN_IF(aUsedFound == maUsedBuffers.end(), "drawinglayer",
                "OOps, non-registered buffer freed (!)");
    if (aUsedFound != maUsedBuffers.end())
    {
        maFreeBuffers.emplace_back(*aUsedFound);
        maUsedBuffers.erase(aUsedFound);
        SAL_WARN_IF(maFreeBuffers.size() > 1000, "drawinglayer",
                    "excessive cached buffers, " << maFreeBuffers.size() << " entries!");
    }
    Start();
}

void VDevBuffer::Invoke()
{
    ::osl::MutexGuard aGuard(m_aMutex);

    while (!maFreeBuffers.empty())
    {
        auto aLastOne = maFreeBuffers.back();
        maDeviceTemplates.erase(aLastOne.buf);
        aLastOne.buf.disposeAndClear();
        maFreeBuffers.pop_back();
    }
}
}

// support for rendering Bitmap and BitmapEx contents

namespace drawinglayer
{
// static global VDev buffer for the VclProcessor2D's (VclMetafileProcessor2D and VclPixelProcessor2D)
VDevBuffer& getVDevBuffer()
{
    // secure global instance with Vcl's safe destroyer of external (seen by
    // library base) stuff, the remembered VDevs need to be deleted before
    // Vcl's deinit
    static vcl::DeleteOnDeinit<VDevBuffer> aVDevBuffer{};
    return *aVDevBuffer.get();
}

impBufferDevice::impBufferDevice(OutputDevice& rOutDev, const basegfx::B2DRange& rRange)
    : mrOutDev(rOutDev)
    , mpContent(nullptr)
    , mpAlpha(nullptr)
{
    basegfx::B2DRange aRangePixel(rRange);
    aRangePixel.transform(mrOutDev.GetViewTransformation());
    const ::tools::Rectangle aRectPixel(floor(aRangePixel.getMinX()), floor(aRangePixel.getMinY()),
                                        ceil(aRangePixel.getMaxX()), ceil(aRangePixel.getMaxY()));
    const Point aEmptyPoint;
    maDestPixel = ::tools::Rectangle(aEmptyPoint, mrOutDev.GetOutputSizePixel());
    maDestPixel.Intersection(aRectPixel);

    if (!isVisible())
        return;

    mpContent = getVDevBuffer().alloc(mrOutDev, maDestPixel.GetSize(), true);

    // #i93485# assert when copying from window to VDev is used
    SAL_WARN_IF(
        mrOutDev.GetOutDevType() == OUTDEV_WINDOW, "drawinglayer",
        "impBufferDevice render helper: Copying from Window to VDev, this should be avoided (!)");

    MapMode aNewMapMode(mrOutDev.GetMapMode());

    const Point aLogicTopLeft(mrOutDev.PixelToLogic(maDestPixel.TopLeft()));
    aNewMapMode.SetOrigin(Point(-aLogicTopLeft.X(), -aLogicTopLeft.Y()));

    mpContent->SetMapMode(aNewMapMode);

    // copy AA flag for new target
    mpContent->SetAntialiasing(mrOutDev.GetAntialiasing());

    // copy RasterOp (e.g. may be RasterOp::Xor on destination)
    mpContent->SetRasterOp(mrOutDev.GetRasterOp());
}

impBufferDevice::~impBufferDevice()
{
    if (mpContent)
    {
        getVDevBuffer().free(*mpContent);
    }

    if (mpAlpha)
    {
        getVDevBuffer().free(*mpAlpha);
    }
}

void impBufferDevice::paint(double fTrans)
{
    if (!isVisible())
        return;

    const Point aEmptyPoint;
    const Size aSizePixel(maDestPixel.GetSize());
    const bool bWasEnabledDst(mrOutDev.IsMapModeEnabled());
#ifdef DBG_UTIL
    static bool bDoSaveForVisualControl(false); // loplugin:constvars:ignore
#endif

    mrOutDev.EnableMapMode(false);
    mpContent->EnableMapMode(false);

#ifdef DBG_UTIL
    if (bDoSaveForVisualControl)
    {
        SvFileStream aNew(
#ifdef _WIN32
            "c:\\content.bmp",
#else
            "~/content.bmp",
#endif
            StreamMode::WRITE | StreamMode::TRUNC);
        Bitmap aContent(mpContent->GetBitmap(aEmptyPoint, aSizePixel));
        WriteDIB(aContent, aNew, false, true);
    }
#endif

    // during painting the buffer, disable evtl. set RasterOp (may be RasterOp::Xor)
    const RasterOp aOrigRasterOp(mrOutDev.GetRasterOp());
    mrOutDev.SetRasterOp(RasterOp::OverPaint);

    if (mpAlpha)
    {
        mpAlpha->EnableMapMode(false);
        AlphaMask aAlphaMask(mpAlpha->GetBitmap(aEmptyPoint, aSizePixel));

#ifdef DBG_UTIL
        if (bDoSaveForVisualControl)
        {
            SvFileStream aNew(
#ifdef _WIN32
                "c:\\transparence.bmp",
#else
                "~/transparence.bmp",
#endif
                StreamMode::WRITE | StreamMode::TRUNC);
            WriteDIB(aAlphaMask.GetBitmap(), aNew, false, true);
        }
#endif

        BitmapEx aContent(mpContent->GetBitmapEx(aEmptyPoint, aSizePixel));
        aAlphaMask.BlendWith(aContent.GetAlpha());
        mrOutDev.DrawBitmapEx(maDestPixel.TopLeft(), BitmapEx(aContent.GetBitmap(), aAlphaMask));
    }
    else if (0.0 != fTrans)
    {
        basegfx::B2DHomMatrix trans, scale;
        trans.translate(maDestPixel.TopLeft().X(), maDestPixel.TopLeft().Y());
        scale.scale(aSizePixel.Width(), aSizePixel.Height());
        mrOutDev.DrawTransformedBitmapEx(
            trans * scale, mpContent->GetBitmapEx(aEmptyPoint, aSizePixel), 1 - fTrans);
    }
    else
    {
        mrOutDev.DrawOutDev(maDestPixel.TopLeft(), aSizePixel, aEmptyPoint, aSizePixel, *mpContent);
    }

    mrOutDev.SetRasterOp(aOrigRasterOp);
    mrOutDev.EnableMapMode(bWasEnabledDst);
}

VirtualDevice& impBufferDevice::getContent()
{
    SAL_WARN_IF(!mpContent, "drawinglayer",
                "impBufferDevice: No content, check isVisible() before accessing (!)");
    return *mpContent;
}

VirtualDevice& impBufferDevice::getTransparence()
{
    SAL_WARN_IF(!mpContent, "drawinglayer",
                "impBufferDevice: No content, check isVisible() before accessing (!)");
    if (!mpAlpha)
    {
        mpAlpha = getVDevBuffer().alloc(mrOutDev, maDestPixel.GetSize(), false);
        mpAlpha->SetMapMode(mpContent->GetMapMode());

        // copy AA flag for new target; masking needs to be smooth
        mpAlpha->SetAntialiasing(mpContent->GetAntialiasing());
    }

    return *mpAlpha;
}
} // end of namespace drawinglayer

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
