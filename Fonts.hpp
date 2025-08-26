#pragma once

#include <filesystem>
#include <vector>

#include <stb_truetype.h> // Don't define implementation, should only be defined in Fonts.cpp
#include <SDL3/SDL_rect.h>

namespace Fonts
{
	class FontPath
	{
	public:
		FontPath() = default;
		explicit FontPath(std::filesystem::path path) : path{ std::move(path) }
		{}

		[[nodiscard]] std::string GetName() const { return path.stem().generic_string(); }
		[[nodiscard]] std::filesystem::path GetPath() const { return path; }

	private:
		std::filesystem::path path;
	};

	class Font
	{
	public:
		explicit Font(const std::filesystem::path& font_path);

		std::vector<uint8_t> CreateTextBitmap(const std::string& text, float line_height, int& out_width, int& out_height) const;
		[[nodiscard]] const FontPath& GetPath() const { return path; }

	private:
		FontPath path;
		std::vector<uint8_t> data;
		stbtt_fontinfo info{};
	};

	void SetupDefaultFont();
	std::vector<FontPath> AvailableFonts();
	std::shared_ptr<Font> GetFont(const FontPath& available_font = {}); // Empty or invalid name returns default font.
}
