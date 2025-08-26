#pragma once

#include <SDL3/SDL_rect.h>

namespace std::filesystem
{
	class path;
}

struct SDL_Surface;
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace Renderer
{
	inline float zoom{ 1.0f };
	inline SDL_FPoint scroll{};

	SDL_Renderer* CreateRenderer();

	SDL_Window* GetWindow();
	SDL_Renderer* GetRenderer();
	void ToggleFullscreen();

	void Update();

	void Exit();
}