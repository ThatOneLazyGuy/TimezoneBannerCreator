#include "Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <imsearch/imsearch.h>
#include <SDL3/SDL_render.h>

#include <future>
#include <iostream>

#include "Renderer.hpp"
#include "ColorUtils.hpp"

using namespace std::chrono;

namespace Image
{
	namespace
	{
		template <typename Type>
		bool ClampValue(Type& value, const Type min, const Type max)
		{
			if (value < min)
			{
				value = min;
				return true;
			}

			if (value > max)
			{
				value = max;
				return true;
			}

			return false;
		}

		template <typename IntType, typename DateType>
		bool DragDate(const std::string& label, DateType& value, const int min = 0, const int max = 0, const float speed = 0.25f)
		{
			int date = static_cast<int>(static_cast<IntType>(value));

			bool changed = false;
			if (min != 0 && max != 0) changed |= ClampValue(date, min, max);

			changed |= ImGui::DragInt(label.c_str(), &date, speed, min, max, "%d", ImGuiSliderFlags_ClampOnInput);
			if (!changed) return false;

			value = DateType{ static_cast<IntType>(date) };
			return true;
		}

		template <typename TimeType>
		bool DragTime(const std::string& label, TimeType& value, const int min, const int max, const float speed = 0.25f)
		{
			int time = static_cast<int>(std::chrono::duration_cast<TimeType>(value).count());

			bool changed = false;
			if (min != 0 && max != 0) changed |= ClampValue(time, min, max);

			changed |= ImGui::DragInt(label.c_str(), &time, speed, min, max, "%d", ImGuiSliderFlags_ClampOnInput);
			if (!changed) return false;

			value = TimeType{ static_cast<unsigned int>(time) };
			return true;
		}

		bool DragDate(const std::string& label, year_month_day& value, const float speed = 0.25f)
		{
			ImGui::Text("%s", label.c_str());
			const float drag_width = ImGui::GetContentRegionAvail().x / 3.0f;

			bool changed = false;
			year year = value.year();
			ImGui::SetNextItemWidth(drag_width);
			changed |= DragDate<int>("##Year", year, 0, 0, speed);
			ImGui::SameLine();

			month month = value.month();
			ImGui::SetNextItemWidth(drag_width);
			changed |= DragDate<unsigned int>("##Month", month, 1, 12, speed);
			ImGui::SameLine();

			const unsigned int max_day_count = static_cast<unsigned int>(year_month_day_last{ year / month / last }.day());
			day day = value.day();
			ImGui::SetNextItemWidth(drag_width);
			changed |= DragDate<unsigned int>("##Day", day, 1, static_cast<int>(max_day_count), speed);

			if (!changed) return false;

			value = year / month / day;
			return true;
		}

		bool DragTime(const std::string& label, hh_mm_ss<seconds>& value, const float speed = 0.25f)
		{
			ImGui::Text("%s", label.c_str());
			const float drag_width = ImGui::GetContentRegionAvail().x / 3.0f;

			bool changed = false;
			hours hour = value.hours();
			ImGui::SetNextItemWidth(drag_width);
			changed |= DragTime("##Hour", hour, 0, 23, speed);
			ImGui::SameLine();

			minutes minute = value.minutes();
			ImGui::SetNextItemWidth(drag_width);
			changed |= DragTime("##Minute", minute, 0, 59, speed);
			ImGui::SameLine();

			seconds second = value.seconds();
			ImGui::SetNextItemWidth(drag_width);
			changed |= DragTime("##Second", second, 0, 59, speed);

			if (!changed) return false;

			value = hh_mm_ss{ hour + minute + second };
			return true;
		}

		// Used asynchronously to show the user a modal dialog while exporting, path and data are non const references because they are moved to this function
		void AsyncExport(std::string path, const int width, const int height, std::vector<uint32_t> data)
		{
			stbi_write_png(path.c_str(), width, height, 4, data.data(), 4 * width);
		}
	}

	Image::Image(void* data, const int width, const int height) : width{ width }, height{ height }
	{
		if (data == nullptr) return;

		CreateTexture(data);
	}

	Image::Image(const std::filesystem::path& file_path, const float scaling)
	{
		const std::string path = file_path.generic_string();

		uint8_t* image_data = stbi_load(path.c_str(), &width, &height, nullptr, 4);
		if (image_data == nullptr)
		{
			std::cout << "Failed to load image" << '\n';
			return;
		}

		if (scaling == 1.0f)
		{
			CreateTexture(image_data);
			stbi_image_free(image_data);

			return;
		}

		// Using std::max because you just know someone is going to try to make an image that is 0 x 0
		const int scaled_width = std::max<int>(static_cast<int>(scaling * static_cast<float>(width)), 1);
		const int scaled_height = std::max<int>(static_cast<int>(scaling * static_cast<float>(height)), 1);
		std::vector<uint8_t> data(static_cast<size_t>(scaled_width) * static_cast<size_t>(scaled_height) * 4);

		stbir_resize_uint8_linear(image_data, width, height, width * 4, data.data(), scaled_width, scaled_height, scaled_width * 4, STBIR_RGBA);
		stbi_image_free(image_data);

		width = scaled_width;
		height = scaled_height;
		CreateTexture(data.data());
	}

	Image::Image(Image&& image) noexcept :
		x{ image.x },
		y{ image.y },
		size{ image.size },
		width{ image.width },
		height{ image.height },
		color{ image.color },
		texture{ image.texture }
	{
		image.texture = nullptr;
	}

	Image::~Image()
	{
		if (texture != nullptr) SDL_DestroyTexture(texture);
	}

	void Image::Render(SDL_Renderer* renderer) const
	{
		if (texture == nullptr) return;

		const SDL_Rect start_rect{ x, y, size.x, size.y };
		SDL_FRect float_rect;
		SDL_RectToFRect(&start_rect, &float_rect);

		SDL_RenderTexture(renderer, texture, nullptr, &float_rect);
	}

	SDL_FRect Image::GetScreenRect() const
	{
		const SDL_Rect start_rect{ x, y, size.x, size.y };
		SDL_FRect float_rect;
		SDL_RectToFRect(&start_rect, &float_rect);

		const float scaling = Renderer::zoom * canvas->base_scale;
		(float_rect.x *= scaling) += canvas->render_offset.x - Renderer::scroll.x;
		(float_rect.y *= scaling) += canvas->render_offset.y - Renderer::scroll.y;
		float_rect.w *= scaling;
		float_rect.h *= scaling;

		return float_rect;
	}

	void Image::SetColor(const uint32_t new_color)
	{
		color = new_color;

		const uint8_t red = color & 0xFF;
		const uint8_t green = (color >> 8) & 0xFF;
		const uint8_t blue = (color >> 16) & 0xFF;
		SDL_SetTextureColorMod(texture, red, green, blue);

		const uint8_t alpha = (color >> 24) & 0xFF;
		SDL_SetTextureAlphaMod(texture, alpha);
	}

	void Image::UI()
	{
		if (ImGui::DragInt2("Size", &size.x, 0.25f)) size = { std::max<int>(0, size.x), std::max<int>(0, size.y) };
		ImGui::SameLine();
		if (ImGui::Button("Reset")) size = { width, height };

		ImVec4 temp_color = ImGui::ColorConvertU32ToFloat4(color);
		if (ImGui::ColorEdit4("Color", &temp_color.x)) SetColor(ImGui::ColorConvertFloat4ToU32(temp_color));
	}

	void Image::CreateTexture(void* data)
	{
		SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, data, 4 * width);
		if (surface == nullptr)
		{
			std::cout << "Failed to create surface: " << SDL_GetError() << '\n';
			return;
		}

		if (texture != nullptr) SDL_DestroyTexture(texture);

		texture = SDL_CreateTextureFromSurface(Renderer::GetRenderer(), surface);
		if (texture == nullptr) std::cout << "Failed to update texture: " << SDL_GetError() << '\n';

		SDL_DestroySurface(surface);

		size = { width, height };
	}

	Text::Text(std::string string, const uint32_t text_color) : text_color{ text_color }, text{ std::move(string) }
	{
		Text::CreateTextTexture();
	}

	void Text::UI()
	{
		UISettings();

		ImGui::Text("Text:");
		if (ImGui::InputTextMultiline("##", &text, ImGui::GetContentRegionAvail())) CreateTextTexture();
	}

	void Text::CreateTextTexture()
	{
		const uint32_t color_alpha = text_color >> 24;
		const uint32_t color_rgb = text_color & 0x00FFFFFF;

		const std::vector<uint8_t>& bitmap_data = font->CreateTextBitmap(text, line_height, width, height);
		std::vector<uint32_t> data(bitmap_data.size(), bg_color);

		for (size_t i = 0; i < bitmap_data.size(); i++)
		{
			const uint32_t alpha = color_alpha * bitmap_data.at(i) / 255;
			uint32_t& data_value = data.at(i);
			data_value = AddColors(color_rgb + (alpha << 24), data_value);
		}

		CreateTexture(data.data());
	}

	void Text::UIFontSelect()
	{
		static std::vector<Fonts::FontPath> available_fonts;
		if (ImGui::BeginCombo("Font", font->GetPath().GetName().c_str()))
		{
			if (available_fonts.empty()) available_fonts = Fonts::AvailableFonts();

			if (ImSearch::BeginSearch())
			{
				ImSearch::SearchBar();

				for (const auto& available_font : available_fonts)
				{
					ImSearch::SearchableItem(available_font.GetName().c_str(),
						[this, &available_font](const char* name)
						{
							if (ImGui::Selectable(name)) SetFont(GetFont(available_font));
						}
					);
				}

				ImSearch::EndSearch();
			}
			ImGui::EndCombo();
		}
		else if (!available_fonts.empty()) available_fonts.clear();
	}

	void Text::UISettings()
	{
		UIFontSelect();

		ImVec4 temp_color = ImGui::ColorConvertU32ToFloat4(text_color);
		if (ImGui::ColorEdit4("Color", &temp_color.x)) SetTextColor(ImGui::ColorConvertFloat4ToU32(temp_color));

		ImVec4 temp_bg_color = ImGui::ColorConvertU32ToFloat4(bg_color);
		if (ImGui::ColorEdit4("Background color", &temp_bg_color.x)) SetBgColor(ImGui::ColorConvertFloat4ToU32(temp_bg_color));

		if (ImGui::DragFloat("Scale", &line_height, 1.0f, 1.0f, std::numeric_limits<float>::max())) CreateTextTexture();

		ImGui::Separator();
	}

	DateTimeText::DateTimeText() : timezones{ current_zone()->name() }
	{
		text = "hh:mmap TMZCITY";
		DateTimeText::CreateTextTexture();
	}

	void DateTimeText::UI()
	{
		UISettings();

		bool changed = false;
		changed |= UITimezoneSelector();

		if (ImGui::InputText("Formatting", &text)) CreateTextTexture();
		ImGui::SameLine();

		static bool show_format_window = false;
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemClicked()) show_format_window = true;

		if (show_format_window) UIFormatWindow(show_format_window);

		changed |= ImGui::Checkbox("LowerCase AM/PM", &lower_am_pm);

		ImGui::Separator();

		if (ImGui::Button("Set current time"))
		{
			date_time.MakeCurrentDateTime();
			changed = true;
		}

		year_month_day date = date_time.GetDate();
		changed |= DragDate("Date", date);

		hh_mm_ss time = date_time.GetTime();
		changed |= DragTime("Time", time);

		if (changed)
		{
			date_time = { date, time };
			CreateTextTexture();
		}
	}

	bool DateTimeText::UITimezoneSelector()
	{
		bool changed = false;

		for (size_t i = 0; i < timezones.size(); i++)
		{
			ImGui::PushID(static_cast<int>(i));

			const std::string_view& timezone = timezones.at(i);

			ImGui::BeginDisabled(i == 0);
			if (ImGui::SmallButton("-"))
			{
				std::swap(timezones.at(i - 1), timezones.at(i));
				changed = true;
			}
			ImGui::EndDisabled();

			ImGui::SameLine();

			ImGui::BeginDisabled(i == timezones.size() - 1);
			if (ImGui::SmallButton("+"))
			{
				std::swap(timezones.at(i), timezones.at(i + 1));
				changed = true;
			}
			ImGui::EndDisabled();

			ImGui::SameLine();

			ImGui::Text("%s", timezone.data());
			ImGui::SameLine();
			if (ImGui::SmallButton("Remove"))
			{
				timezones.erase(timezones.begin() + static_cast<int64_t>(i));
				--i;

				changed = true;
			}
			ImGui::PopID();
		}

		static std::string_view selected_timezone;
		if (ImGui::BeginCombo("Timezone", selected_timezone.data()))
		{
			if (ImSearch::BeginSearch())
			{
				ImSearch::SearchBar();

				const auto& timezone_database = get_tzdb();
				const auto& available_timezones = timezone_database.zones;

				for (const auto& timezone : available_timezones)
				{
					const std::string_view timezone_name = timezone.name();
					if (std::ranges::find(timezones, timezone_name) != timezones.end()) continue;

					ImSearch::SearchableItem(timezone_name.data(),
						[timezone_name](const char* name)
						{
							if (ImGui::Selectable(name))
							{
								selected_timezone = timezone_name;
							}
						});
				}

				ImSearch::EndSearch();
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine();

		ImGui::BeginDisabled(selected_timezone.empty());
		if (ImGui::SmallButton("+"))
		{
			timezones.push_back(selected_timezone);
			selected_timezone = {};
			changed = true;
		}
		ImGui::EndDisabled();

		return changed;
	}

	void DateTimeText::CreateTextTexture()
	{
		const uint32_t color_alpha = text_color >> 24;
		const uint32_t color_rgb = text_color & 0x00FFFFFF;

		std::string formated_text = FormatDateTime(text, timezones, date_time, lower_am_pm);
		const std::vector<uint8_t>& bitmap_data = font->CreateTextBitmap(formated_text, line_height, width, height);
		std::vector<uint32_t> data(bitmap_data.size(), bg_color);

		for (size_t i = 0; i < bitmap_data.size(); i++)
		{
			const uint32_t alpha = color_alpha * bitmap_data.at(i) / 255;
			uint32_t& data_value = data.at(i);
			data_value = AddColors(color_rgb + (alpha << 24), data_value);
		}

		CreateTexture(data.data());
	}

	void DateTimeText::UIFormatWindow(bool& show_format_window) const
	{
		if (ImGui::Begin("Formats", &show_format_window))
		{
			if (ImGui::BeginTable("Formats", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders))
			{
				ImGui::TableSetupColumn("Format");
				ImGui::TableSetupColumn("Result");
				ImGui::TableSetupColumn("Description");
				ImGui::TableHeadersRow();

				const zoned_time zoned_time = date_time.GetZonedTime();
				for (const auto& formatter : DateTime::date_time_formatters)
				{
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", formatter.custom_format.data());

					ImGui::TableSetColumnIndex(1);
					ImGui::PushID(formatter.custom_format.data());

					std::string formatted_date_time{ formatter.replacement_format };
					formatted_date_time = std::vformat("{:" + formatted_date_time += '}', std::make_format_args(zoned_time));
					ImGui::Text("%s", formatted_date_time.c_str());

					ImGui::PopID();

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%s", formatter.description.data());

				}

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("TMZCITY");

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", current_zone()->name().data());

				ImGui::TableSetColumnIndex(2);
				ImGui::Text("The time zone's city name");

				ImGui::EndTable();
			}
		}
		ImGui::End();
	}

	Canvas::Canvas(Image&& image) : image{ std::move(image) }
	{
		CreateRenderTarget();
	}

	Canvas::Canvas(const std::filesystem::path& path, const float scaling) : image{ path, scaling }
	{
		CreateRenderTarget();
	}

	Canvas::~Canvas()
	{
		if (IsValid()) SDL_DestroyTexture(target);
	}

	void Canvas::UpdateScaleAndOffset(const SDL_FPoint& working_area)
	{
		float canvas_height = static_cast<float>(image.GetHeight());
		base_scale = working_area.y / canvas_height;

		const float scale = Renderer::zoom * base_scale;
		const float canvas_width = static_cast<float>(image.GetWidth()) * scale;
		canvas_height *= scale;

		content_size = working_area + ImVec2{ std::max<float>(canvas_width, working_area.x), std::max<float>(canvas_height, working_area.y) };

		render_offset = content_size / 2.0f - ImVec2{ canvas_width, canvas_height } / 2.0f;
	}

	std::future<void> Canvas::Export(std::string&& path) const
	{
		SDL_Renderer* renderer = Renderer::GetRenderer();
		SDL_SetRenderTarget(renderer, target);

		SDL_Surface* canvas_surface = SDL_RenderReadPixels(renderer, nullptr);
		if (canvas_surface == nullptr)
		{
			std::cout << "Failed to get canvas surface: " << SDL_GetError() << '\n';
			return {};
		}

		// Copy the data, so it can be moved to a separate thread
		const int target_width = image.GetWidth();
		const int target_height = image.GetHeight();

		uint32_t* pixel_data = static_cast<uint32_t*>(canvas_surface->pixels);
		std::vector data(pixel_data, pixel_data + static_cast<size_t>(target_width) * static_cast<size_t>(target_height));

		std::future future = std::async(std::launch::async, &AsyncExport, std::move(path), target_width, target_height, std::move(data));
		SDL_DestroySurface(canvas_surface);

		SDL_SetRenderTarget(renderer, nullptr);

		return future;
	}

	void Canvas::CreateRenderTarget()
	{
		target = SDL_CreateTexture(Renderer::GetRenderer(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, image.GetWidth(), image.GetHeight());
		if (target == nullptr)
		{
			std::cout << "Failed to create canvas target: " << SDL_GetError() << '\n';
			return;
		}

		SDL_SetTextureScaleMode(target, SDL_SCALEMODE_NEAREST);

		// We should technically also update the scale offset and content size here, but getting that info here isn't easy, 
		// and it means things will only look wrong for a single frame after creating the canvas
	}
}
