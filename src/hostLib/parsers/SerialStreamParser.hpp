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

#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include <atomic>

#include "hal/System/MutexInterface.hpp"
#include "hal/Usb/TtyParser.hpp"
#include "hal/Usb/CommandParser.hpp"

// Command structure: [whitespace]<command-char>[command]<\n>

//! Command parser for processing commands from a TTY stream
class SerialStreamParser : public TtyParser
{
public:
    //! Constructor
    SerialStreamParser(MutexInterface& m, char helpChar);
    //! Adds a command parser to my list of parsers - must be done before any other function called
    virtual void addCommandParser(std::shared_ptr<CommandParser> parser) override final;
    //! Called from the process receiving characters on the TTY
    void addChars(const char* chars, uint32_t len) override final;
    //! Called from the process handling maple bus execution
    virtual void process() final;

private:
    //! Max of 2 KB of memory to use for tty RX queue
    static const uint32_t MAX_QUEUE_SIZE = 2048;
    //! String of characters that are considered whitespace
    static const char* WHITESPACE_CHARS;
    //! String of characters that are considered end of line characters
    static const char* INPUT_EOL_CHARS;
    //! The singular character that is replaced in the RX list as the EOL char
    static const char RX_EOL_CHAR;
    //! String of characters that are treated as a backspace
    static const char* BACKSPACE_CHARS;
    //! Receive queue
    std::vector<char> mParserRx;
    //! Flag that is set to true if the last read character is an EOL (used to ignore further EOL)
    bool mLastIsEol;
    //! Mutex used to serialize addChars and process
    MutexInterface& mParserMutex;
    //! Flag when end of line detected on add
    std::atomic<bool> mCommandReady;
    //! The command character which prints help for all commands
    const char mHelpChar;
    //! Parsers that may handle data
    std::vector<std::shared_ptr<CommandParser>> mParsers;
    //! true when overflow in mParserRx
    bool mOverflowDetected;
};
