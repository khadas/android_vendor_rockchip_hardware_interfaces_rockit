/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "RTSurfaceCallback"

#include <string.h>
#include <gralloc_priv_omx.h>
#include <ui/GraphicBufferAllocator.h>

#include "RTSurfaceCallback.h"
#include "RockitPlayer.h"
#include "sideband/RTSidebandWindow.h"

using namespace ::android;

RTSurfaceCallback::RTSurfaceCallback(const sp<IGraphicBufferProducer> &bufferProducer)
    : mTunnel(0),
      mSidebandHandle(NULL),
      mSidebandWindow(NULL) {
    mNativeWindow = new Surface(bufferProducer, true);
}

RTSurfaceCallback::~RTSurfaceCallback() {
    ALOGD("~RTSurfaceCallback(%p) construct", this);
    if (mSidebandHandle) {
        mSidebandWindow->freeBuffer(&mSidebandHandle);
    }
    if (mSidebandWindow.get()) {
        mSidebandWindow->release();
        mSidebandWindow.clear();
    }

    if (mNativeWindow.get() != NULL) {
        mNativeWindow.clear();
    }
}

INT32 RTSurfaceCallback::setNativeWindow(const sp<IGraphicBufferProducer> &bufferProducer) {
    if (bufferProducer.get() == NULL)
        return 0;

    if(getNativeWindow() == NULL) {
        mNativeWindow = new Surface(bufferProducer, true);
    } else {
        ALOGD("already set native window");
    }
    return 0;
}

INT32 RTSurfaceCallback::connect(INT32 mode) {
    ALOGV("%s %d in", __FUNCTION__, __LINE__);
    (void)mode;
    if (getNativeWindow() == NULL)
        return -1;

    return native_window_api_connect(mNativeWindow.get(), NATIVE_WINDOW_API_MEDIA);
}

INT32 RTSurfaceCallback::disconnect(INT32 mode) {
    ALOGV("%s %d in", __FUNCTION__, __LINE__);
    (void)mode;
    if (getNativeWindow() == NULL)
        return -1;

    return native_window_api_disconnect(mNativeWindow.get(), NATIVE_WINDOW_API_MEDIA);;
}

INT32 RTSurfaceCallback::allocateBuffer(RTNativeWindowBufferInfo *info) {
    INT32                       ret = 0;
    buffer_handle_t             bufferHandle = NULL;
    gralloc_private_handle_t    privHandle;
    ANativeWindowBuffer        *buf = NULL;

    memset(info, 0, sizeof(RTNativeWindowBufferInfo));
    if (mTunnel) {
        mSidebandWindow->allocateBuffer((buffer_handle_t *)&bufferHandle);
    } else {
        if (getNativeWindow() == NULL)
            return -1;

        ret = native_window_dequeue_buffer_and_wait(mNativeWindow.get(), &buf);
        if (buf) {
            bufferHandle = buf->handle;
        }

    }

    if (bufferHandle) {
        Rockchip_get_gralloc_private((UINT32 *)bufferHandle, &privHandle);

        if (mTunnel) {
            info->windowBuf = (void *)bufferHandle;
        } else {
            info->windowBuf = (void *)buf;
        }

        // use buffer_handle binder
        info->name = 0xFFFFFFFE;
        info->size = privHandle.size;
        info->dupFd = privHandle.share_fd;
    }

    return 0;
}

INT32 RTSurfaceCallback::freeBuffer(void *buf, INT32 fence) {
    ALOGV("%s %d buf=%p in", __FUNCTION__, __LINE__, buf);
    INT32 ret = 0;
    if (mTunnel) {
        ret = mSidebandWindow->freeBuffer((buffer_handle_t *)&buf);
    } else {
        if (getNativeWindow() == NULL)
            return -1;

        ret = mNativeWindow->cancelBuffer(mNativeWindow.get(), (ANativeWindowBuffer *)buf, fence);
    }

    return ret;
}

INT32 RTSurfaceCallback::remainBuffer(void *buf, INT32 fence) {
    ALOGV("%s %d buf=%p in", __FUNCTION__, __LINE__, buf);
    INT32 ret = 0;
    if (mTunnel) {
        ret = mSidebandWindow->remainBuffer((buffer_handle_t)buf);
    } else {
        if (getNativeWindow() == NULL)
            return -1;

        ret = mNativeWindow->cancelBuffer(mNativeWindow.get(), (ANativeWindowBuffer *)buf, fence);
    }

    return ret;
}

INT32 RTSurfaceCallback::queueBuffer(void *buf, INT32 fence) {
    ALOGV("%s %d buf=%p in", __FUNCTION__, __LINE__, buf);
    INT32 ret = 0;
    if (mTunnel) {
        ret = mSidebandWindow->queueBuffer((buffer_handle_t)buf);
    } else {
        if (getNativeWindow() == NULL)
            return -1;

        ret = mNativeWindow->queueBuffer(mNativeWindow.get(), (ANativeWindowBuffer *)buf, fence);
    }
    return ret;
}

INT32 RTSurfaceCallback::dequeueBuffer(void **buf) {
    ALOGV("%s %d in", __FUNCTION__, __LINE__);
    (void)buf;
    return 0;

}

INT32 RTSurfaceCallback::dequeueBufferAndWait(RTNativeWindowBufferInfo *info) {
    ALOGV("%s %d in", __FUNCTION__, __LINE__);
    INT32                       ret = 0;
    buffer_handle_t             bufferHandle = NULL;
    gralloc_private_handle_t    privHandle;
    ANativeWindowBuffer *buf = NULL;

    memset(info, 0, sizeof(RTNativeWindowBufferInfo));
    if (mTunnel) {
        mSidebandWindow->dequeueBuffer((buffer_handle_t *)&bufferHandle);
    } else {
        if (getNativeWindow() == NULL)
            return -1;

        ret = native_window_dequeue_buffer_and_wait(mNativeWindow.get(), &buf);
        if (buf) {
            bufferHandle = buf->handle;
        }
    }

    if (bufferHandle) {
        Rockchip_get_gralloc_private((UINT32 *)bufferHandle, &privHandle);

        if (mTunnel) {
            info->windowBuf = (void *)bufferHandle;
        } else {
            info->windowBuf = (void *)buf;
        }
        info->dupFd = privHandle.share_fd;
    }
    return ret;
}

INT32 RTSurfaceCallback::mmapBuffer(RTNativeWindowBufferInfo *info, void **ptr) {
    status_t err = OK;
    ANativeWindowBuffer *buf = NULL;
    void *tmpPtr = NULL;
    (void)ptr;

    if (info->windowBuf == NULL || ptr == NULL) {
        ALOGE("lockBuffer bad value, windowBuf=%p, &ptr=%p", info->windowBuf, ptr);
        return RT_ERR_VALUE;
    }

    if (mTunnel)
        return RT_ERR_UNSUPPORT;

    buf = static_cast<ANativeWindowBuffer *>(info->windowBuf);

    sp<GraphicBuffer> graphicBuffer(GraphicBuffer::from(buf));
    err = graphicBuffer->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, &tmpPtr);
    if (err != OK) {
        ALOGE("graphicBuffer lock failed err - %d", err);
        return RT_ERR_BAD;
    }

    *ptr = tmpPtr;

    return RT_OK;
}

INT32 RTSurfaceCallback::munmapBuffer(void **ptr, INT32 size, void *buf) {
    status_t err = OK;
    (void)ptr;
    (void)size;

    if (mTunnel)
        return RT_ERR_UNSUPPORT;

    sp<GraphicBuffer> graphicBuffer(
            GraphicBuffer::from(static_cast<ANativeWindowBuffer *>(buf)));
    err = graphicBuffer->unlock();
    if (err != OK) {
        ALOGE("graphicBuffer unlock failed err - %d", err);
        return RT_ERR_BAD;
    }

    return RT_OK;
}

INT32 RTSurfaceCallback::setCrop(
        INT32 left,
        INT32 top,
        INT32 right,
        INT32 bottom) {
        ALOGV("%s %d in crop(%d,%d,%d,%d)", __FUNCTION__, __LINE__, left, top, right, bottom);
        android_native_rect_t crop;

        crop.left = left;
        crop.top = top;
        crop.right = right;
        crop.bottom = bottom;
        if (mTunnel) {
            mSidebandWindow->setCrop(left, top, right, bottom);
        }

        if (getNativeWindow() == NULL)
            return -1;

        return native_window_set_crop(mNativeWindow.get(), &crop);
}

INT32 RTSurfaceCallback::setUsage(INT32 usage) {
    ALOGV("%s %d in usage=0x%x", __FUNCTION__, __LINE__, usage);
    if (getNativeWindow() == NULL)
        return -1;

    return native_window_set_usage(mNativeWindow.get(), usage);;
}

INT32 RTSurfaceCallback::setScalingMode(INT32 mode) {
    ALOGV("%s %d in", __FUNCTION__, __LINE__);
    if (getNativeWindow() == NULL)
        return -1;

    return native_window_set_scaling_mode(mNativeWindow.get(), mode);;
}

INT32 RTSurfaceCallback::setDataSpace(INT32 dataSpace) {
    ALOGV("%s %d in dataSpace=0x%x", __FUNCTION__, __LINE__, dataSpace);
    if (getNativeWindow() == NULL)
        return -1;

    return native_window_set_buffers_data_space(mNativeWindow.get(), (android_dataspace_t)dataSpace);
}

INT32 RTSurfaceCallback::setTransform(INT32 transform) {
    ALOGV("%s %d in", __FUNCTION__, __LINE__);
    if (getNativeWindow() == NULL)
        return -1;

    return native_window_set_buffers_transform(mNativeWindow.get(), transform);
}

INT32 RTSurfaceCallback::setSwapInterval(INT32 interval) {
    ALOGV("%s %d in", __FUNCTION__, __LINE__);
    (void)interval;
    return 0;
}

INT32 RTSurfaceCallback::setBufferCount(INT32 bufferCount) {
    ALOGV("%s %d in", __FUNCTION__, __LINE__);
    if (getNativeWindow() == NULL)
        return -1;

    return native_window_set_buffer_count(mNativeWindow.get(), bufferCount);
}

INT32 RTSurfaceCallback::setBufferGeometry(
        INT32 width,
        INT32 height,
        INT32 format) {
    ALOGV("%s %d in width=%d, height=%d, format=0x%x", __FUNCTION__, __LINE__, width, height, format);
    if (getNativeWindow() == NULL)
        return -1;

    native_window_set_buffers_dimensions(mNativeWindow.get(), width, height);
    native_window_set_buffers_format(mNativeWindow.get(), format);
    if (mTunnel) {
        mSidebandWindow->setBufferGeometry(width, height, format);
    }

    return 0;
}

INT32 RTSurfaceCallback::setSidebandStream(RTSidebandInfo info) {
    ALOGV("%s %d in", __FUNCTION__, __LINE__);

    buffer_handle_t             buffer = NULL;

    mSidebandWindow = new RTSidebandWindow();
    mSidebandWindow->init(info);
    mSidebandWindow->allocateSidebandHandle(&buffer);
    if (!buffer) {
        ALOGE("allocate buffer from sideband window failed!");
        return -1;
    }
    mSidebandHandle = buffer;
    mTunnel = 1;

    if (getNativeWindow() == NULL)
        return -1;

    return native_window_set_sideband_stream(mNativeWindow.get(), (native_handle_t *)buffer);
}

INT32 RTSurfaceCallback::query(INT32 cmd, INT32 *param) {
    ALOGV("%s %d in", __FUNCTION__, __LINE__);
    if (getNativeWindow() == NULL)
        return -1;

    return mNativeWindow->query(mNativeWindow.get(), cmd, param);

}

void* RTSurfaceCallback::getNativeWindow() {
    return (void *)mNativeWindow.get();
}

