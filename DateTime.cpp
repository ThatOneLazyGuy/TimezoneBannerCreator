#include "DateTime.hpp"

#include <array>
#include <filesystem>

namespace DateTime
{
	namespace
	{
		// Source: https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
		size_t ReplaceInString(std::string& string, const std::string& replace, const std::string& with)
		{
			if (string.empty() || replace.empty() || with.empty()) return 0;

			size_t replaced_count = 0;

			size_t start_pos = 0;
			while ((start_pos = string.find(replace, start_pos)) != std::string::npos)
			{
				string.replace(start_pos, replace.size(), with);
				start_pos += with.size(); // In case 'to' contains 'from', like replacing 'x' with 'yx'

				replaced_count++;
			}

			return replaced_count;
		}

		size_t Replace(std::string& string, const size_t i, const std::string_view& replace, const std::string_view& with)
		{
			if (string.compare(i, replace.size(), replace) != 0) return 0;

			string.replace(i, replace.size(), with);
			return with.size();
		}
	}

	std::string ReplaceTime(std::string format)
	{
		if (format.empty()) return {};

		constexpr std::array<std::pair<std::string_view, std::string_view>, 25> formatting_replacements
		{ {
			{"DATETIME", "%c"}, // Full date and time
			{"DATE", "%x"}, // Full date

			{"DAYW", "%A"}, // Name of the day in the week
			{"DW", "%a"}, // Shorthand name for the day in the week

			{"DNW", "%u"}, // Number of the day in the week (monday = 1)
			{"DIW", "%iw"}, // Index of the day in the week (monday = 0)

			{"DN", "%j"}, // Number of the day in the year (January 1 = 001, 3 digits)

			{"DD", "%d"}, // Day in the month (2 digits)
			{"D", "%e"}, // Day in the month (1 or 2 digits)

			{"WN", "%W"}, // Number of the week in the year (first monday of year: WN = 01, before that: WN = 00, 2 digits)

			{"MONTH", "%B"}, // Name of the month
			{"MON", "%b"}, // Shorthand name of the month
			{"MM", "%m"}, // Month in year (2 digits)

			{"CCCC", "%C"}, // Century (4 digits)

			{"YYYY", "%Y"}, // Year (4 digits)
			{"YY", "%y"}, // Year (2 digits)

			// ----------------------------

			{"TZOF", "%z"}, // Timezone offset (e.g. +0230)
			{"TZ:OF", "%Ez"}, // Timezone offset with minute separator (e.g. +02:30)
			{"TZ", "%Z"}, // Timezone abbreviation

			// -----------------------------

			{"HH", "%H"}, // Hour in day (24-hour clock, 2 digits)
			{"hh", "%I"}, // Hour in day (12-hour clock, 2 digits)
			{"mm", "%M"}, // Minute in hour (2 digits)
			{"ss", "%S"}, // Second in hour (2 digits)

			{"ap", "%p"}, // AM or PM of day

			{"time", "%X"} // Full time of day
		} };

		for (size_t i = 0; i < format.size(); i++)
		{
			for (const auto& [replace, with] : formatting_replacements)
			{
				i += Replace(format, i, replace, with);
			}

		}

		return format;
	}

	std::string UpdateText(const std::string& format, const std::vector<std::string_view>& selected_timezones, const DateTime& date_time, const bool lower_am_pm)
	{
		using namespace std::chrono;

		if (format.empty()) return "";

		std::string text;
		for (size_t i = 0; i < selected_timezones.size(); i++)
		{
			const std::string_view timezone_name = selected_timezones.at(i);
			const std::string timezone_city = std::filesystem::path{ timezone_name }.filename().generic_string();
			const local_time time_in_zone = date_time.GetLocalizedTimePoint(timezone_name);

			try
			{
				text += std::vformat("{:" + format + '}', std::make_format_args(time_in_zone));
			}
			catch (const std::format_error&)
			{
				// Just add the formating if it is invalid, this makes it clear enough that the formatting has broken
				text += format;
			}

			if (i != selected_timezones.size() - 1) text += '\n';

			ReplaceInString(text, "TMZCITY", timezone_city);
		}

		if (lower_am_pm)
		{
			ReplaceInString(text, "PM", "pm");
			ReplaceInString(text, "AM", "am");
		}

		return text;
	}
}
