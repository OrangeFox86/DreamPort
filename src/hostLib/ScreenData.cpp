// MIT License
//
// Copyright (c) 2022-2025 James Smith of OrangeFox86
// https://github.com/OrangeFox86/DreamcastControllerUsbPico
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "ScreenData.hpp"
#include <cstring>
#include <assert.h>
#include "hal/System/LockGuard.hpp"
#include "utils.h"

const uint32_t ScreenData::DEFAULT_SCREENS[ScreenData::NUM_DEFAULT_SCREENS][ScreenData::NUM_SCREEN_WORDS] = {
    {
        0x1FF8FFFF, 0xFFF80000, 0x00000000, 0x00000AEF, 0xDBF80000, 0xB6269208, 0x0000EDAE, 0xC2E807E0,
        0x81148AE8, 0x0FF05FEB, 0x1AE81C38, 0x88F6FA08, 0x1BD81AFE, 0xDBF81BD8, 0x18922800, 0x18187F9F,
        0x96D81FF8, 0x324A1078, 0x0FF0984C, 0x97980000, 0x6865AD38, 0x000011CE, 0x7FB00000, 0xAC6EA910,
        0x0C30ABDF, 0xCB200C30, 0xFE6D69C8, 0x0C30869C, 0x1EB00C30, 0x97D0F1E0, 0x0FF02C1F, 0xE2880FF0,
        0x6D64E458, 0x0C30E9DE, 0xDFC80C30, 0x0081C000, 0x0C30FEAA, 0xABF80C30, 0x82038208, 0x0FF0BA7B,
        0xEAE807E0, 0xBA9D82E8, 0x0000BA65, 0xCAE80000, 0x825B6A08, 0x0000FE25, 0x4BF80000, 0x00000000
    },
    {
        0x1FF8FFFF, 0xFFF80000, 0x00000000, 0x00000AEF, 0xDBF80000, 0xB6269208, 0x0000EDAE, 0xC2E807E0,
        0x81148AE8, 0x0FF05FEB, 0x1AE81C38, 0x88F6FA08, 0x1BD81AFE, 0xDBF81BD8, 0x18922800, 0x18187F9F,
        0x96D81FF8, 0x324A1078, 0x0FF0984C, 0x97980000, 0x6865AD38, 0x000011CE, 0x7FB00000, 0xAC6EA910,
        0x07F0ABDF, 0xCB200FF0, 0xFE6D69C8, 0x0C30869C, 0x1EB00C30, 0x97D0F1E0, 0x0C302C1F, 0xE28807F0,
        0x6D64E458, 0x07F0E9DE, 0xDFC80C30, 0x0081C000, 0x0C30FEAA, 0xABF80C30, 0x82038208, 0x0FF0BA7B,
        0xEAE807F0, 0xBA9D82E8, 0x0000BA65, 0xCAE80000, 0x825B6A08, 0x0000FE25, 0x4BF80000, 0x00000000
    },
    {
        0x1FF8FFFF, 0xFFF80000, 0x00000000, 0x00000AEF, 0xDBF80000, 0xB6269208, 0x0000EDAE, 0xC2E807E0,
        0x81148AE8, 0x0FF05FEB, 0x1AE81C38, 0x88F6FA08, 0x1BD81AFE, 0xDBF81BD8, 0x18922800, 0x18187F9F,
        0x96D81FF8, 0x324A1078, 0x0FF0984C, 0x97980000, 0x6865AD38, 0x000011CE, 0x7FB00000, 0xAC6EA910,
        0x07E0ABDF, 0xCB200FF0, 0xFE6D69C8, 0x0C30869C, 0x1EB00030, 0x97D0F1E0, 0x00302C1F, 0xE2880030,
        0x6D64E458, 0x0030E9DE, 0xDFC80030, 0x0081C000, 0x0030FEAA, 0xABF80C30, 0x82038208, 0x0FF0BA7B,
        0xEAE807E0, 0xBA9D82E8, 0x0000BA65, 0xCAE80000, 0x825B6A08, 0x0000FE25, 0x4BF80000, 0x00000000
    },
    {
        0x1FF8FFFF, 0xFFF80000, 0x00000000, 0x00000AEF, 0xDBF80000, 0xB6269208, 0x0000EDAE, 0xC2E807E0,
        0x81148AE8, 0x0FF05FEB, 0x1AE81C38, 0x88F6FA08, 0x1BD81AFE, 0xDBF81BD8, 0x18922800, 0x18187F9F,
        0x96D81FF8, 0x324A1078, 0x0FF0984C, 0x97980000, 0x6865AD38, 0x000011CE, 0x7FB00000, 0xAC6EA910,
        0x03F0ABDF, 0xCB2007F0, 0xFE6D69C8, 0x0E30869C, 0x1EB00C30, 0x97D0F1E0, 0x0C302C1F, 0xE2880C30,
        0x6D64E458, 0x0C30E9DE, 0xDFC80C30, 0x0081C000, 0x0C30FEAA, 0xABF80E30, 0x82038208, 0x07F0BA7B,
        0xEAE803F0, 0xBA9D82E8, 0x0000BA65, 0xCAE80000, 0x825B6A08, 0x0000FE25, 0x4BF80000, 0x00000000
    }
};

ScreenData::ScreenData(MutexInterface& mutex, uint32_t defaultScreenNum) :
    mMutex(mutex),
    mNewDataAvailable(false)
{
    if (defaultScreenNum > NUM_DEFAULT_SCREENS)
    {
        defaultScreenNum = 0;
    }
    std::memcpy(mDefaultScreen, DEFAULT_SCREENS[defaultScreenNum], sizeof(mDefaultScreen));
    resetToDefault();
}

void ScreenData::setData(const uint32_t* data, uint32_t startIndex, uint32_t numWords)
{
    assert(startIndex + numWords <= sizeof(mScreenData));
    LockGuard lockGuard(mMutex);
    if (lockGuard.isLocked())
    {
        std::memcpy(mScreenData + startIndex, data, numWords * sizeof(uint32_t));
        mNewDataAvailable = true;
    }
    else
    {
        DEBUG_PRINT("FAULT: failed to set screen data\n");
    }
}

void ScreenData::setDataToADefault(uint32_t defaultScreenNum)
{
    if (defaultScreenNum > NUM_DEFAULT_SCREENS)
    {
        defaultScreenNum = 0;
    }
    setData(DEFAULT_SCREENS[defaultScreenNum]);
}

void ScreenData::resetToDefault()
{
    std::memcpy(mScreenData, mDefaultScreen, sizeof(mScreenData));
    // Always force an update
    mNewDataAvailable = true;
}

bool ScreenData::isNewDataAvailable() const
{
    return mNewDataAvailable;
}

void ScreenData::readData(uint32_t* out)
{
    LockGuard lockGuard(mMutex);
    if (lockGuard.isLocked())
    {
        mNewDataAvailable = false;
    }
    else
    {
        DEBUG_PRINT("FAULT: failed to properly read screen data\n");
    }
    // Allow this to happen, even if locking failed
    std::memcpy(out, mScreenData, sizeof(mScreenData));
}
