#include "Fonts.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <map>

#include <SDL3/SDL_rect.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "ColorUtils.hpp"

namespace Fonts
{
	namespace
	{
		const std::filesystem::path FONTS_LOCATION{ "C:/Windows/Fonts" };
		FontPath default_font_path{ "" };

		std::map<std::filesystem::path, std::weak_ptr<Font>> font_map;
	}

	Font::Font(const std::filesystem::path& font_path)
	{
		// binary mode is only for switching off newline translation
		std::ifstream file{ font_path, std::ios::binary };
		if (!file.is_open())
		{
			std::cout << "Failed to open font file" << '\n';
			return;
		}

		path = FontPath{ font_path };
		data.clear();

		file.unsetf(std::ios::skipws);

		file.seekg(0, std::ios::end);
		std::streampos file_size = file.tellg();
		file.seekg(0, std::ios::beg);

		data.reserve(file_size);
		data.insert(data.begin(),
			std::istream_iterator<uint8_t>{file},
			std::istream_iterator<uint8_t>{});

		file.close();

		stbtt_InitFont(&info, data.data(), stbtt_GetFontOffsetForIndex(data.data(), 0));
	}

	// Based on stb true type examples: https://github.com/justinmeiners/stb-truetype-example/blob/master/main.c
	std::vector<uint8_t> Font::CreateTextBitmap(const std::string& text, const float line_height, int& out_width, int& out_height) const
	{
		struct CharacterBitmap
		{
			std::vector<uint8_t> data;
			SDL_Rect bounds;
		};

		/* calculate font scaling */
		const float scale = stbtt_ScaleForPixelHeight(&info, line_height);

		int ascent;
		int descent;
		int line_gap;
		stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
		// Rounding with casts isn't really correct, but it only seems to work? so whatever
		ascent = static_cast<int>(floorf(static_cast<float>(ascent) * scale));
		descent = static_cast<int>(floorf(static_cast<float>(descent) * scale));
		line_gap = static_cast<int>(floorf(static_cast<float>(line_gap) * scale));
		const int scaled_line_height = ascent - descent + line_gap;

		int min_x = std::numeric_limits<int>::max();
		int min_y = std::numeric_limits<int>::max();

		int max_x = std::numeric_limits<int>::min();
		int max_y = std::numeric_limits<int>::min();

		int x = 0;
		int y = 0;

		std::vector<CharacterBitmap> character_infos;
		for (size_t i = 0; i < text.size(); ++i)
		{
			const char character = text[i];
			if (character == '\t') continue;

			if (character == '\n')
			{
				x = 0;
				y += scaled_line_height;
				continue;
			}

			// how wide is this character
			int advance_width;
			int left_side_bearing;
			stbtt_GetCodepointHMetrics(&info, character, &advance_width, &left_side_bearing);
			left_side_bearing = static_cast<int>(floorf(static_cast<float>(left_side_bearing) * scale));

			// (Note that each Codepoint call has an alternative Glyph version which caches the work required to look up the character word[i].)

			int x1, y1, x2, y2;
			stbtt_GetCodepointBitmapBox(&info, character, scale, scale, &x1, &y1, &x2, &y2);

			SDL_Rect bounds{ x + left_side_bearing, y + y1 + ascent, x2 - x1, y2 - y1 };

			std::vector<uint8_t> bitmap_data(static_cast<size_t>(bounds.w * bounds.h), 0);
			stbtt_MakeCodepointBitmap(&info, bitmap_data.data(), bounds.w, bounds.h, bounds.w, scale, scale, character);

			character_infos.emplace_back(std::move(bitmap_data), bounds);

			min_x = std::min<int>(min_x, bounds.x);
			min_y = std::min<int>(min_y, bounds.y);

			max_x = std::max<int>(max_x, bounds.x + bounds.w);
			max_y = std::max<int>(max_y, y + scaled_line_height);

			x += static_cast<int>(floorf(static_cast<float>(advance_width) * scale));

			// add kerning
			if (i < text.size() - 1 && text[i + 1] != '\n')
			{
				const int kern = stbtt_GetCodepointKernAdvance(&info, character, text[i + 1]);
				x += static_cast<int>(floorf(static_cast<float>(kern) * scale));
			}
		}

		const int width = max_x - min_x;
		const int height = max_y - min_y;

		std::vector<uint8_t> bitmap_data(static_cast<size_t>(width * height), 0);
		for (auto& [character_data, character_bounds] : character_infos)
		{
			for (int64_t y_pos = 0; y_pos < character_bounds.h; y_pos++) for (int64_t x_pos = 0; x_pos < character_bounds.w; x_pos++)
			{
				const int64_t bitmap_index = character_bounds.x - min_x + x_pos + (character_bounds.y - min_y + y_pos) * width;
				const int64_t character_index = x_pos + y_pos * character_bounds.w;

				uint8_t& bitmap_value = bitmap_data.at(bitmap_index);
				bitmap_value = AddColorComponent(bitmap_value, character_data.at(character_index));
			}
		}

		out_width = width;
		out_height = height;

		return bitmap_data;
	}

	void SetupDefaultFont()
	{
		for (const auto& entry : std::filesystem::directory_iterator{ FONTS_LOCATION })
		{
			const std::filesystem::path& path = entry.path();
			if (path.extension().generic_string() != ".ttf") continue;

			default_font_path = FontPath{ path };
			return;
		}
	}

	std::vector<FontPath> AvailableFonts()
	{
		std::vector<FontPath> fonts;
		for (const auto& entry : std::filesystem::directory_iterator{ FONTS_LOCATION })
		{
			const std::filesystem::path& path = entry.path();
			if (path.extension().generic_string() != ".ttf") continue;

			fonts.emplace_back(path);
		}

		return fonts;
	}

	std::shared_ptr<Font> GetFont(const FontPath& available_font)
	{
		const std::filesystem::path& font_path = available_font.GetPath();
		if (font_path.empty() || !exists(font_path)) return GetFont(default_font_path);

		auto& weak_font_pointer = font_map[font_path];
		if (!weak_font_pointer.expired()) return weak_font_pointer.lock();

		auto new_font = std::make_shared<Font>(font_path);
		weak_font_pointer = new_font;
		return new_font;
	}
}
