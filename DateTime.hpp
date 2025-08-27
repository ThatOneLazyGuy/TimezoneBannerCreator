#pragma once

#include <array>
#include <string>
#include <chrono>

namespace DateTime
{
	using namespace std::chrono;

	class DateTime
	{
	public:
		DateTime()
		{
			MakeCurrentDateTime();
		}
		explicit DateTime(const std::string_view& time_zone) : timezone{ time_zone }
		{
			MakeCurrentDateTime();
		}
		DateTime(const year_month_day& year_month_day, const hh_mm_ss<seconds>& hour_minute_second) :
			date{ year_month_day }, time{ hour_minute_second }
		{
		}

		[[nodiscard]] local_time<seconds> GetTimePoint() const { return local_days{ date } + time.to_duration(); }
		[[nodiscard]] year_month_day GetDate() const { return date; }
		[[nodiscard]] hh_mm_ss<seconds> GetTime() const { return time; }

		void MakeCurrentDateTime()
		{
			const time_point now = GetTimeZone().to_local(floor<seconds>(system_clock::now()));
			const local_time local_days = floor<days>(now);
			date = year_month_day{ local_days };
			time = hh_mm_ss{ floor<seconds>(now - local_days) };
		}

		local_seconds GetLocalizedTimePoint(const std::string_view& time_zone) const { return locate_zone(time_zone)->to_local(GetSystemTimePoint()); }

		[[nodiscard]] zoned_time<seconds> GetZonedTime() const { return zoned_time{ timezone, GetTimePoint() }; }
		[[nodiscard]] zoned_time<seconds> GetLocalizedZonedTime(const std::string_view& time_zone) const { return zoned_time{ time_zone, GetLocalizedTimePoint(time_zone) }; }

		const time_zone& GetTimeZone() const { return *locate_zone(timezone); }

	private:
		sys_seconds GetSystemTimePoint() const
		{
			return GetTimeZone().to_sys(GetTimePoint());
		}

		std::string_view timezone = current_zone()->name();
		year_month_day date{};
		hh_mm_ss<seconds> time{};
	};

	inline time_point<local_t, seconds> GetTimePoint() { return current_zone()->to_local(floor<seconds>(system_clock::now())); }
	inline year_month_day GetDate() { return year_month_day{ floor<days>(GetTimePoint()) }; }
	inline hh_mm_ss<seconds> GetTime()
	{
		const time_point date_time = GetTimePoint();
		return hh_mm_ss{ floor<seconds>(date_time - floor<days>(date_time)) };
	}
	std::string FormatDateTime(const std::string& string, const std::vector<std::string_view>& selected_timezones, const DateTime& date_time, bool lower_am_pm = false);

	struct DateTimeFormat
	{
		std::string_view custom_format;
		std::string_view replacement_format;
		std::string_view description;
	};

	constexpr std::array<DateTimeFormat, 25> date_time_formatters
	{ {
		{"DATETIME", "%c", "Full date and time"},
		{"DATE", "%x", "Full date"},

		{"DAYW", "%A", "Name of the day in the week"},
		{"DW", "%a", "Shorthand name for the day in the week"},

		{"DNW", "%u", "Number of the day in the week (monday = 1)"},
		{"DIW", "%w", "Index of the day in the week (monday = 0)"},

		{"DN", "%j", "Number of the day in the year (January 1 = 001, 3 digits)"},

		{"DD", "%d", "Day in the month (2 digits)"},
		{"D", "%e", "Day in the month (1 or 2 digits)"},

		{"WN", "%W", "Number of the week in the year (first monday of year: WN = 01, before that: WN = 00, 2 digits)"},

		{"MONTH", "%B", "Name of the month"},
		{"MON", "%b", "Shorthand name of the month"},
		{"MM", "%m", "Month in year (2 digits)"},

		{"CCCC", "%C", "Century (4 digits)"},

		{"YYYY", "%Y", "Year (4 digits)"},
		{"YY", "%y", "Year (2 digits)"},

		// ----------------------------

		{"TZOF", "%z", "Timezone offset (e.g. +0230)"},
		{"TZ:OF", "%Ez", "Timezone offset with minute separator (e.g. +02:30)"},
		{"TZ", "%Z", "Timezone abbreviation"},

		// -----------------------------

		{"TIME", "%X", "Full time of day"},

		{"HH", "%H", "Hour in day (24-hour clock, 2 digits)"},
		{"hh", "%I", "Hour in day (12-hour clock, 2 digits)"},
		{"mm", "%M", "Minute in hour (2 digits)"},
		{"ss", "%S", "Second in hour (2 digits)"},

		{"ap", "%p", "AM or PM of day"},
	} };

}
