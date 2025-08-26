#pragma once

#include <cstdint>
#include <string>

#include "Fonts.hpp"
#include "DateTime.hpp"

namespace std
{
	namespace filesystem
	{
		class path;
	}

	template<typename>
	class future;
}

struct SDL_Texture;
struct SDL_Renderer;

namespace Image
{
	class Image
	{
	public:
		Image() = default;
		Image(void* data, int width, int height);
		explicit Image(const std::filesystem::path& file_path, float scaling = 1.0f);

		Image(Image&) = delete;
		Image(Image&& image) noexcept;
		Image& operator=(Image&) = delete;
		Image& operator=(Image&&) = delete;

		virtual ~Image();

		void Render(SDL_Renderer* renderer) const;
		[[nodiscard]] SDL_FRect GetScreenRect() const;
		[[nodiscard]] void* GetTexture() const { return texture; }
		[[nodiscard]] int GetWidth() const { return width; }
		[[nodiscard]] int GetHeight() const { return height; }

		void SetColor(const uint32_t new_color);
		[[nodiscard]] uint32_t GetColor() const { return color; }

		virtual void UI();

		int x = 0;
		int y = 0;

		SDL_Point size{ 0, 0 };

	protected:
		void CreateTexture(void* data);

		int width{ 0 };
		int height{ 0 };

		uint32_t color{ 0xFFFFFFFF };
		SDL_Texture* texture{ nullptr };
	};

	class Text : public Image
	{
	public:
		Text(std::string string = "Text", uint32_t text_color = 0xFFFFFFFF);

		Text(Text&) = delete;
		Text(Text&&) = delete;
		Text& operator=(Text&) = delete;
		Text& operator=(Text&&) = delete;

		virtual ~Text() override = default;

		void SetFont(const std::shared_ptr<Fonts::Font>& new_font)
		{
			font = new_font;
			CreateTextTexture();
		}

		void SetTextColor(const uint32_t new_color)
		{
			text_color = new_color;
			CreateTextTexture();
		}
		[[nodiscard]] uint32_t GetTextColor() const { return text_color; }

		void SetBgColor(const uint32_t new_color)
		{
			bg_color = new_color;
			CreateTextTexture();
		}
		[[nodiscard]] uint32_t GetBgColor() const { return bg_color; }

		void SetScale(const float scale)
		{
			line_height = scale;
			CreateTextTexture();
		}
		[[nodiscard]] float GetScale() const { return line_height; }

		void SetText(const std::string& string)
		{
			text = string;
			CreateTextTexture();
		}
		[[nodiscard]] std::string GetText() const { return text; }

		const Fonts::FontPath& GetFontPath() const { return font->GetPath(); }

		virtual void UI() override;

	protected:
		virtual void CreateTextTexture();
		void UIFontSelect();
		void UISettings();

		std::shared_ptr<Fonts::Font> font{ Fonts::GetFont() };
		uint32_t text_color{ 0xFFFFFFFF };
		uint32_t bg_color{ 0x00000000 };

		float line_height{ 60.0f };
		std::string text{ "Text" };
	};

	class TimeText : public Text
	{
	public:
		TimeText();

		virtual void UI() override;

	private:
		virtual void CreateTextTexture() override;
		bool UITimezoneSelector();

		std::vector<std::string_view> timezones;
		DateTime::DateTime date_time;
		bool lower_am_pm = true;
	};

	inline std::vector<std::unique_ptr<Image>> images;

	inline size_t selection_index = std::numeric_limits<size_t>::max();
	// Convenience functions for dealing with selection:
	inline void ResetSelection() { selection_index = std::numeric_limits<size_t>::max(); };
	[[nodiscard]] inline bool IsSelectionValid() { return selection_index < images.size(); }
	[[nodiscard]] inline std::unique_ptr<Image>& GetSelection() { return images.at(selection_index); };

	struct Canvas
	{
		Canvas(Image&& image);
		Canvas(const std::filesystem::path& path, float scaling);
		~Canvas();

		[[nodiscard]] bool IsValid() const { return target != nullptr; }

		SDL_Texture* target{ nullptr };
		Image image;

		void UpdateScaleAndOffset(const SDL_FPoint& working_area);
		[[nodiscard]] std::future<void> ExportCanvas(std::string&& path) const;

		float base_scale{ 1.0f };
		SDL_FPoint render_offset{};
		SDL_FPoint content_size{};

	private:
		void CreateRenderTarget();
	};

	inline std::unique_ptr<Canvas> canvas;
}