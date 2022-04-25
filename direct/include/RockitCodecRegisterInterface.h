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

#ifndef ROCKIT_CODEC_REGISTER_INTERFACE_H
#define ROCKIT_CODEC_REGISTER_INTERFACE_H

#include "rt_error.h"
#include "rt_type.h"
#include "RTLibDefine.h"

namespace android {

typedef struct _RTAdecDecoder {
    RTCodecID enType;
    char aszName[17];
    // open decoder
    int32_t (*pfnOpenDecoder)(void *pDecoderAttr, void **ppDecoder);
    int32_t (*pfnDecodeFrm)(void *pDecoder, void *pParam);
    // get audio frames infor
    int32_t (*pfnGetFrmInfo)(void *pDecoder, void *pInfo);
    // close audio decoder
    int32_t (*pfnCloseDecoder)(void *pDecoder);
    // reset audio decoder
    int32_t (*pfnResetDecoder)(void *pDecoder);
} RTAdecDecoder;

typedef RT_RET (*RockitRegisterDecoder)(int32_t *ps32Handle, const RTAdecDecoder *pstDecoder);
typedef RT_RET (*RockitUnRegisterDecoder)(int32_t s32Handle);

typedef struct _RockitRegisterDecoderFunc {
    RockitRegisterDecoder registerDecoder;
    RockitUnRegisterDecoder unRegisterDecoder;
} RockitRegisterDecoderFunc;

class RockitCodecRegisterInterface {
 public:
    RockitCodecRegisterInterface();
    ~RockitCodecRegisterInterface();

    static RT_RET registerCodec(int32_t *ps32Handle, const RTAdecDecoder *pstDecoder);
    static RT_RET unRegisterCodec(int32_t s32Handle);

 private:
    RockitCodecRegisterInterface(const RockitCodecRegisterInterface &);
    RockitCodecRegisterInterface &operator=(const RockitCodecRegisterInterface &);
};

}  // namespace android

#endif  // ROCKIT_CODEC_REGISTER_INTERFACE_H
