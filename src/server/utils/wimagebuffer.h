// Copyright (C) 2023 rewine <luhongxu@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <qwbufferinterface.h>
#include <QImage>

extern "C" {
#include <wlr/types/wlr_buffer.h>
}

WAYLIB_SERVER_BEGIN_NAMESPACE

class WImageBufferImpl : public QWLRoots::QWBufferInterface
{
public:
    WImageBufferImpl(const QImage &bufferImage);
    ~WImageBufferImpl();

    bool beginDataPtrAccess(uint32_t flags, void **data, uint32_t *format, size_t *stride) override;
    void endDataPtrAccess() override;

private:
    QImage image;
};

WAYLIB_SERVER_END_NAMESPACE
