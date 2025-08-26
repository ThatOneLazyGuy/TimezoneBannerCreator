#include "Renderer.hpp"

#include <iostream>
#include <filesystem>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_init.h>

#include "Image.hpp"

namespace Renderer
{
	namespace
	{
		constexpr float GRID_APPEAR_ZOOM_DEPTH{ 6.0f };
		constexpr float CHECKERBOARD_SCALE{ 8.0f };

		constexpr uint32_t BACKGROUND_COLOR{ 0xFF404040 };
		constexpr uint32_t SELECTION_COLOR{ 0xFF0000FF };
		constexpr uint32_t GRID_COLOR{ 0xFF606060 };

		SDL_Window* window;
		SDL_Renderer* renderer;

		uint32_t checkerboard_data[4]{ 0xFF8F8F8F, 0xFFBFBFBF, 0xFFBFBFBF, 0xFF8F8F8F };
		SDL_Texture* checkerboard_texture{ nullptr };

		void SetDrawColor(const uint32_t color)
		{
			if (!SDL_SetRenderDrawColor(renderer, color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF, color >> 24))
				std::cout << "Failed to set draw color: " << SDL_GetError() << '\n';
		}
	}

	SDL_Renderer* CreateRenderer()
	{
		SDL_Init(SDL_INIT_VIDEO);

		SDL_Rect display_bounds{ 0, 0, 1920, 1080 };
		if (!SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &display_bounds))
			std::cout << "Failed to get primary display bounds: " << SDL_GetError() << '\n';

		constexpr SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_MAXIMIZED;
		SDL_CreateWindowAndRenderer("Timezone Banner Creator", display_bounds.w / 2, display_bounds.h / 2, window_flags, &window, &renderer);
		if (window == nullptr)
		{
			std::cout << "Failed to create window: " << SDL_GetError() << '\n';
			return nullptr;
		}
		if (renderer == nullptr)
		{
			std::cout << "Failed to create renderer: " << SDL_GetError() << '\n';
			SDL_DestroyWindow(window);
			return nullptr;
		}

		if (!SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_ADAPTIVE)) SDL_SetRenderVSync(renderer, 1);

		SetDrawColor(BACKGROUND_COLOR);

		SDL_Surface* checkerboard_surface = SDL_CreateSurfaceFrom(2, 2, SDL_PIXELFORMAT_RGBA32, checkerboard_data, 8);
		if (checkerboard_surface == nullptr)
		{
			std::cout << "Failed to create checkerboard surface: " << SDL_GetError() << '\n';
		}
		else
		{
			checkerboard_texture = SDL_CreateTextureFromSurface(renderer, checkerboard_surface);
			if (checkerboard_texture == nullptr) std::cout << "Failed to create checkerboard texture: " << SDL_GetError() << '\n';
			else SDL_SetTextureScaleMode(checkerboard_texture, SDL_SCALEMODE_NEAREST);
		}

		return renderer;
	}

	SDL_Window* GetWindow() { return window; }
	SDL_Renderer* GetRenderer() { return renderer; }

	void ToggleFullscreen()
	{
		{ SDL_SetWindowFullscreen(window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)); }
	}

	void Update()
	{
		if (!Image::canvas) return;

		SDL_SetRenderTarget(renderer, Image::canvas->target);

		Image::canvas->image.Render(renderer);
		for (const auto& image : Image::images)
		{
			image->Render(renderer);
		}

		SDL_SetRenderTarget(renderer, nullptr);

		const SDL_FRect canvas_rect = Image::canvas->image.GetScreenRect();
		if (checkerboard_texture != nullptr) SDL_RenderTextureTiled(renderer, checkerboard_texture, nullptr, CHECKERBOARD_SCALE, &canvas_rect);

		SDL_RenderTexture(renderer, Image::canvas->target, nullptr, &canvas_rect);

		// Draw the grid lines if we have zoomed in enough to see them
		if (zoom > GRID_APPEAR_ZOOM_DEPTH)
		{
			SetDrawColor(GRID_COLOR);

			// Used to calculate the index of the first and last pixels of the canvas being rendered, we can then use this to only draw the grid lines that are on screen
			const float scaling = zoom * Image::canvas->base_scale;
			SDL_FRect unscaled_canvas_rect = canvas_rect;
			unscaled_canvas_rect.x /= scaling;
			unscaled_canvas_rect.y /= scaling;
			(unscaled_canvas_rect.w /= scaling) += unscaled_canvas_rect.x;
			(unscaled_canvas_rect.h /= scaling) += unscaled_canvas_rect.y;

			const int first_vertical_line = static_cast<int>(-std::min<float>(0, unscaled_canvas_rect.x)) + 1;
			const int first_horizontal_line = static_cast<int>(-std::min<float>(0, unscaled_canvas_rect.y)) + 1;

			int render_width, render_height;
			SDL_GetRenderOutputSize(renderer, &render_width, &render_height);

			const int last_vertical_line = static_cast<int>(std::min<float>(unscaled_canvas_rect.w, static_cast<float>(render_width) / scaling) - unscaled_canvas_rect.x);
			const int last_horizontal_line = static_cast<int>(std::min<float>(unscaled_canvas_rect.h, static_cast<float>(render_height) / scaling) - unscaled_canvas_rect.y);

			const SDL_FPoint offset{ Image::canvas->render_offset.x - scroll.x, Image::canvas->render_offset.y - scroll.y };

			const float background_height = static_cast<float>(Image::canvas->image.GetHeight()) * scaling + offset.y;
			for (int i = first_vertical_line; i <= last_vertical_line; i++)
			{
				const float horizontal_position = static_cast<float>(i) * scaling + offset.x;
				SDL_RenderLine(renderer, horizontal_position, offset.y, horizontal_position, background_height);
			}

			const float background_width = static_cast<float>(Image::canvas->image.GetWidth()) * scaling + offset.x;
			for (int i = first_horizontal_line; i <= last_horizontal_line; i++)
			{
				const float vertical_position = static_cast<float>(i) * scaling + offset.y;
				SDL_RenderLine(renderer, offset.x, vertical_position, background_width, vertical_position);
			}
		}

		if (Image::IsSelectionValid())
		{
			SetDrawColor(SELECTION_COLOR);

			const auto& selected_image = Image::GetSelection();
			const SDL_FRect selection_rect = selected_image->GetScreenRect();
			if (!SDL_RenderRect(renderer, &selection_rect)) std::cout << "Failed to render selection rect: " << SDL_GetError() << '\n';
		}

		SetDrawColor(BACKGROUND_COLOR);
	}

	void Exit()
	{
		if (checkerboard_texture != nullptr) SDL_DestroyTexture(checkerboard_texture);

		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
	}
}
