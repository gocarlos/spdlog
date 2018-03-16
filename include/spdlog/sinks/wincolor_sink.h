//
// Copyright(c) 2016 spdlog
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#include "../common.h"
#include "../details/null_mutex.h"
#include "base_sink.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <wincon.h>

namespace spdlog { namespace sinks {
/*
 * Windows color console sink. Uses WriteConsoleA to write to the console with colors
 */
template<class Mutex> class wincolor_sink : public base_sink<Mutex>
{
public:
    const WORD BOLD = FOREGROUND_INTENSITY;
    const WORD RED = FOREGROUND_RED;
    const WORD CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE;
    const WORD WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    const WORD YELLOW = FOREGROUND_RED | FOREGROUND_GREEN;

    wincolor_sink(HANDLE std_handle)
        : out_handle_(std_handle)
    {
        colors_[level::trace] = CYAN;
        colors_[level::debug] = CYAN;
        colors_[level::info] = WHITE | BOLD;
        colors_[level::warn] = YELLOW | BOLD;
        colors_[level::err] = RED | BOLD;                         // red bold
        colors_[level::critical] = BACKGROUND_RED | WHITE | BOLD; // white bold on red background
        colors_[level::off] = 0;
    }

    ~wincolor_sink() override
    {
        this->flush();
    }

    wincolor_sink(const wincolor_sink &other) = delete;
    wincolor_sink &operator=(const wincolor_sink &other) = delete;

    // change the color for the given level
    void set_color(level::level_enum level, WORD color)
    {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::_mutex);
        colors_[level] = color;
    }

protected:
    void _sink_it(const details::log_msg &msg) override
    {
        auto color = colors_[msg.level];
        auto orig_attribs = set_console_attribs(color);
        WriteConsoleA(out_handle_, msg.formatted.data(), static_cast<DWORD>(msg.formatted.size()), nullptr, nullptr);
        SetConsoleTextAttribute(out_handle_, orig_attribs); // reset to orig colors
    }

    void _flush() override
    {
        // windows console always flushed?
    }

private:
    HANDLE out_handle_;
    std::unordered_map<level::level_enum, WORD, level::level_hasher> colors_;

    // set color and return the orig console attributes (for resetting later)
    WORD set_console_attribs(WORD attribs)
    {
        CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
        GetConsoleScreenBufferInfo(out_handle_, &orig_buffer_info);
        WORD back_color = orig_buffer_info.wAttributes;
        // retrieve the current background color
        back_color &= static_cast<WORD>(~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY));
        // keep the background color unchanged
        SetConsoleTextAttribute(out_handle_, attribs | back_color);
        return orig_buffer_info.wAttributes; // return orig attribs
    }
};

//
// windows color console to stdout
//
template<class Mutex> class wincolor_stdout_sink : public wincolor_sink<Mutex>
{
public:
    wincolor_stdout_sink()
        : wincolor_sink<Mutex>(GetStdHandle(STD_OUTPUT_HANDLE))
    {
    }
};

using wincolor_stdout_sink_mt = wincolor_stdout_sink<std::mutex>;
using wincolor_stdout_sink_st = wincolor_stdout_sink<details::null_mutex>;

//
// windows color console to stderr
//
template<class Mutex> class wincolor_stderr_sink : public wincolor_sink<Mutex>
{
public:
    wincolor_stderr_sink()
        : wincolor_sink<Mutex>(GetStdHandle(STD_ERROR_HANDLE))
    {
    }
};

using wincolor_stderr_sink_mt = wincolor_stderr_sink<std::mutex>;
using wincolor_stderr_sink_st = wincolor_stderr_sink<details::null_mutex>;

}} // namespace spdlog::sinks
