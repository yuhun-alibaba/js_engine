/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(WEBGPU)

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(COCOA)
OBJC_CLASS MTLDepthStencilState;
#endif

namespace WebCore {

class GPUDevice;
class GPUDepthStencilDescriptor;

class GPUDepthStencilState : public RefCounted<GPUDepthStencilState> {
public:
    static RefPtr<GPUDepthStencilState> create(GPUDevice*, GPUDepthStencilDescriptor*);
    WEBCORE_EXPORT ~GPUDepthStencilState();

    WEBCORE_EXPORT String label() const;
    WEBCORE_EXPORT void setLabel(const String&);

#if PLATFORM(COCOA)
    WEBCORE_EXPORT MTLDepthStencilState *platformDepthStencilState();
#endif

private:
    GPUDepthStencilState(GPUDevice*, GPUDepthStencilDescriptor*);
    
#if PLATFORM(COCOA)
    RetainPtr<MTLDepthStencilState> m_depthStencilState;
#endif
};
    
} // namespace WebCore
#endif
