
#include <windows.h>

#include "Gut.h"
#include "GUtWin32.h"

static HWND g_hWnd = NULL;
static HINSTANCE g_hInstance = NULL;
static HANDLE g_ThreadHandle = NULL;
static unsigned long g_ThreadID = 0;

static int g_iWindowPosX = 100;
static int g_iWindowPosY = 100;
static int g_iWindowWidth = 512;
static int g_iWindowHeight = 512;

void GutSetWindowHandleWin32(HWND hWnd)
{
	g_hWnd = hWnd;
}

HWND GutGetWindowHandleWin32(void)
{
	return g_hWnd;
}

HINSTANCE GutGetWindowInstanceWin32(void)
{
	return g_hInstance;
}

void GutGetWindowSize(int &w, int &h)
{
	w = g_iWindowWidth;
	h = g_iWindowHeight;
}

void GutGetMouseState(int &x, int &y, int button[3])
{
	POINT p;

	GetCursorPos(&p);
	ScreenToClient(g_hWnd, &p);
	
	x = p.x;
	y = p.y;
	
	button[0] = GetKeyState(VK_LBUTTON) & 0x80 ? 1 : 0;
	button[1] = GetKeyState(VK_MBUTTON) & 0x80 ? 1 : 0;
	button[2] = GetKeyState(VK_RBUTTON) & 0x80 ? 1 : 0;
}

// `窗口信息处理函数`
static LRESULT WINAPI WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// `检查信息种类`
	switch (message)
	{
		case WM_DESTROY:
		{
			// `如果窗口被关掉了, 就主动发出程序该结束的信息.`
			PostQuitMessage(0);
			break;
		}

		case WM_SIZE: // `窗口大小被改变`
		{
			int w = LOWORD(lParam);
			int h = HIWORD(lParam);
			g_iWindowWidth = w;
			g_iWindowHeight = h;
			if ( g_GutCallBack.OnSize && GutGetGraphicsDeviceType()!=GUT_UNKNOWN ) 
			{
				// `有设置窗口大小改变的处理函数的话, 就去调用它.`
				g_GutCallBack.OnSize(w, h);
			}
			break;
		}

		case WM_PAINT: // `窗口需要重画`
		{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
			if ( g_GutCallBack.OnPaint )
			{
				// `有设置重绘信息处理函数的话, 就去调用它.`
				g_GutCallBack.OnPaint();
			}
			break;
		}

		default: // `其它信息都直接交给内置的信息处理函数去做`
		{
			return DefWindowProc(hwnd, message, wParam, lParam);
			break;
		}
	}

	return 0;
} // WndProc

void GutCloseWindow(void)
{
	PostQuitMessage(0);
}

// `检查Windows操作系统传来的信息`
bool GutProcessMessage(void)
{
	MSG	msg;
	// `看看是否任何信息`
	if ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
	{
		// `有信息的话, 就把它送给信息处理函数.`
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if ( msg.message==WM_QUIT )
		{
			// `收到关闭窗口的信息会返回false来结束循环.`
			return false;
		}
	}

	return true;
}

bool GutFullScreen(int width, int height)
{
	char *title = "fullscreen";

	DEVMODE dmScreenSettings;								// Device Mode
	memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
	dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
	dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
	dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
	dmScreenSettings.dmBitsPerPel	= 32;					// Selected Bits Per Pixel
	dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

	// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
	if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
	{
		// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
		return false;
	}


	WNDCLASS	window_class;

	memset(&window_class, 0, sizeof(WNDCLASS));

	// `设置窗口类型`
	window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	// `设置信息处理函数为WndProc`
	window_class.lpfnWndProc = WndProc;
	// `获得目前运行的程序`
	window_class.hInstance = GetModuleHandle(NULL);
	// `使用默认的鼠标`
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	// `背景设为黑色, 其实是否设置没有区别.`
	window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	// `给定一个注册窗口时使用的名字`
	window_class.lpszClassName = title;
	// `向操作系统注册一个新的窗口`
	if (RegisterClass(&window_class) == 0)
		return 0;
	
	DWORD dwExStyle=WS_EX_APPWINDOW;	// Window Extended Style
	DWORD dwStyle=WS_POPUP;				// Windows Style
	ShowCursor(FALSE);					// Hide Mouse Pointer

	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values

	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	g_iWindowPosX = g_iWindowPosY = 0;
	g_iWindowWidth = width;
	g_iWindowHeight = height;

	HWND window_handle = CreateWindowEx(	
		dwExStyle,
		"fullscreen",
		title,
		dwStyle |
		WS_CLIPSIBLINGS |
		WS_CLIPCHILDREN,
		0, 0,
		WindowRect.right-WindowRect.left,
		WindowRect.bottom-WindowRect.top,
		NULL,
		NULL,
		window_class.hInstance,
		NULL
		);

	if (window_handle == NULL)
		return false;

	// `记录下用来代表窗口和目前运行程序的对象, 以后可以再用到.`
	g_hWnd = window_handle;
	g_hInstance = window_class.hInstance;
	// `显示窗口`
	ShowWindow(window_handle, SW_SHOWNORMAL);
	SetActiveWindow(window_handle);
	SetForegroundWindow(window_handle);	
	SetFocus(window_handle);
	
	return true;
}


// `打开窗口`
// `x, y = 窗口左上角的位置`
// `width, height = 窗口大小`
// `title = 窗口标题栏所显示的文字`
bool GutCreateWindow(int x, int y, int width, int height, const char *title)
{
	static bool registered = false;

	WNDCLASS	window_class;

	memset(&window_class, 0, sizeof(WNDCLASS));

	// `设置窗口类型`
	window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	// `设置信息处理函数为WndProc`
	window_class.lpfnWndProc = WndProc;
	// `获得目前运行的程序`
	window_class.hInstance = GetModuleHandle(NULL);
	// `使用默认的鼠标`
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	// `背景设为黑色, 其实是否设置没有区别.`
	window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	// `给定一个注册窗口时使用的名字`
	window_class.lpszClassName = title;
	// `向操作系统注册一个新的窗口`
	if ( !registered )
	{
		if (RegisterClass(&window_class) == 0)
			return false;
		registered = true;
	}

	DWORD window_style;

	if ( g_GutCallBack.OnSize )
	{
		// `如果有处理窗口缩放的信息, 才容许鼠标改变窗口大小.`
		window_style = WS_OVERLAPPEDWINDOW;
	}
	else
	{
		// `如果是否处理窗口缩放的信息, 不容许鼠标改变窗口大小.`
		window_style = WS_BORDER | WS_SYSMENU;
	}

	if ( x<0 || y<0 )
	{
		// 使用默认值
		x = g_iWindowPosX;
		y = g_iWindowPosY;
	}

	if ( width<0 || height < 0 )
	{
		// 使用默认值
		width = g_iWindowWidth;
		height = g_iWindowHeight;
	}

	// `计算窗口的边线会占掉的点数, 必要时要增加窗口大小.`
	RECT window_rect;
	SetRect(&window_rect, x, y, x+width, y+height);
	AdjustWindowRect(&window_rect, window_style, FALSE);
	
	g_iWindowPosX = x;
	g_iWindowPosY = y;
	g_iWindowWidth = width;
	g_iWindowHeight = height;

	// `准备工作完成, 打开一个窗口.`
	HWND window_handle = CreateWindowEx(
		WS_EX_APPWINDOW,
		title,
		title,
		window_style,
		window_rect.left,	// x
		window_rect.top,	// y
		window_rect.right - window_rect.left,	// width
		window_rect.bottom - window_rect.top,	// height
		NULL,
		NULL,
		window_class.hInstance,
		NULL
		);

	if (window_handle == NULL)
		return false;

	// `记录下用来代表窗口和目前运行程序的对象, 以后可以再用到.`
	g_hWnd = window_handle;
	g_hInstance = window_class.hInstance;
	// `显示窗口`
	ShowWindow(window_handle, SW_SHOWNORMAL);
	SetActiveWindow(window_handle);
	
	return true;
}
