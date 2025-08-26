#pragma once

struct SDL_FPoint;
struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;

namespace UI
{
	void UpdateScale();

	void Setup();
	void ProcessEvents(const SDL_Event* event);
	void Update(SDL_Renderer* renderer);
	void Exit();
}