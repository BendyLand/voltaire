/**************************************************************************/
/*  test_time.cpp                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_time)

#include "core/os/time.h"

namespace TestTime {

#define YEAR_KEY "year"
#define MONTH_KEY "month"
#define DAY_KEY "day"
#define WEEKDAY_KEY "weekday"
#define HOUR_KEY "hour"
#define MINUTE_KEY "minute"
#define SECOND_KEY "second"
#define DST_KEY "dst"

TEST_CASE("[Time] Unix time conversion to/from datetime string") {
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1970-01-01T00:00:00") == 0, "Time get_unix_time_from_datetime_string: The timestamp for Unix epoch is zero.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1970-01-01 00:00:00") == 0, "Time get_unix_time_from_datetime_string: The timestamp for Unix epoch with space is zero.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1970-01-01") == 0, "Time get_unix_time_from_datetime_string: The timestamp for Unix epoch without time is zero.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("00:00:00") == 0, "Time get_unix_time_from_datetime_string: The timestamp for zero time without date is zero.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1969-12-31T23:59:59") == -1, "Time get_unix_time_from_datetime_string: The timestamp for just before Unix epoch is negative one.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1234-05-06T07:08:09") == -23215049511, "Time get_unix_time_from_datetime_string: The timestamp for an arbitrary datetime is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1234-05-06 07:08:09") == -23215049511, "Time get_unix_time_from_datetime_string: The timestamp for an arbitrary datetime with space is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1234-05-06") == -23215075200, "Time get_unix_time_from_datetime_string: The timestamp for an arbitrary date without time is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("07:08:09") == 25689, "Time get_unix_time_from_datetime_string: The timestamp for an arbitrary time without date is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("2014-02-09T22:10:30") == 1391983830, "Time get_unix_time_from_datetime_string: The timestamp for GODOT IS OPEN SOURCE is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("2014-02-09 22:10:30") == 1391983830, "Time get_unix_time_from_datetime_string: The timestamp for GODOT IS OPEN SOURCE with space is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("2014-02-09") == 1391904000, "Time get_unix_time_from_datetime_string: The date for GODOT IS OPEN SOURCE without time is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("22:10:30") == 79830, "Time get_unix_time_from_datetime_string: The time for GODOT IS OPEN SOURCE without date is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("-1000000000-01-01T00:00:00") == -31557014167219200, "Time get_unix_time_from_datetime_string: In the year negative a billion, Japan might not have been here.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1000000-01-01T00:00:00") == 31494784780800, "Time get_unix_time_from_datetime_string: The timestamp for the year a million is as expected.");

	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(0) == "1970-01-01T00:00:00", "Time get_datetime_string_from_unix_time: The timestamp string for Unix epoch is zero.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(0, true) == "1970-01-01 00:00:00", "Time get_datetime_string_from_unix_time: The timestamp string for Unix epoch with space is zero.");
	CHECK_MESSAGE(Time::get_date_string_from_unix_time(0) == "1970-01-01", "Time get_date_string_from_unix_time: The date string for zero is Unix epoch date.");
	CHECK_MESSAGE(Time::get_time_string_from_unix_time(0) == "00:00:00", "Time get_time_string_from_unix_time: The date for zero zero is Unix epoch date.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(-1) == "1969-12-31T23:59:59", "Time get_time_string_from_unix_time: The timestamp string for just before Unix epoch is as expected.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(-23215049511) == "1234-05-06T07:08:09", "Time get_datetime_string_from_unix_time: The timestamp for an arbitrary datetime is as expected.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(-23215049511, true) == "1234-05-06 07:08:09", "Time get_datetime_string_from_unix_time: The timestamp for an arbitrary datetime with space is as expected.");
	CHECK_MESSAGE(Time::get_date_string_from_unix_time(-23215075200) == "1234-05-06", "Time get_date_string_from_unix_time: The timestamp for an arbitrary date without time is as expected.");
	CHECK_MESSAGE(Time::get_time_string_from_unix_time(25689) == "07:08:09", "Time get_time_string_from_unix_time: The timestamp for an arbitrary time without date is as expected.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(1391983830) == "2014-02-09T22:10:30", "Time get_datetime_string_from_unix_time: The timestamp for GODOT IS OPEN SOURCE is as expected.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(1391983830, true) == "2014-02-09 22:10:30", "Time get_datetime_string_from_unix_time: The timestamp for GODOT IS OPEN SOURCE with space is as expected.");
	CHECK_MESSAGE(Time::get_date_string_from_unix_time(1391904000) == "2014-02-09", "Time get_date_string_from_unix_time: The date for GODOT IS OPEN SOURCE without time is as expected.");
	CHECK_MESSAGE(Time::get_time_string_from_unix_time(79830) == "22:10:30", "Time get_time_string_from_unix_time: The time for GODOT IS OPEN SOURCE without date is as expected.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(31494784780800) == "1000000-01-01T00:00:00", "Time get_datetime_string_from_unix_time: The timestamp for the year a million is as expected.");
	CHECK_MESSAGE(Time::get_offset_string_from_offset_minutes(0) == "+00:00", "Time get_offset_string_from_offset_minutes: The offset string is as expected.");
	CHECK_MESSAGE(Time::get_offset_string_from_offset_minutes(-600) == "-10:00", "Time get_offset_string_from_offset_minutes: The offset string is as expected.");
	CHECK_MESSAGE(Time::get_offset_string_from_offset_minutes(345) == "+05:45", "Time get_offset_string_from_offset_minutes: The offset string is as expected.");
}

TEST_CASE("[Time] System time methods") {
	const uint64_t ticks_msec = Time::get_ticks_msec();
	const uint64_t ticks_usec = Time::get_ticks_usec();

	CHECK_MESSAGE(Time::get_unix_time_from_system() > 1000000000, "Time get_unix_time_from_system: The timestamp from system time doesn't fail and is very positive.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string(Time::get_datetime_string_from_system()) > 1000000000, "Time get_datetime_string_from_system: The timestamp from system time doesn't fail and is very positive.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string(Time::get_date_string_from_system()) > 1000000000, "Time get_datetime_string_from_system: The date from system time doesn't fail and is very positive.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string(Time::get_time_string_from_system()) < 86400, "Time get_datetime_string_from_system: The time from system time doesn't fail and is within the acceptable range.");

	CHECK_MESSAGE(Time::get_ticks_msec() >= ticks_msec, "Time get_ticks_msec: The value has not decreased.");
	CHECK_MESSAGE(Time::get_ticks_usec() > ticks_usec, "Time get_ticks_usec: The value has increased.");
}

} // namespace TestTime
