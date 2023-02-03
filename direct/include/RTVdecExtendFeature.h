#ifndef __RKVDEC_EXTEND_FEATURE_H_
#define __RKVDEC_EXTEND_FEATURE_H_

#include <ui/GraphicBuffer.h>
#include "RTSurfaceInterface.h"

namespace android
{
class RTVdecExtendFeature
{
public:
    static int configFrameHdrDynamicMeta(buffer_handle_t hnd, int64_t offset);
};
}

#endif