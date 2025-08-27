#include "UI.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>

#include <portable-file-dialogs.h>
#undef min
#undef max

#include <imgui_internal.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_sdlrenderer3.h>
#include <imsearch/imsearch.h>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "Image.hpp"
#include "Renderer.hpp"



using namespace std::chrono;

namespace UI
{
	namespace
	{
		constexpr float ITEM_SELECT_IMAGE_SCALE{ 4.0f };
		constexpr float START_SELECT_IMAGE_SCALE{ 12.0f };

		constexpr float UI_SCALING_FACTOR{ 1.25f };
		float ui_scale{ 1.0f };

		void SelectedTextMenu()
		{
			if (!Image::IsSelectionValid()) return;

			Image::GetSelection()->UI();
		}

		bool ButtonRightAlign(const char* label)
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			const float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;

			const float offset = ImGui::GetContentRegionAvail().x - size;
			if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

			return ImGui::Button(label);
		}

		// Thanks: https://math.stackexchange.com/questions/1169409/formula-to-best-fit-a-rectangle-inside-another-by-scaling
		ImVec2 GetImageDrawSize(const int image_width, const int image_height, const ImVec2 max_size)
		{
			float new_width = static_cast<float>(image_width);
			float new_height = static_cast<float>(image_height);

			const float scale = std::min<float>(max_size.x / new_width, max_size.y / new_height);
			new_width *= scale;
			new_height *= scale;

			return { new_width, new_height };
		}

		void ImageSelection(size_t& i)
		{
			const float image_area_height = ITEM_SELECT_IMAGE_SCALE * ImGui::GetFontSize();

			const ImVec2 image_area{ image_area_height, image_area_height };
			const float selectable_height = image_area.y + ImGui::GetStyle().FramePadding.y * 2.0f;

			const ImVec2 selectable_origin = ImGui::GetCursorPos();
			if (ImGui::Selectable("##", Image::selection_index == i, ImGuiSelectableFlags_AllowOverlap, { 0.0f, selectable_height })) Image::selection_index = i;

			ImGui::SameLine();

			const auto& image = Image::images.at(i);
			const ImVec2 image_area_middle = selectable_origin + ImVec2{ selectable_height / 2.0f, selectable_height / 2.0f };
			const ImVec2 image_size = GetImageDrawSize(image->GetWidth(), image->GetHeight(), image_area);
			ImGui::SetCursorPos(image_area_middle - image_size / 2.0f);
			ImGui::Image(image->GetTexture(), image_size);

			const ImVec2 image_area_min = (image_area_middle - image_area / 2.0f) - ImVec2{ 1.0f, 1.0f };
			const ImVec2 image_area_max = image_area_min + image_area + ImVec2{ 2.0f, 2.0f };
			ImGui::GetWindowDrawList()->AddRect(ImGui::GetWindowPos() + image_area_min, ImGui::GetWindowPos() + image_area_max, 0xFF808080);

			ImGui::SameLine();

			// Advance the cursor so that the buttons after this are not directly next to the image
			ImGui::SetCursorPosX(image_area_max.x + ImGui::GetStyle().FramePadding.x * 2.0f);

			const float button_pos_y = image_area_middle.y - ImGui::GetFrameHeight() / 2.0f;

			ImGui::BeginDisabled(i == 0);
			ImGui::SetCursorPosY(button_pos_y);
			if (ImGui::Button("-")) std::swap(Image::images.at(i - 1), Image::images.at(i));
			ImGui::EndDisabled();

			ImGui::SameLine();

			ImGui::BeginDisabled(i == Image::images.size() - 1);
			ImGui::SetCursorPosY(button_pos_y);
			if (ImGui::Button("+")) std::swap(Image::images.at(i), Image::images.at(i + 1));
			ImGui::EndDisabled();

			ImGui::SameLine();

			ImGui::SetCursorPosY(button_pos_y);
			if (ButtonRightAlign("Remove"))
			{
				Image::images.erase(Image::images.begin() + static_cast<int64_t>(i));
				--i;
			}
		}

		std::filesystem::path ImagePopup(const std::string& title)
		{
			const std::vector<std::string> filters
			{
				"Image", "*.png *.jpg *.jpeg",
				"PNG Image", "*.png",
				"JPEG Image", "*.jpg *.jpeg"
			};

			const std::vector result = pfd::open_file{ title, "", filters }.result();
			if (result.empty() || !std::filesystem::exists(result.at(0))) return {};

			return result.at(0);
		}

		void AddImageMenu()
		{
			if (ImGui::BeginMenu("Add +"))
			{
				if (ImGui::MenuItem("Image"))
				{
					const std::vector<std::string> filters
					{
						"Image", "*.png *.jpg *.jpeg",
						"PNG Image", "*.png",
						"JPEG Image", "*.jpg *.jpeg"
					};

					const std::filesystem::path result = ImagePopup("Select image");
					if (!result.empty()) Image::images.push_back(std::make_unique<Image::Image>(result));
				}

				if (ImGui::MenuItem("Text")) Image::images.push_back(std::make_unique<Image::Text>());

				if (ImGui::MenuItem("DateTime Text")) Image::images.push_back(std::make_unique<Image::DateTimeText>());

				ImGui::EndMenu();
			}
		}

		std::filesystem::path start_selected_path;
		std::unique_ptr<Image::Image> start_selected_image;

		void CanvasImageSelect()
		{
			if (!ImGui::IsPopupOpen("Select canvas image")) ImGui::OpenPopup("Select canvas image");

			ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size / ImVec2{ 3.0f, 2.0f });
			ImGui::SetNextWindowPos(ImVec2{ ImGui::GetMainViewport()->Size / 2.0f - ImGui::GetCurrentContext()->NextWindowData.SizeVal / 2.0f });
			if (ImGui::BeginPopupModal("Select canvas image", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
			{
				static float scaling = 1.0f;

				if (ImGui::Button("Choose canvas image"))
				{
					std::filesystem::path result = ImagePopup("canvas image");
					if (!result.empty())
					{
						start_selected_path = std::move(result);
						start_selected_image = std::make_unique<Image::Image>(start_selected_path);
						scaling = 1.0f;
					}
				}

				const bool image_selection_valid{ start_selected_image };

				ImVec2 resolution{};
				ImVec2 scaled_resolution{};

				if (image_selection_valid)
				{
					resolution = ImVec2{ static_cast<float>(start_selected_image->GetWidth()), static_cast<float>(start_selected_image->GetHeight()) };
					scaled_resolution = ImVec2{ std::max<float>(scaling * resolution.x, 1.0f), std::max<float>(scaling * resolution.y, 1.0f) };

					const ImVec2 avail_size{ ImGui::GetContentRegionAvail().x, START_SELECT_IMAGE_SCALE * ImGui::GetFontSize() };
					const ImVec2 image_size = GetImageDrawSize(start_selected_image->GetWidth(), start_selected_image->GetHeight(), avail_size);

					ImGui::SetCursorPosX(avail_size.x / 2.0f - image_size.x / 2.0f);
					ImGui::Image(start_selected_image->GetTexture(), image_size);
				}

				ImGui::BeginDisabled(!image_selection_valid);
				ImGui::Text("Image resolution: %.0f x %.0f", resolution.x, resolution.y);

				ImGui::SliderFloat("Scale", &scaling, 0.1f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_ClampOnInput);

				ImGui::Text("Scaled resolution: %.0f x %.0f", scaled_resolution.x, scaled_resolution.y);

				if (ImGui::Button("Select"))
				{
					if (scaling == 1.0f) Image::canvas = std::make_unique<Image::Canvas>(std::move(*start_selected_image));
					else Image::canvas = std::make_unique<Image::Canvas>(start_selected_path, scaling);

					start_selected_image.reset();

					if (Image::canvas->IsValid()) ImGui::CloseCurrentPopup();
					else Image::canvas.reset();
				}
				ImGui::EndDisabled();

				ImGui::EndPopup();
			}
		}

		bool dragging = false;
		ImVec2 mouse_offset;

		void EditorInput()
		{
			if (ImGui::IsWindowHovered())
			{
				if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && !ImGui::IsKeyDown(ImGuiKey_LeftShift))
				{
					// TODO: Correctly zoom to the mouse

					const float wheel_delta = ImGui::GetIO().MouseWheel;
					if (wheel_delta != 0.0f)
					{
						static float mouse_wheel = 0.0f;
						mouse_wheel = std::clamp<float>(mouse_wheel + wheel_delta, -20.0f, 20.0f);

						mouse_offset /= Renderer::zoom * Image::canvas->base_scale;
						Renderer::zoom = std::powf(10, mouse_wheel / 20.0f);
						mouse_offset *= Renderer::zoom * Image::canvas->base_scale;
					}
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					bool clicked_image = false;
					for (size_t i = Image::images.size() - 1; i < Image::images.size(); i--)
					{
						const auto& image = Image::images.at(i);

						const SDL_FRect image_rect = image->GetScreenRect();
						const SDL_FPoint mouse_position = ImGui::GetMousePos();

						clicked_image |= SDL_PointInRectFloat(&mouse_position, &image_rect);
						if (clicked_image)
						{
							Image::selection_index = i;
							break;
						}
					}

					if (!clicked_image) Image::ResetSelection();
				}
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) dragging = true;

				if (ImGui::IsKeyDown(ImGuiKey_Escape)) Image::ResetSelection();

				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
				{
					if (!dragging && Image::IsSelectionValid())
					{
						const SDL_FRect image_rect = Image::GetSelection()->GetScreenRect();
						const SDL_FPoint mouse_position = ImGui::GetMousePos();

						if (SDL_PointInRectFloat(&mouse_position, &image_rect))
						{
							dragging = true;
							mouse_offset = ImVec2{ image_rect.x, image_rect.y } - mouse_position;
						}
					}
				}
			}

			if (dragging)
			{
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
				{
					const ImVec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
					ImGui::SetScrollX(ImGui::GetScrollX() - mouse_delta.x);
					ImGui::SetScrollY(ImGui::GetScrollY() - mouse_delta.y);
					ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
				}

				if (Image::IsSelectionValid() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
				{
					const auto& selection = Image::GetSelection();

					const ImVec2 new_position = ((ImGui::GetMousePos() + mouse_offset) - Image::canvas->render_offset + Renderer::scroll) / (Image::canvas->base_scale * Renderer::zoom);
					selection->x = static_cast<int>(new_position.x);
					selection->y = static_cast<int>(new_position.y);
				}
			}

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle) || ImGui::IsMouseReleased(ImGuiMouseButton_Left)) dragging = false;

			Renderer::scroll.x = ImGui::GetScrollX();
			Renderer::scroll.y = ImGui::GetScrollY();
		}

		ImVec2 working_area{};
	}

	void UpdateScale()
	{
		SDL_Window* window = Renderer::GetWindow();
		ImGui::GetStyle().FontScaleDpi = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(window)) * UI_SCALING_FACTOR * ui_scale;
	}

	void Setup()
	{
		SDL_Window* window = Renderer::GetWindow();
		SDL_Renderer* renderer = SDL_GetRenderer(window);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImSearch::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.LogFilename = nullptr;

		ImGui::StyleColorsDark();

		UpdateScale();

		ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
		ImGui_ImplSDLRenderer3_Init(renderer);
	}

	void ProcessEvents(const SDL_Event* event)
	{
		ImGui_ImplSDL3_ProcessEvent(event);
	}

	std::future<void> export_future;

	void Update(SDL_Renderer* renderer)
	{
		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		if (!Image::canvas)
		{
			CanvasImageSelect();

			ImGui::Render();
			ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
			return;
		}

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::MenuItem("Export"))
			{
				const std::filesystem::path result = pfd::save_file{ "Export png", "", {"PNG image", "*.png"} }.result();
				if (!result.empty())
				{
					export_future = Image::canvas->Export(result.generic_string());
				}
			}

			ImGui::EndMainMenuBar();
		}

		int render_height;
		if (!SDL_GetCurrentRenderOutputSize(renderer, nullptr, &render_height))
			std::cout << "Failed to get the renderer height: " << SDL_GetError() << '\n';

		constexpr ImGuiWindowFlags edit_window_flags =
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoNav;

		// Window position and height are offset with the frame height to account for the main menu bar at the top
		ImGui::SetNextWindowPos(ImVec2{ 0.0f, ImGui::GetFrameHeight() });
		ImGui::SetNextWindowSize(working_area + ImVec2{ 0.0f, ImGui::GetStyle().ScrollbarSize });

		// Sets the content size, this is so we can use the imgui scrollbars since its convenient
		ImGui::SetNextWindowContentSize(Image::canvas->content_size);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
		if (ImGui::Begin("##Editor window", nullptr, edit_window_flags))
		{
			EditorInput();
		}
		ImGui::End();
		ImGui::PopStyleVar();


		if (ImGui::Begin("Image items", nullptr, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				AddImageMenu();

				ImGui::EndMenuBar();
			}

			for (size_t i = Image::images.size() - 1; i < Image::images.size(); i--)
			{
				ImGui::PushID(static_cast<int>(i));

				ImageSelection(i);

				ImGui::PopID();
			}
		}
		ImGui::End();

		if (ImGui::Begin("Selected image"))
		{
			SelectedTextMenu();
		}
		ImGui::End();

		if (!ImGui::IsPopupOpen("Exporting") && export_future.valid()) ImGui::OpenPopup("Exporting");

		ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size / ImVec2{ 6.0f, 4.0f });
		ImGui::SetNextWindowPos(ImVec2{ ImGui::GetMainViewport()->Size / 2.0f - ImGui::GetCurrentContext()->NextWindowData.SizeVal / 2.0f });
		if (ImGui::BeginPopupModal("Exporting", nullptr,  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			const std::string text = "Exporting image...";
			const ImVec2 text_size = ImGui::CalcTextSize(text.c_str());

			ImGui::SetCursorPos(ImGui::GetContentRegionAvail() / 2.0f - text_size / 2.0f);
			ImGui::Text("%s", text.c_str());

			if (export_future.wait_for(seconds{ 0 }) == std::future_status::ready)
			{
				export_future.get();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		int window_width, window_height;
		SDL_GetRenderOutputSize(Renderer::GetRenderer(), &window_width, &window_height);

		working_area = ImVec2{ static_cast<float>(window_width), static_cast<float>(window_height) };
		working_area.y -= ImGui::GetFrameHeight() + ImGui::GetStyle().ScrollbarSize;

		Image::canvas->UpdateScaleAndOffset(working_area);

		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
	}

	void Exit()
	{
		ImGui_ImplSDLRenderer3_Shutdown();
		ImGui_ImplSDL3_Shutdown();

		ImSearch::DestroyContext();
		ImGui::DestroyContext();
	}
}
