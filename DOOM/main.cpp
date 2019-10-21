#include <windows.h>
#include <cstdint>
#include <cassert>

#include "input.h"
#include "timer.h"
#include "image.h"
#include "render_stuff.h"


using namespace m::Timer;
#define PI 3.14159265359f

bool running = true;
Render_State surface;


LRESULT CALLBACK win_callback(HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lParam)
{
	LRESULT res = 0;

	switch (uMsg)
	{
		case WM_CLOSE:
		case WM_DESTROY:
		{
			running = false;
		} break;

		case WM_SIZE:
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			surface.width = rect.right - rect.left;
			surface.height = rect.bottom - rect.top;

			int size = surface.width * surface.height * sizeof(unsigned int);

			if (surface.memory) VirtualFree(surface.memory, 0 , MEM_RELEASE);
			surface.memory = (uint32_t*)VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

			surface.bitmap_info.bmiHeader.biSize = sizeof(surface.bitmap_info.bmiHeader);
			surface.bitmap_info.bmiHeader.biWidth = surface.width;
			surface.bitmap_info.bmiHeader.biHeight = surface.height;
			surface.bitmap_info.bmiHeader.biPlanes = 1;
			surface.bitmap_info.bmiHeader.biBitCount = 32;
			surface.bitmap_info.bmiHeader.biCompression = BI_RGB;

		} break;

		default:
		{
			res = DefWindowProc(hwnd, uMsg, wparam, lParam);
		}
	}
	return res;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPiv, LPSTR args, int someshit)
{
	// create window class
	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = "Game";
	window_class.lpfnWndProc = win_callback;

	// reg window
	RegisterClass(&window_class);

	// create window
	HWND window = CreateWindow(window_class.lpszClassName, "Game!!!", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1024, 512, 0, 0, hInst, 0);
	HDC hdc = GetDC(window);

	// game vars
	uint32_t column[2048];

	float player_x = 5.499; // player x position
	float player_y = 2.645; // player y position
	float player_a = 1.523; // player view direction



	const size_t map_w = 16; // map width
	const size_t map_h = 16; // map height
	const char map[] =
		"0000111111111000"
		"1              0"
		"1     111111   0"
		"1     0        0"
		"0     0  1110000"
		"0     1        0"
		"0   100        0"
		"0   0   11100  0"
		"0   0   0      0"
		"0   0   1  00000"
		"0       1      0"
		"1       1      0"
		"0       0      0"
		"0 0000000      0"
		"0              0"
		"0001111111100000"; // our game map
	assert(sizeof(map) == map_w * map_h + 1); // +1 for the null terminated string

	const size_t map_cell_w = win_w / map_w / 3;
	const size_t map_cell_h = win_h / map_h / 2;


	// colors
	const size_t ncolors = 10;
	uint32_t* colors = new uint32_t[ncolors];
	for (size_t i = 0; i < ncolors; i++) {
		colors[i] = pack_color((i * 130) % 255, (i * 32)% 255, rand() % 255);
	}

	uint32_t* walltext = NULL; // textures for the walls
	size_t walltext_size;  // texture dimensions (it is a square)
	size_t walltext_cnt;   // number of different textures in the image
	if (!load_texture("walls2.png", walltext, walltext_size, walltext_cnt))
	{
		//std::cerr << "Failed to load wall textures" << std::endl;
		return -1;
	}

	// input
	Input input;

	timer_init(FRAMELOCK60);
	while (running)
	{
		// Input
		MSG msg;
		while (PeekMessage(&msg, window, 0, 0, PM_REMOVE))
		{
			switch (msg.message)
			{
			case WM_KEYUP:
			case WM_KEYDOWN:
			{
				uint32_t vk_code = (uint32_t)msg.wParam;
				bool is_down = ((msg.lParam & (1 << 31)) == 0);

				switch (vk_code)
				{
				case VK_LEFT:
				{
					input.buttons[BUTTON_LROTATE].is_down = is_down;
					input.buttons[BUTTON_LROTATE].changed = true;

				}break;
				case VK_RIGHT:
				{
					input.buttons[BUTTON_RROTATE].is_down = is_down;
					input.buttons[BUTTON_RROTATE].changed = true;

				}break;
				case VK_W:
				{
					input.buttons[BUTTON_UP].is_down = is_down;
					input.buttons[BUTTON_UP].changed = true;

				}break;
				case VK_S:
				{
					input.buttons[BUTTON_DOWN].is_down = is_down;
					input.buttons[BUTTON_DOWN].changed = true;

				}break;
				case VK_A:
				{
					input.buttons[BUTTON_LEFT].is_down = is_down;
					input.buttons[BUTTON_LEFT].changed = true;

				}break;
				case VK_D:
				{
					input.buttons[BUTTON_RIGHT].is_down = is_down;
					input.buttons[BUTTON_RIGHT].changed = true;

				}break;
				}
			}break;
			default:
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}break;
			}
		}

		// Controls
		if (input.buttons[BUTTON_RROTATE].is_down)
			player_a += 0.02f;

		if (input.buttons[BUTTON_LROTATE].is_down)
			player_a -= 0.02f;

		if (input.buttons[BUTTON_UP].is_down)
		{
			player_x += nFrameTime * cosf(player_a) * 0.000005f;
			player_y += nFrameTime * sinf(player_a) * 0.000005f;
		}

		if (input.buttons[BUTTON_DOWN].is_down)
		{
			player_x -= nFrameTime * cosf(player_a) * 0.000005f;
			player_y -= nFrameTime * sinf(player_a) * 0.000005f;
		}

		if (input.buttons[BUTTON_LEFT].is_down)
		{
			player_y += nFrameTime * cosf(player_a) * 0.000005f;
			player_x += nFrameTime * sinf(player_a) * 0.000005f;
		}

		if (input.buttons[BUTTON_RIGHT].is_down)
		{
			player_y -= nFrameTime * cosf(player_a) * 0.000005f;
			player_x -= nFrameTime * sinf(player_a) * 0.000005f;
		}

		// Simulate

		for (int i = 0; i < map_h; i++)
		{
			for (int j = 0; j < map_w; j++)
			{
				int x = j * map_cell_w;
				int y = i * map_cell_h;

				if (map[i * map_w + j] != ' ')
					draw_rectangle(&surface, x, y, map_cell_w, map_cell_h, colors[map[i * map_w + j] - 48]); // -48 get id from char
				else
					draw_rectangle(&surface, x, y, map_cell_w, map_cell_h, pack_color(255, 255, 255));
			}
		}


		draw_rectangle(&surface, player_x* map_cell_w - 2, player_y* map_cell_h - 2, 5, 5, pack_color(60, 60, 60));

		const float fov = PI / 3.0f; // field of view

		for (int i = 0; i < win_w / 2; i++)
		{
			float angle = player_a - fov / 2 + fov * i / float(surface.width / 2);

			// clear screen
			if (i == 0)
			{
				draw_rectangle(&surface, win_w / 2,  0,         win_w / 2, win_h / 2, pack_color(190, 190, 190));
				draw_rectangle(&surface, win_w / 2,  win_h / 2, win_w / 2, win_h / 2, pack_color(255, 255, 255));
			}


			for (float t = 0.0f; t < 20; t += 0.025f)
			{
				float cx = player_x + t * cos(angle);
				float cy = player_y + t * sin(angle);

				size_t pix_x = cx * map_cell_w;
				size_t pix_y = cy * map_cell_h;
				surface.memory[pix_x + pix_y * surface.width] = pack_color(160, 160, 160);

				if (map[int(cx) + int(cy) * map_w] != ' ')
				{
					size_t column_height = min(2048, (win_h / t / cos(angle - player_a)));
					int texid = (int)map[int(cx) + int(cy) * map_w] - 48;


					float hitx = cx - floor(cx + 0.5f); 
					float hity = cy - floor(cy + 0.5f); 
					int x_texcoord = hitx * walltext_size;
					if (std::abs(hity) > std::abs(hitx))
					{
						x_texcoord = hity * walltext_size;
					}
					if (x_texcoord < 0) x_texcoord += walltext_size; 
					assert(x_texcoord >= 0 && x_texcoord < (int)walltext_size);


					texture_column(column, walltext, walltext_size, walltext_cnt, texid, x_texcoord, column_height);
					pix_x = win_w / 2 + i;
					for (size_t j = 0; j < column_height; j++) {
						pix_y = j + win_h / 2 - column_height / 2;
						if (pix_y < 0 || pix_y >= win_h) continue;
						surface.memory[pix_x + pix_y * win_w] = column[j];
					}
					break;
				}
			}

		}


		// Render
		StretchDIBits(hdc, 0, 0, surface.width, surface.height, 0, 0, surface.width, surface.height, surface.memory, &surface.bitmap_info, DIB_RGB_COLORS, SRCCOPY);

		// Timer
		timer_update();

	}
		
	return 0;
}