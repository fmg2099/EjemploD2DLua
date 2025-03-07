
#include <iostream>
#include "Windows.h"
#include "D2D1.h"
#include "D2D1Helper.h"
#include "lua.hpp"
#pragma comment(lib, "d2d1")




//variables de windows api
HINSTANCE hInstance;
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void closeApp( HWND );
HWND hWnd;
enum EWindowStyle {
	standard = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
	borderless = WS_POPUP,
	aero_borderless = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
};

//Cosas de Direct2D
ID2D1Factory* d2dFactory = NULL;
ID2D1HwndRenderTarget* renderTarget;
ID2D1SolidColorBrush* brush;
D2D1_COLOR_F clearColor;
HRESULT CreateGraphicsResources();
void DiscardGraphicsResources();
void CalculateLayout();

//////////////////////////////////////////////////////////////////////////
// Funciones de ayuda
//////////////////////////////////////////////////////////////////////////
void Log(LPCWCHAR message)
{
	std::wcout << message << std::endl;
}
void Log(const char* msg)
{
	std::cout << msg << std::endl;
}
template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// Funciones expuestas a lua
//////////////////////////////////////////////////////////////////////////

int Clear(lua_State* L)
{
	if (renderTarget != NULL)
	{
		float r = (float)lua_tonumber(L, 1) /255;
		float g = (float)lua_tonumber(L, 2) /255;
		float b = (float)lua_tonumber(L, 3) /255;
		clearColor = D2D1::ColorF(r, g, b);
		renderTarget->Clear(clearColor);
	}
	return 0;
}

int SetBrushColor(lua_State* L)
{
	if (renderTarget != NULL)
	{
		int r = (int)lua_tonumber(L, 1);
		int g = (int)lua_tonumber(L, 2);
		int b = (int)lua_tonumber(L, 3);

		HRESULT hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(r/(float)255, g/ (float)255, b/ (float)255), &brush);
	}
	return 0;
}
int DrawCircle(lua_State* L)
{
	if (renderTarget != NULL)
	{
		float x = (float)lua_tonumber(L, 1);
		float y = (float)lua_tonumber(L, 2);
		float radius = (float)lua_tonumber(L, 3);
		renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), brush);
	}
	return 0;
}

int DrawRect(lua_State* L)
{
	if (renderTarget != NULL)
	{
		float x = (float)lua_tonumber(L, 1);
		float y = (float)lua_tonumber(L, 2);
		float w = (float)lua_tonumber(L, 3);
		float h = (float)lua_tonumber(L, 4);
		renderTarget->FillRectangle(D2D1::RectF(x, y, x + w, y + h), brush);
	}
	return 0;
}

int DrawLine(lua_State* L)
{
	if (renderTarget != NULL)
	{
		float x1 = (float)lua_tonumber(L, 1);
		float y1 = (float)lua_tonumber(L, 2);
		float x2 = (float)lua_tonumber(L, 3);
		float y2 = (float)lua_tonumber(L, 4);
		renderTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush);
	}
	return 0;
}

void luaDraw(lua_State* L, float dt)
{
	lua_getglobal(L, "Draw");
	if (lua_isfunction(L, -1))
	{
		//Log(L"Calling draw function from lua");
		lua_pushnumber(L, dt);
		if (lua_pcall(L, 1, 0, 0) != 0)
		{
			Log(L"Error calling draw function from lua");
			Log(lua_tostring(L, -1));
		}
	}
	else
	{
		Log(L"Draw function not found in lua");
	}
}
//para crear la biblioteca de funciones en lua
int lua_mymodule(lua_State* L)
{
	static const luaL_Reg myModule[] =
	{
	{ "Clear", Clear },
	{ "SetBrushColor", SetBrushColor },
	{ "DrawCircle", DrawCircle },
	{ "DrawRect", DrawRect },
	{ "DrawLine", DrawLine },
	{ NULL, NULL }
	};
	luaL_newlib(L, myModule);
	return 1;
}




int main(int argc, char** argv)
{
	Log(L"hólá múndö");

	//////////////////////////////////////////////////////////////////////////
	// Inicializacion de la ventana
	//////////////////////////////////////////////////////////////////////////

	hInstance = GetModuleHandle(NULL);
	LPCWSTR szTitle = L"Ventana";
	LPCWSTR szWindowClass = L"VentanaClass";

	WNDCLASSEXW wcex;
	ZeroMemory(&wcex, sizeof(WNDCLASSEXW)); //recordad el error en clase del 5mar25
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION)); //icon for the window
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	//wcex.hbrBackground = CreateSolidBrush(RGB(8, 8, 8)); //background color
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;//MAKEINTRESOURCEW(IDC_GLDX);
	wcex.lpszClassName = szWindowClass;
	RegisterClassExW(&wcex);

	int desktopWidth = GetSystemMetrics(SM_CXSCREEN);
	int desktopHeight = GetSystemMetrics(SM_CYSCREEN);
	int windowWidth = 800;
	int windowHeight = 600;

	hWnd = CreateWindowW(szWindowClass,
		szTitle,
		EWindowStyle::borderless,
		(desktopWidth - windowWidth) / 2,  //position x at center
		(desktopHeight - windowHeight) / 2,  //position y at center
		windowWidth, windowHeight,
		nullptr,
		nullptr,
		hInstance,
		nullptr);
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	//////////////////////////////////////////////////////////////////////////
	// Inicializacion de direct2D - se hace en WM_CREATE
	//////////////////////////////////////////////////////////////////////////
	

	//////////////////////////////////////////////////////////////////////////
	// Inicializacion de lua
	//////////////////////////////////////////////////////////////////////////

	lua_State* L = luaL_newstate();
	//guardar el estado de lua para poder usarlo en el wndproc
	SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(L));

	ShowWindow(hWnd, SW_SHOW);

	luaL_openlibs(L);
	luaL_requiref(L, "SimpleDraw", lua_mymodule, 1); //SimpleDraw es el nombre de la biblioteca a importar en lua
	lua_pop(L, 1);
	//luaL_dostring(L, "print('Hello from Lua!')");
	if (luaL_dofile(L, "main.lua"))
	{
		Log(L"Error cargando el scripto main.lua");
		Log(lua_tostring(L, -1));
	}
	else
	{
		Log(L"Lua file loaded");
	}

	luaDraw(L, 0.0f);


	//////////////////////////////////////////////////////////////////////////
	// Ciclo principal
	//////////////////////////////////////////////////////////////////////////

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND _hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	lua_State* L = reinterpret_cast<lua_State*>(GetWindowLongPtr(_hWnd, GWLP_USERDATA));

	switch (message)
	{
		case WM_CREATE:
		{
			//cornflower blue para recordar xna
			clearColor = D2D1::ColorF(D2D1::ColorF::CornflowerBlue);
			if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory)))
			{
				Log(L"Failed to create D2D1 factory");
				return -1;
			}
			else
			{
				Log(L"D2D1 factory created");
			}
			//crear timer para el ciclo de update y draw
			SetTimer(_hWnd, 1, 16, NULL); //id 1 y 16ms para 60fps aprox
		}
		break;

		case WM_TIMER:
		{
			InvalidateRect(_hWnd, NULL, FALSE);
		} break;

		case WM_KEYDOWN:
		{
			PWCHAR buffer = new WCHAR[100];
			wsprintfW(buffer, L"Key down: %i", (int)wParam);
			Log(buffer);
			if (wParam == VK_ESCAPE)
			{
				closeApp(_hWnd);
			}
		}
			break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			//beginpaint de windows
			BeginPaint(_hWnd, &ps);
			HRESULT hr = CreateGraphicsResources();
			if (SUCCEEDED(hr))
			{
				renderTarget->BeginDraw();
				if (L != NULL)
					luaDraw(L, 0.0f);
				else
					Log(L"Lua state is null");
				renderTarget->EndDraw();
			}			
			EndPaint(_hWnd, &ps);
			return 0;
		}
		case WM_SIZE:
		{
			if (renderTarget != NULL)
			{
				RECT rc;
				GetClientRect(_hWnd, &rc);
				D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
				renderTarget->Resize(size);
				InvalidateRect(_hWnd, NULL, FALSE);
			}
			break;
		}
		case WM_DESTROY:
			DiscardGraphicsResources();
			SafeRelease(&d2dFactory);
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(_hWnd, message, wParam, lParam);
	}
	return 0;
}

void closeApp(HWND hWnd)
{
	DestroyWindow(FindWindowW(NULL, L"Ventana"));
}

HRESULT CreateGraphicsResources()
{
	HRESULT hr = S_OK;
	if (renderTarget == NULL)
	{
		RECT rc;
		GetClientRect(hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
		//crea el render target en la ventana
		hr = d2dFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hWnd, size),
			&renderTarget);

		if (SUCCEEDED(hr))
		{
			const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
			hr = renderTarget->CreateSolidColorBrush(color, &brush);

			if (SUCCEEDED(hr))
			{
				CalculateLayout();
			}
		}
	}
	return hr;
}

void CalculateLayout()
{
	if (renderTarget != NULL)
	{
		D2D1_SIZE_F size = renderTarget->GetSize();
		const float x = size.width / 2;
		const float y = size.height / 2;
		const float radius = min(x, y);
		//ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
	}
}

void DiscardGraphicsResources()
{
	SafeRelease(&renderTarget);
	SafeRelease(&brush);
}