#include "stdhdrs.h"
#include "vncviewer.h"
#include "clientconnection.h"


void 
ClientConnection::DestroyD3D(void)
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
ClientConnection::InitD3D(HWND hwnd, int width, int height, int bpp,bool fullscreen)
{
	// create the IDirect3D9 object
	pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3D9 == NULL)
		return E_FAIL;
	
	// get the display mode
	D3DDISPLAYMODE d3ddm;
	pD3D9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	// set the presentation parameters
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferWidth = width;
	d3dpp.BackBufferHeight = height;
	d3dpp.BackBufferCount = 2;
	format=d3ddm.Format;
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
		}
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
		}
	}
		}
	pD3DDevice9->CreateOffscreenPlainSurface(width, height,d3ddm.Format, D3DPOOL_SYSTEMMEM, &surface, NULL);
	if (format==D3DFMT_X8R8G8B8 || format==D3DFMT_A8R8G8B8)
	{
	if (bpp!=32) return E_FAIL;

//	m_directxformat.redMask = 0xff0000;
//	m_directxformat.greenMask = 0xff00;
//	m_directxformat.blueMask = 0x00ff;
	}
	if (format==D3DFMT_A1R5G5B5 || format==D3DFMT_X1R5G5B5)
	{
	if (bpp!=16) return E_FAIL;

//	m_directxformat.redMask = 0x7c0000;
//	m_directxformat.greenMask = 0x03e0;
//	m_directxformat.blueMask = 0x001f;
	}
	if (format==D3DFMT_R5G6B5)
	{
	if (bpp!=16) return E_FAIL;
//	m_directxformat.redMask = 0x7c0000;
//	m_directxformat.greenMask = 0x07e0;
//	m_directxformat.blueMask = 0x001f;
	}
	return S_OK;
}
bool
ClientConnection:: valid()
	{
		return pD3DDevice9!=0 && surface!=0;
	}
bool
ClientConnection:: paintdevice()
	{
		// check if valid

		if (!valid())
			return false;

		// copy surface to back buffer

		LPDIRECT3DSURFACE9 backBuffer;

		pD3DDevice9->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

		if (!backBuffer)
			return false;

		HRESULT result = pD3DDevice9->UpdateSurface(surface, 0, backBuffer, NULL);

		backBuffer->Release();

		if (FAILED(result))
			return false;

		// present back buffer to display

		if (FAILED(pD3DDevice9->Present(NULL, NULL, NULL, NULL)))
			return false;

		// tell windows that we dont need to repaint anything

		ValidateRect(m_hwnd, NULL);

		return true;
	}
bool
ClientConnection:: paint()
	{
		if (directxlocked==true) return true;
//		directxlocked=true;
		if (!pD3DDevice9)
		{
			HDC dc = GetDC(m_hwnd);

			if (dc)
			{
				HBRUSH brush = CreateSolidBrush(RGB(0,0,0));
				SelectObject(dc, brush);
				RECT rect;
				GetClientRect(m_hwnd, &rect);
				Rectangle(dc, 0, 0, rect.right, rect.bottom);
				DeleteObject(brush);
				ReleaseDC(m_hwnd, dc);
			}

			ValidateRect(m_hwnd, NULL);
//			directxlocked=false;
			return true;
		}
		else
		{
			bool result=paintdevice();
//			directxlocked=false;
			return result;
		}
	}

unsigned char *
ClientConnection:: Preupdate(unsigned char * bits)
	{
		// check valid
	if (directxlocked==true) return bits;
	directxlocked=true;

		if (!valid())
			return false;

		// handle device loss

		HRESULT result = pD3DDevice9->TestCooperativeLevel();

		if (result==D3DERR_DEVICELOST)
			return false;

		if (result==D3DERR_DRIVERINTERNALERROR)
			return false;

		if (result==D3DERR_DEVICENOTRESET)
		{             
			// reset device

			if (FAILED(pD3DDevice9->Reset(&d3dpp)))
				return false;
		}

		// copy pixels to surface

		if (!surface)
			return false;

		D3DLOCKED_RECT lock;

		if (FAILED(surface->LockRect(&lock, NULL, D3DLOCK_DISCARD)))
			return false;
//		vnclog.Print(0, _T("Lockrect\n"));

		unsigned char *data = (unsigned char*) lock.pBits;
		const int pitch = lock.Pitch;

		return data;
}
bool
ClientConnection:: Afterupdate()
{
	if (directxlocked==false) return true;
	if (!surface) return false;
	    ///copy pixels

		surface->UnlockRect();
//		vnclog.Print(0, _T("UNLockrect %i\n"),directxlocked);

		// paint display
		bool result= paintdevice();
		directxlocked=false;
		return result;
}