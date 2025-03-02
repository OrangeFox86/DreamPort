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

#include "NonVolatilePicoSystemMemory.hpp"

#include "hal/System/LockGuard.hpp"

#include "hardware/flash.h"
#include "pico/stdlib.h"

#include <assert.h>
#include <string.h>
#include <algorithm>


NonVolatilePicoSystemMemory::NonVolatilePicoSystemMemory(uint32_t flashOffset, uint32_t size) :
    SystemMemory(),
    mOffset(flashOffset),
    mSize(size),
    mLocalMem(size),
    mMutex(),
    mProgrammingState(ProgrammingState::WAITING_FOR_JOB),
    mSectorQueue(),
    mDelayedWriteTime(0),
    mLastActivityTime(0)
{
    assert(flashOffset % SECTOR_SIZE == 0);

    // Copy all of flash into volatile memory
    const uint8_t* const readFlash = (const uint8_t *)(XIP_BASE + mOffset);
    mLocalMem.write(0, readFlash, size);
}

uint32_t NonVolatilePicoSystemMemory::getMemorySize()
{
    return mSize;
}

const uint8_t* NonVolatilePicoSystemMemory::read(uint32_t offset, uint32_t& size)
{
    mLastActivityTime = time_us_64();
    // A copy of memory is kept in RAM because nothing can be read from flash while erase is
    // processing which takes way too long for this to return within 500 microseconds
    return mLocalMem.read(offset, size);
}

bool NonVolatilePicoSystemMemory::write(uint32_t offset, const void* data, uint32_t& size)
{
    // This entire function is serialized with process()
    LockGuard lock(mMutex, true);

    mLastActivityTime = time_us_64();

    // First, store this data into local RAM
    bool success = mLocalMem.write(offset, data, size);

    if (size > 0)
    {
        uint16_t firstSector = offset / SECTOR_SIZE;
        uint16_t lastSector = (offset + size - 1) / SECTOR_SIZE;
        bool delayWrite = false;
        bool itemAdded = false;
        for (uint32_t i = firstSector; i <= lastSector; ++i)
        {
            std::list<uint16_t>::iterator it =
                std::find(mSectorQueue.begin(), mSectorQueue.end(), i);

            if (it == mSectorQueue.end())
            {
                // Add this sector
                mSectorQueue.push_back(i);
                itemAdded = true;
            }
            else if (it == mSectorQueue.begin())
            {
                // Currently processing this sector - delay write even further
                delayWrite = true;
            }
        }

        if (itemAdded)
        {
            // No longer need to delay write because caller moved on to another sector
            mDelayedWriteTime = 0;
        }
        else if (delayWrite)
        {
            setWriteDelay();
        }
    }

    return success;
}

uint64_t NonVolatilePicoSystemMemory::getLastActivityTime()
{
    // WARNING: Not an atomic read, but this isn't a critical thing anyway
    return mLastActivityTime;
}

void NonVolatilePicoSystemMemory::process()
{
    mMutex.lock();
    bool unlock = true;

    switch (mProgrammingState)
    {
        case ProgrammingState::WAITING_FOR_JOB:
        {
            if (!mSectorQueue.empty())
            {
                uint16_t sector = *mSectorQueue.begin();
                uint32_t flashByte = sectorToFlashByte(sector);

                mLastActivityTime = time_us_64();
                setWriteDelay();
                mProgrammingState = ProgrammingState::SECTOR_ERASING;

                // flash_range_erase blocks until erase is complete, so don't hold the lock
                // TODO: It should be possible to execute a non-blocking erase command then
                //       periodically check status within SECTOR_ERASING state until complete.
                //       It's not that important at the moment because this is the only process
                //       running in core 1.
                mMutex.unlock();
                unlock = false;
                flash_range_erase(flashByte, SECTOR_SIZE);
            }
        }
        break;

        case ProgrammingState::SECTOR_ERASING:
        {
            // Nothing to do in this state
            mProgrammingState = ProgrammingState::DELAYING_WRITE;
        }
        // Fall through

        case ProgrammingState::DELAYING_WRITE:
        {
            mLastActivityTime = time_us_64();
            // Write is delayed until the host moves on to writing another sector or if timeout
            // is reached. This helps ensure that the same sector isn't written multiple times.
            if (time_us_64() >= mDelayedWriteTime)
            {
                uint16_t sector = *mSectorQueue.begin();
                uint32_t localByte = sector * SECTOR_SIZE;
                uint32_t size = SECTOR_SIZE;
                const uint8_t* mem = mLocalMem.read(localByte, size);

                assert(mem != nullptr);

                uint32_t flashByte = sectorToFlashByte(sector);
                flash_range_program(flashByte, mem, SECTOR_SIZE);

                mSectorQueue.pop_front();
                mProgrammingState = ProgrammingState::WAITING_FOR_JOB;
            }
        }
        break;

        default:
        {
            assert(false);
        }
        break;
    }

    if (unlock)
    {
        mMutex.unlock();
    }
}

uint32_t NonVolatilePicoSystemMemory::sectorToFlashByte(uint16_t sector)
{
    return mOffset + (sector * SECTOR_SIZE);
}

void NonVolatilePicoSystemMemory::setWriteDelay()
{
    mDelayedWriteTime = time_us_64() + WRITE_DELAY_US;
}
