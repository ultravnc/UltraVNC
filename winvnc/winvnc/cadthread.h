#if (!defined(_WINVNC_VNCCAD))
#define _WINVNC_VNCCAD

#include <winsock2.h>
#include "windows.h"
#include "stdhdrs.h"

class vncCad
{
public:
	vncCad();
	static DWORD WINAPI Cadthread(LPVOID lpParam);
	static void Enable_softwareCAD_elevated();
	static bool IsSoftwareCadEnabled();
	static bool ISUACENabled();
	static void Enable_softwareCAD();
	static void delete_softwareCAD();
	static void delete_softwareCAD_elevated();
private:	
};

#endif