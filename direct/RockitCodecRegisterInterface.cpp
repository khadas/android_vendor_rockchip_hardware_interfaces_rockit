/*

 * Copyright 2022 Rockchip Electronics Co. LTD
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
#define LOG_TAG "RockitCodecRegisterInterface"

#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <utils/Log.h>
#include <utils/Mutex.h>
#include <cutils/properties.h>
#include "RTLibDefine.h"
#include "RockitCodecRegisterInterface.h"

namespace android {

static android::Mutex gRegisterCodecLock;
static RockitRegisterDecoderFunc *gRegisterCodecOpts = RT_NULL;
static const char *gRegisterLibName = ROCKIT_PLAYER_LIB_NAME;
static void *libHandler = RT_NULL;
static int gRegisterRefNum = 0;

static RockitRegisterDecoderFunc* dlsymRegisterCodec(void *handler) {
    void *opts = malloc(sizeof(RockitRegisterDecoderFunc));
    RockitRegisterDecoderFunc *registerOpts = reinterpret_cast<RockitRegisterDecoderFunc *>(opts);
    registerOpts->registerDecoder = (RockitRegisterDecoder)dlsym(handler, "RockitRegisterDecoder");

    if (registerOpts->registerDecoder == RT_NULL) {
        ALOGE("dlsym for register decoder failed, dlerror: %s", dlerror());
    }

    registerOpts->unRegisterDecoder = (RockitUnRegisterDecoder)dlsym(handler, "RockitUnRegisterDecoder");
    if (registerOpts->unRegisterDecoder == RT_NULL) {
        ALOGE("dlsym for unregister decoder failed, dlerror: %s", dlerror());;
    }
    return registerOpts;
}

static RT_RET initRegisterCodecCtxOpts() {
    Mutex::Autolock autoLock(gRegisterCodecLock);

    gRegisterRefNum++;
    if (gRegisterCodecOpts == RT_NULL) {
        libHandler = dlopen(gRegisterLibName, RTLD_LAZY);
        if (RT_NULL == libHandler) {
            ALOGE("failed to load library(%s) error: %s", gRegisterLibName, strerror(errno));
            return RT_ERR_BAD;
        }
        gRegisterCodecOpts = dlsymRegisterCodec(libHandler);
    }
    return RT_OK;
}

static RT_RET deinitRegisterCodecCtxOpts() {
    Mutex::Autolock autoLock(gRegisterCodecLock);
    INT32 ret = 0;

    gRegisterRefNum--;
    if (gRegisterRefNum > 0) {
        return ret;
    }
    if (libHandler) {
        ret = dlclose(libHandler);
    }

    if (gRegisterCodecOpts) {
        free(gRegisterCodecOpts);
    }
    return (ret == 0) ? RT_OK : RT_ERR_BAD;;
}

RockitCodecRegisterInterface::RockitCodecRegisterInterface() {
}

RockitCodecRegisterInterface::~RockitCodecRegisterInterface() {
}

RT_RET RockitCodecRegisterInterface::registerCodec(int32_t *ps32Handle, const RTAdecDecoder *pstDecoder) {
    RT_RET ret = RT_OK;

    initRegisterCodecCtxOpts();
    if (gRegisterCodecOpts && gRegisterCodecOpts->registerDecoder) {
        ret = gRegisterCodecOpts->registerDecoder(ps32Handle, pstDecoder);
    }
    return ret;
}

RT_RET RockitCodecRegisterInterface::unRegisterCodec(int32_t s32Handle) {
    RT_RET ret = RT_OK;

    if (gRegisterCodecOpts && gRegisterCodecOpts->unRegisterDecoder) {
        ret = gRegisterCodecOpts->unRegisterDecoder(s32Handle);
    }
    deinitRegisterCodecCtxOpts();
    return ret;
}

}
