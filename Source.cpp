#include <iostream>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>

#include "Fonts.hpp"
#include "Image.hpp"
#include "Renderer.hpp"
#include "UI.hpp"

namespace
{
	void ProcessEvents(bool& running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			UI::ProcessEvents(&event);

			switch (event.type)
			{
			case SDL_EVENT_QUIT:
				running = false;
				break;

			case SDL_EVENT_KEY_DOWN:
				if (event.key.scancode == SDL_SCANCODE_F11)
				{
					Renderer::ToggleFullscreen();
				}
				break;

			case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
			case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
				UI::UpdateScale();
				break;

			default:
				break;
			}
		}
	}
}

int main(int, char* [])
{
	Fonts::SetupDefaultFont();

	SDL_Renderer* renderer = Renderer::CreateRenderer();
	if (renderer == nullptr)
	{
		std::cout << "Failed to create renderer, exiting" << '\n';
		SDL_Quit();
		return 1;
	}

	UI::Setup();

	bool running = true;
	do
	{
		ProcessEvents(running);

		if (!SDL_RenderClear(renderer))
		{
			std::cout << "Failed to clear the renderer: " << SDL_GetError() << '\n';
			continue;
		}

		Renderer::Update();
		UI::Update(renderer);

		if (!SDL_RenderPresent(renderer)) std::cout << "Failed to present renderer: " << SDL_GetError() << '\n';

	} while (running);

	Image::images.clear();
	Image::canvas.reset();

	UI::Exit();
	SDL_Quit();

	return 0;
}
