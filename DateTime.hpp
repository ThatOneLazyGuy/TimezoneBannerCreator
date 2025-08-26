#pragma once

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

		[[nodiscard]] time_point<local_t, seconds> GetTimePoint() const { return local_days{ date } + time.to_duration(); }
		[[nodiscard]] year_month_day GetDate() const { return date; }
		[[nodiscard]] hh_mm_ss<seconds> GetTime() const { return time; }

		void MakeCurrentDateTime()
		{
			const time_point now = GetTimeZone().to_local(floor<seconds>(system_clock::now()));
			const local_time local_days = floor<days>(now);
			date = year_month_day{ local_days };
			time = hh_mm_ss{ floor<seconds>(now - local_days) };
		}

		local_time<seconds> GetLocalizedTimePoint(const std::string_view& time_zone) const { return locate_zone(time_zone)->to_local(GetSystemTimePoint()); }

	private:
		sys_time<seconds> GetSystemTimePoint() const
		{
			return GetTimeZone().to_sys(GetTimePoint());
		}
		const time_zone& GetTimeZone() const { return *locate_zone(timezone); }

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
	std::string ReplaceTime(std::string format);
	std::string UpdateText(const std::string& format, const std::vector<std::string_view>& selected_timezones, const DateTime& date_time, bool lower_am_pm = false);

}
