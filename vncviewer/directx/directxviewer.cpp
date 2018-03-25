#include "directxviewer.h"
#include <stdio.h>

int pitch=0;
bool paintbuzy;
#define ID_REQUEST_REFRESH              111

static INT MaskToShift(DWORD mask)
{
    int shift;

    if (mask==0)
        return 0;

    shift=0;
    while ((mask&1)==0) {
        mask>>=1;
        shift++;
    }
    return shift;
}

ViewerDirectxClass::ViewerDirectxClass()
{
	directxlocked=false;
	surface=0;
	pD3DDevice9=NULL;
	pD3D9=NULL;
	devicelost=true;
	D3DLibrary = NULL;
	d3dCreate=NULL;
	D3DLibrary = LoadLibrary("d3d9");
	if (!D3DLibrary) 
	{
		return;
	}

    d3dCreate = (D3DCREATETYPE) GetProcAddress(D3DLibrary, "Direct3DCreate9");
	if(!d3dCreate)
	{
		
		FreeLibrary(D3DLibrary);
		D3DLibrary=NULL;
		return;
	}
}

ViewerDirectxClass::~ViewerDirectxClass()
{
	if (D3DLibrary)
	{
		DestroyD3D();
		FreeLibrary(D3DLibrary);
	}
}

void 
ViewerDirectxClass::DestroyD3D(void)
{
	if (directxlocked==true) Afterupdate();
	if (surface)
		{
			surface->Release();
			surface = 0;
		}
	if (pD3DDevice9)
	{
		pD3DDevice9->Release();
		pD3DDevice9 = NULL;
	}

	if (pD3D9)
	{
		pD3D9->Release();
		pD3D9 = NULL;
	}
}

HRESULT
ViewerDirectxClass::InitD3D(HWND hwnd, HWND hwndm,int width, int height, bool fullscreen,int bit,int shift)
{
	
	if (!D3DLibrary) return E_FAIL;
	//PITCHTEST
	parent_hwnd=hwnd;
	mwidth=width;
	mheight=height;
	mbit=bit;
	mshift=shift;
	hwnd_sendm=hwndm;
	// create the IDirect3D9 object
	pD3D9 = (*d3dCreate)(D3D_SDK_VERSION);

	//pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D9 == NULL)
	{
		
		return E_FAIL;
	}

	
	// get the display mode
	D3DDISPLAYMODE d3ddm;
	pD3D9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	// set the presentation parameters
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferWidth = width;
	d3dpp.BackBufferHeight = height;
	d3dpp.BackBufferCount = 2;
		if (bit==32) 
	{
	format=d3ddm.Format;
	}
	else if (bit==16)
	{
		if (shift==MaskToShift(0xF800))
		{
			d3ddm.Format=D3DFMT_R5G6B5;
			format=d3ddm.Format;
		}
		if (shift==MaskToShift(0x7c00))
		{
			d3ddm.Format=D3DFMT_A1R5G5B5;
			format=d3ddm.Format;
		}
	}
	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.Windowed = !fullscreen;
	//d3dpp.EnableAutoDepthStencil = true;
	//d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
		D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &pD3DDevice9)))
	{
		
		if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
			D3DDEVTYPE_HAL, hwnd,
			D3DCREATE_MIXED_VERTEXPROCESSING,
			&d3dpp, &pD3DDevice9)))
		{
			
			if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
				D3DDEVTYPE_HAL, hwnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING,
				&d3dpp, &pD3DDevice9)))
			{
				
				pD3DDevice9=NULL;
			}
			else
			{
				
			}
			//pD3DDevice9=NULL;
		}
		else
		{
			
		}
	}
	else
	{
		
	}

	if (!pD3DDevice9)
	{
		d3dpp.BackBufferCount = 1;
		if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
		D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &pD3DDevice9)))
		{
		
		if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
			D3DDEVTYPE_HAL, hwnd,
			D3DCREATE_MIXED_VERTEXPROCESSING,
			&d3dpp, &pD3DDevice9)))
			{
				
				if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
					D3DDEVTYPE_HAL, hwnd,
					D3DCREATE_SOFTWARE_VERTEXPROCESSING,
					&d3dpp, &pD3DDevice9)))
				{
					
					return E_FAIL;
				}
				else
				{
					
				}
			//	return E_FAIL;
			}
			else
			{
				
			}
		}
		else
		{
			
		}
	}//!pD3

	pD3DDevice9->CreateOffscreenPlainSurface(width, height,d3ddm.Format, D3DPOOL_SYSTEMMEM, &surface, NULL);
	if (format==D3DFMT_X8R8G8B8 || format==D3DFMT_A8R8G8B8)
	{
	m_directxformat.bitsPerPixel=32;
	m_directxformat.depth=32;
	m_directxformat.redMask = 0xff0000;
	m_directxformat.redShift=MaskToShift(m_directxformat.redMask);
	m_directxformat.greenMask = 0xff00;
	m_directxformat.greenShift=MaskToShift(m_directxformat.greenMask);
	m_directxformat.blueMask = 0x00ff;
	m_directxformat.blueShift=MaskToShift(m_directxformat.blueMask);
	}
	if (format==D3DFMT_A1R5G5B5 || format==D3DFMT_X1R5G5B5)
	{
	m_directxformat.bitsPerPixel=16;
	m_directxformat.depth=16;
	m_directxformat.redMask = 0x7c00;
	m_directxformat.redShift=MaskToShift(m_directxformat.redMask);
	m_directxformat.greenMask = 0x03e0;
	m_directxformat.greenShift=MaskToShift(m_directxformat.greenMask);
	m_directxformat.blueMask = 0x001f;
	m_directxformat.blueShift=MaskToShift(m_directxformat.blueMask);
	}
	if (format==D3DFMT_R5G6B5)
	{
	m_directxformat.bitsPerPixel=16;
	m_directxformat.depth=16;
	m_directxformat.redMask=0xF800;
	m_directxformat.redShift=MaskToShift(m_directxformat.redMask);
	m_directxformat.greenMask = 0x07e0;
	m_directxformat.greenShift=MaskToShift(m_directxformat.greenMask);
	m_directxformat.blueMask = 0x001f;
	m_directxformat.blueShift=MaskToShift(m_directxformat.blueMask);
	}
	//edMask = 0x7c00; greenMask = 0x03e0; blueMask = 0x001f;
	//PostMessage(hwnd_sendm,WM_SYSCOMMAND,ID_REQUEST_REFRESH,0);
	return S_OK;
}



HRESULT
ViewerDirectxClass::ReInitD3D()
{
	
	if (!D3DLibrary) return E_FAIL;
	//PITCHTEST
	HWND hwnd=parent_hwnd;
	int width=mwidth;
	int height=mheight;
	int bit=mbit;
	int shift=mshift;
	bool fullscreen=false;
	// create the IDirect3D9 object
	pD3D9 = (*d3dCreate)(D3D_SDK_VERSION);

	//pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D9 == NULL)
	{
		
		return E_FAIL;
	}

	
	// get the display mode
	D3DDISPLAYMODE d3ddm;
	pD3D9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	// set the presentation parameters
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferWidth = width;
	d3dpp.BackBufferHeight = height;
	d3dpp.BackBufferCount = 2;
	if (bit==32) 
	{
	format=d3ddm.Format;
	}
	else if (bit==16)
	{
		if (shift==MaskToShift(0xF800))
		{
			d3ddm.Format=D3DFMT_R5G6B5;
			format=d3ddm.Format;
		}
		if (shift==MaskToShift(0x7c00))
		{
			d3ddm.Format=D3DFMT_A1R5G5B5;
			format=d3ddm.Format;
		}
	}
	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.Windowed = !fullscreen;
	//d3dpp.EnableAutoDepthStencil = true;
	//d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
		D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &pD3DDevice9)))
	{
		
		if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
			D3DDEVTYPE_HAL, hwnd,
			D3DCREATE_MIXED_VERTEXPROCESSING,
			&d3dpp, &pD3DDevice9)))
		{
			
			if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
				D3DDEVTYPE_HAL, hwnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING,
				&d3dpp, &pD3DDevice9)))
			{
				
				pD3DDevice9=NULL;
			}
			else
			{
				
			}
			//pD3DDevice9=NULL;
		}
		else
		{
			
		}
	}
	else
	{
		
	}

	if (!pD3DDevice9)
	{
		d3dpp.BackBufferCount = 1;
		if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
		D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &pD3DDevice9)))
		{
		
		if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
			D3DDEVTYPE_HAL, hwnd,
			D3DCREATE_MIXED_VERTEXPROCESSING,
			&d3dpp, &pD3DDevice9)))
			{
				
				if (FAILED(pD3D9->CreateDevice(D3DADAPTER_DEFAULT, 
					D3DDEVTYPE_HAL, hwnd,
					D3DCREATE_SOFTWARE_VERTEXPROCESSING,
					&d3dpp, &pD3DDevice9)))
				{
					
					return E_FAIL;
				}
				else
				{
					
				}
			//	return E_FAIL;
			}
			else
			{
				
			}
		}
		else
		{
			
		}
	}//!pD3

	pD3DDevice9->CreateOffscreenPlainSurface(width, height,d3ddm.Format, D3DPOOL_SYSTEMMEM, &surface, NULL);
	if (format==D3DFMT_X8R8G8B8 || format==D3DFMT_A8R8G8B8)
	{
	m_directxformat.bitsPerPixel=32;
	m_directxformat.depth=32;
	m_directxformat.redMask = 0xff0000;
	m_directxformat.redShift=MaskToShift(m_directxformat.redMask);
	m_directxformat.greenMask = 0xff00;
	m_directxformat.greenShift=MaskToShift(m_directxformat.greenMask);
	m_directxformat.blueMask = 0x00ff;
	m_directxformat.blueShift=MaskToShift(m_directxformat.blueMask);
	}
	if (format==D3DFMT_A1R5G5B5 || format==D3DFMT_X1R5G5B5)
	{
	m_directxformat.bitsPerPixel=16;
	m_directxformat.depth=16;
	m_directxformat.redMask = 0x7c00;
	m_directxformat.redShift=MaskToShift(m_directxformat.redMask);
	m_directxformat.greenMask = 0x03e0;
	m_directxformat.greenShift=MaskToShift(m_directxformat.greenMask);
	m_directxformat.blueMask = 0x001f;
	m_directxformat.blueShift=MaskToShift(m_directxformat.blueMask);
	}
	if (format==D3DFMT_R5G6B5)
	{
	m_directxformat.bitsPerPixel=16;
	m_directxformat.depth=16;
	m_directxformat.redMask=0xF800;
	m_directxformat.redShift=MaskToShift(m_directxformat.redMask);
	m_directxformat.greenMask = 0x07e0;
	m_directxformat.greenShift=MaskToShift(m_directxformat.greenMask);
	m_directxformat.blueMask = 0x001f;
	m_directxformat.blueShift=MaskToShift(m_directxformat.blueMask);
	}
	//edMask = 0x7c00; greenMask = 0x03e0; blueMask = 0x001f;
	//PostMessage(hwnd_sendm,WM_SYSCOMMAND,ID_REQUEST_REFRESH,0);
	return S_OK;
}

bool
ViewerDirectxClass:: valid()
	{
		return pD3DDevice9!=0 && surface!=0;
	}
bool
ViewerDirectxClass:: paintdevice()
	{
		// check if valid

		if (!valid())
			return false;

		// copy surface to back buffer

		LPDIRECT3DSURFACE9 backBuffer;
		HRESULT result=NULL;

		if (SUCCEEDED(pD3DDevice9->BeginScene()))
		{
#ifdef _DEBUG
		char			szText[256];
		sprintf(szText," ++++++++++BeginScene \n");
		OutputDebugString(szText);		
#endif
		pD3DDevice9->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

		if (!backBuffer)
			return false;

		result = pD3DDevice9->UpdateSurface(surface, 0, backBuffer, NULL);

		backBuffer->Release();
		pD3DDevice9->EndScene();
#ifdef _DEBUG
		sprintf(szText," ++++++++++ENdScene \n");
		OutputDebugString(szText);		
#endif
		}


		if (result!= D3D_OK)
			return false;

		// present back buffer to display
#ifdef _DEBUG
		char			szText[256];
		sprintf(szText," ++++++++++Present \n");
		OutputDebugString(szText);		
#endif
		if (pD3DDevice9->Present(NULL, NULL, NULL, NULL)!= D3D_OK)
			return false;
#ifdef _DEBUG
		sprintf(szText," ++++++++++Present2 \n");
		OutputDebugString(szText);		
#endif
		// tell windows that we dont need to repaint anything

		ValidateRect(parent_hwnd, NULL);

		return true;
	}
bool
ViewerDirectxClass:: paint()
{
	if (directxlocked==true)
		Afterupdate();	
	if (devicelost==true) 
		return true;		
	if (!pD3DDevice9) {
		HDC dc = GetDC(parent_hwnd);
		if (dc) {
			HBRUSH brush = CreateSolidBrush(RGB(0,0,0));
			SelectObject(dc, brush);
			RECT rect;
			GetClientRect(parent_hwnd, &rect);
			Rectangle(dc, 0, 0, rect.right, rect.bottom);
			DeleteObject(brush);
			ReleaseDC(parent_hwnd, dc);
		}

		ValidateRect(parent_hwnd, NULL);
		return true;
	}
	else {
		bool result=paintdevice();
		return result;
	}
}

unsigned char *
ViewerDirectxClass:: Preupdate(unsigned char * bits)
{
	int counter2=0;
	while (paintbuzy) {
		Sleep(2);
		counter2++;
		if (counter2>300)
			break;
	}

	if (!valid())
			return NULL;
	if (directxlocked == true) 
		return bits;
#ifdef _DEBUG
	char			szText[256];
				sprintf(szText,"Preupdate 2\n");
				OutputDebugString(szText);
#endif

	int counter=0;
	unsigned char *mybits=Preupdate2(bits);
	while (mybits==NULL) {
		mybits=Preupdate2(bits);
		counter++;
		if (counter>20) {
			Sleep(1500);
			DestroyD3D();
			ReInitD3D();
			break;
		}
	}
	return (mybits);
}

unsigned char *
ViewerDirectxClass:: Preupdate2(unsigned char * bits)
	{
		HRESULT result = pD3DDevice9->TestCooperativeLevel();

		if (result==D3DERR_DEVICELOST)
			{
			Sleep( 250 );
			devicelost=true;
			return NULL;
			}

		if (result==D3DERR_DRIVERINTERNALERROR)
			{
			devicelost=true;
			return NULL;
			}

		if (result==D3DERR_DEVICENOTRESET)
		{             
			// reset device

			if (FAILED(pD3DDevice9->Reset(&d3dpp)))
			{
			devicelost=true;
			return NULL;
			}
		}

		// copy pixels to surface

		if (!surface)
			{
			devicelost=true;
			return NULL;
			}

		D3DLOCKED_RECT lock;

		if (FAILED(surface->LockRect(&lock, NULL, D3DLOCK_DISCARD)))
		{
			devicelost=true;
			return NULL;
		}
		directxlocked=true;
		devicelost=false;
		unsigned char *data = (unsigned char*) lock.pBits;
		pitch=lock.Pitch;
		return data;
}
bool
ViewerDirectxClass:: Afterupdate()
{

#ifdef _DEBUG
	char			szText[256];
				sprintf(szText,"After update1 2\n");
				OutputDebugString(szText);
#endif
	if (devicelost==true) return true;
	if (directxlocked==false) return true;
	if (!surface) return false;
#ifdef _DEBUG
//	char			szText[256];
				sprintf(szText,"After update2 2\n");
				OutputDebugString(szText);
#endif
	    ///copy pixels
		surface->UnlockRect();
		directxlocked=false;
		return true;
}