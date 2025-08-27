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

	std::string ReplaceDateTime(std::string format)
	{
		if (format.empty()) return {};

		for (size_t i = 0; i < format.size(); i++)
		{
			for (const auto& [custom_format, replacement_format, explanation] : date_time_formatters)
			{
				i += Replace(format, i, custom_format, replacement_format);
			}

		}

		return format;
	}

	std::string FormatDateTime(const std::string& string, const std::vector<std::string_view>& selected_timezones, const DateTime& date_time, const bool lower_am_pm)
	{
		if (string.empty()) return "";

		const std::string format = ReplaceDateTime(string);

		std::string text;
		for (size_t i = 0; i < selected_timezones.size(); i++)
		{
			const std::string_view timezone_name = selected_timezones.at(i);
			const std::string timezone_city = std::filesystem::path{ timezone_name }.filename().generic_string();
			const zoned_time time_in_zone = date_time.GetLocalizedZonedTime(timezone_name);

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
