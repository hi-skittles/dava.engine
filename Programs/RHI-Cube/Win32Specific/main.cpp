#include "DAVAEngine.h"

#if defined(__DAVAENGINE_WIN32__)

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    return DAVA::Core::Run(0, 0, hInstance);
}

#elif defined(__DAVAENGINE_WIN_UAP__)

[Platform::MTAThread] int main(Platform::Array<Platform::String ^> ^ args)
{
    return DAVA::Core::Run(0, 0, 0);
}

#endif // defined(__DAVAENGINE_WIN32__)

/*int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR    lpCmdLine, int       nCmdShow)
{
	// find leaks mega-string: {,,msvcr71d.dll}_crtBreakAlloc
	//
	// POSSIBLE LEAKS LIST:
	// remember -- always clear all allocated data for static STL containers
	// 
	return 0;
}*/