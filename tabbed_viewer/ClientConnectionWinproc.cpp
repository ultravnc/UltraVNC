#define _WIN32_WINDOWS 0x0410
#define WINVER 0x0400
#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"
#include "AboutBox.h"
#include <commctrl.h>

#define ID_MDI_FIRSTCHILD 60000

extern char sz_L75[64];
extern char sz_L76[64];
extern char sz_L77[64];
extern char sz_L78[64];
extern char sz_L79[64];
extern char sz_L80[64];
extern char sz_L81[64];
extern char sz_L82[64];
extern char sz_L83[64];
extern char sz_L84[64];
extern char sz_L85[64];
extern char sz_L86[64];
extern char sz_L87[64];
extern char sz_L88[64];

extern int g_iMaxChild;
extern int g_iNumChild;
extern bool fullscreen;
extern CHILD* g_child;
extern HANDLE m_bitmapBACK;
extern HANDLE m_bitmapNONE;

extern HWND m_hwndMain;

extern HWND m_hwndTab;
extern TCHAR g_FileName[MAX_PATH];;
//extern HWND m_Status;
extern bool g_autoscale;

//
//
//
LRESULT CALLBACK ClientConnection::WndProchwnd(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	
	//	HWND parent;
		ClientConnection *_this = (ClientConnection *)
#if defined (_MSC_VER) && _MSC_VER <= 1200		
		GetWindowLong(hwnd, GWL_USERDATA);
#else
		GetWindowLongPtr(hwnd, GWLP_USERDATA);
#endif	
	if (_this == NULL) return DefWindowProc(hwnd, iMsg, wParam, lParam);
	
	switch (iMsg) 
			{
////////////////////////////////////////////////////////////////////////////////////////////////////////
			case WM_CLOSE:
				{
				
					// sf@2002 - Do not close vncviewer if the File Transfer GUI is open !
					if (_this->m_pFileTransfer->m_fFileTransferRunning)
					{
						_this->m_pFileTransfer->ShowFileTransferWindow(true);
						MessageBox(NULL, sz_L85, 
							sz_L88, 
							MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
						return 0;
					}
					
					// sf@2002 - Do not close vncviewer if the Text Chat GUI is open !
					if (_this->m_pTextChat->m_fTextChatRunning)
					{
						_this->m_pTextChat->ShowChatWindow(true);
						MessageBox(NULL, sz_L86, 
							sz_L88, 
							MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
						return 0;
					}

					if (_this->m_fOptionsOpen)
					{
						MessageBox(NULL, sz_L87, 
							sz_L88, 
							MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
						return 0;
					}
					//PostMessage(GetParent(hwnd), WM_MDICASCADE, 0, 0);
					SendMessage(GetParent(hwnd),WM_MDIDESTROY,(WPARAM)hwnd,0L); 
//					DestroyWindow(_this->m_hwnd);
					break;
				}
				
			case WM_DESTROY:
				{
					vnclog.Print(4, _T("DESTOY WINDOW\n"));
					_this->KillThread();
					// sf@2002 - Do not close vncviewer if the File Transfer GUI is open !
					if (_this->m_pFileTransfer->m_fFileTransferRunning)
					{
						_this->m_pFileTransfer->ShowFileTransferWindow(true);
						MessageBox(NULL, sz_L85, 
							sz_L88, 
							MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
						return 0;
					}
					
					// sf@2002 - Do not close vncviewer if the Text Chat GUI is open !
					if (_this->m_pTextChat->m_fTextChatRunning)
					{
						_this->m_pTextChat->ShowChatWindow(true);
						MessageBox(NULL, sz_L86, 
							sz_L88, 
							MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
						return 0;
					}
					if (_this->dormantTimer != 0)
					{
						KillTimer(_this->m_hwnd, _this->dormantTimer);
						_this->dormantTimer=0;
					}
					BOOL res = ChangeClipboardChain( hwnd, _this->m_hwndNextViewer);
					if (_this->m_waitingOnEmulateTimer)
						{
							KillTimer(_this->m_hwnd, _this->m_emulate3ButtonsTimer);
							KillTimer(_this->m_hwnd, 3335);
							_this->m_waitingOnEmulateTimer = false;
						}
					SendMessage(GetParent(hwnd),WM_MDIREFRESHMENU ,0,0);
					DrawMenuBar(m_hwndMain);
					_this->m_hwnd=NULL;
					


					if (_this->m_fOptionsOpen)
					{
						MessageBox(NULL, sz_L87, 
							sz_L88, 
							MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
						return 0;
					}
					// Close the worker thread as well
					g_iNumChild--;
					vnclog.Print(4, _T("Tab %d\n"), g_iNumChild);
					if(g_iNumChild !=0)
            
						{
							int iLoop=0;
							int iThisChild=0;
							for (iLoop = 1; iLoop < (g_iNumChild); iLoop++)
							{
								if (hwnd==g_child[iLoop].hWnd)
									iThisChild=iLoop;
							}
							vnclog.Print(4, _T("Tab2 iThisChild %d\n"), iThisChild);
							if (iThisChild!=0)
							{
                            if(iThisChild != g_iNumChild)
								{
									for (iLoop = iThisChild; iLoop < (g_iNumChild); iLoop++)
										{
											g_child[iLoop] = g_child[iLoop + 1];
											TCITEM tie;
											tie.mask = TCIF_TEXT; 
											tie.iImage = -1; 
											tie.pszText = g_child[iLoop].temptitle; 
											TabCtrl_SetItem(m_hwndTab,iLoop,&tie);
										}
								}						
							}
							vnclog.Print(4, _T("Tab2 iThisChild %d\n"), iThisChild);
							TabCtrl_DeleteItem(m_hwndTab,g_iNumChild);
						}
				vnclog.Print(4, _T("join \n"));	
//				WaitForSingleObject(_this->KillEvent,500);
				vnclog.Print(4, _T("join \n"));
/*					try
						{
						//	Beep(100,100);
							void *p;
							_this->join(&p);  // After joining, _this is no longer valid
						} catch (omni_thread_invalid& e) {
							// The thread probably hasn't been started yet,
						}*/
					}
				vnclog.Print(4, _T("join \n"));
					
					break;
			case WM_NCDESTROY :
				try
						{
						//	Beep(100,100);
							void *p;
							_this->join(&p);  // After joining, _this is no longer valid
						} catch (omni_thread_invalid& e) {
							// The thread probably hasn't been started yet,
						}
				break;

////////////////////////////////////////////////////////////////////////////////////////////////////////
			case WM_CREATE:
				SetTimer(_this->m_hwnd,3335, 1000, NULL);
				return 0;
				
			case WM_REGIONUPDATED:
				//_this->DoBlit();
				_this->SendAppropriateFramebufferUpdateRequest();
				return 0;
				
			case WM_PAINT:
				_this->DoBlit();
				return 0;

			case WM_ERASEBKGND:
				return 0;
				
			case WM_TIMER:
				if (wParam == _this->m_emulate3ButtonsTimer)
				{
					_this->SubProcessPointerEvent( 
						_this->m_emulateButtonPressedX,
						_this->m_emulateButtonPressedY,
						_this->m_emulateKeyFlags);
					KillTimer(_this->m_hwnd, _this->m_emulate3ButtonsTimer);
					_this->m_waitingOnEmulateTimer = false;
				}
				if (wParam == 999)
				{
					_this->SendIncrementalFramebufferUpdateRequest();
				}
				return 0;

			case WM_QUERYOPEN:
					_this->SetDormant(false);
					return true;
////////////////////////////////////////////////////////////////////////////////////////////////////////				
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
				if (_this->m_SWselect) 
				{
					_this->m_SWpoint.x=LOWORD(lParam);
					_this->m_SWpoint.y=HIWORD(lParam);
					_this->SendSW(_this->m_SWpoint.x,_this->m_SWpoint.y);
					return 0;
				}
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MOUSEMOVE:
				{
					if (_this->m_SWselect) {return 0;}
					if (!_this->m_running) return 0;
					if (GetFocus() != _this->m_hwnd) return 0;
					int x = LOWORD(lParam);
					int y = HIWORD(lParam);
					wParam = MAKEWPARAM(LOWORD(wParam), 0);
					if ( _this->m_opts.m_ViewOnly) return 0;
					_this->ProcessPointerEvent(x,y, wParam, iMsg);
					return 0;
				}
				
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
				{
					if (!_this->m_running) return 0;
					if ( _this->m_opts.m_ViewOnly) return 0;
					_this->ProcessKeyEvent((int) wParam, (DWORD) lParam);
					return 0;
				}
				
			case WM_CHAR:
			case WM_SYSCHAR:
			case WM_DEADCHAR:
			case WM_SYSDEADCHAR:
				return 0;


////////////////////////////////////////////////////////////////////////////////////////////////////////
				
			case WM_SETFOCUS:
				TheAccelKeys.SetWindowHandle(_this->m_opts.m_NoHotKeys ? 0 : hwnd);
				if (_this->InFullScreenMode())
					SetWindowPos(hwnd, HWND_TOPMOST, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE);
				return 0;

				// Cacnel modifiers when we lose focus
			case WM_KILLFOCUS:
				{
					
					if (!_this->m_running) return 0;
					if (_this->InFullScreenMode()) {
						// We must top being topmost, but we want to choose our
						// position carefully.
						HWND foreground = GetForegroundWindow();
						HWND hwndafter = NULL;
						if ((foreground == NULL) || 
							(GetWindowLong(foreground, GWL_EXSTYLE) & WS_EX_TOPMOST)) {
							hwndafter = HWND_NOTOPMOST;
						} else {
							hwndafter = GetNextWindow(foreground, GW_HWNDNEXT); 
						}
						
						SetWindowPos(hwnd, hwndafter, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
					}
					if (!_this->m_running) return 0;
					if ( _this->m_opts.m_ViewOnly) return 0;
					_this->SendKeyEvent(XK_Alt_L,     false);
					_this->SendKeyEvent(XK_Control_L, false);
					_this->SendKeyEvent(XK_Shift_L,   false);
					_this->SendKeyEvent(XK_Alt_R,     false);
					_this->SendKeyEvent(XK_Control_R, false);
					_this->SendKeyEvent(XK_Shift_R,   false);
					return 0;
				}
			case WM_SETCURSOR:
				{
					// if we have the focus, let the cursor change as normal
//					if (GetFocus() == hwnd) 
//						break;
					RECT rect;
					POINT point;
					GetClientRect(hwnd,&rect);
					GetCursorPos(&point);
					ScreenToClient(hwnd, &point);
					if (!PtInRect(&rect,point)) break;

					
					HCURSOR h;
					switch (_this->m_opts.m_localCursor) {
					case NOCURSOR:
						h= LoadCursor(_this->m_pApp->m_instance, MAKEINTRESOURCE(IDC_NOCURSOR));
						break;
					case NORMALCURSOR:
						h= LoadCursor(NULL, IDC_ARROW);
						break;
					case DOTCURSOR:
					default:
						h= LoadCursor(_this->m_pApp->m_instance, MAKEINTRESOURCE(IDC_DOTCURSOR));
					}
					if (_this->m_SWselect) h= LoadCursor(_this->m_pApp->m_instance, MAKEINTRESOURCE(IDC_CURSOR1));
					SetCursor(h);
					return 0;
				}

////////////////////////////////////////////////////////////////////////////////////////////////////////

				
				
			case WM_QUERYNEWPALETTE:
				{
					TempDC hDC(hwnd);
					
					// Select and realize hPalette
					PaletteSelector p(hDC, _this->m_hPalette);
					InvalidateRect(hwnd, NULL, FALSE);
					UpdateWindow(hwnd);
					return TRUE;
				}
				
			case WM_PALETTECHANGED:
				// If this application did not change the palette, select
				// and realize this application's palette
				if ((HWND) wParam != hwnd)
				{
					// Need the window's DC for SelectPalette/RealizePalette
					TempDC hDC(hwnd);
					PaletteSelector p(hDC, _this->m_hPalette);
					// When updating the colors for an inactive window,
					// UpdateColors can be called because it is faster than
					// redrawing the client area (even though the results are
					// not as good)
					UpdateColors(hDC);
					
				}
				break;
				
///////////////////////////////////////////////////////////////////////////////////////////////////
			case WM_SIZING:
				{
					if (_this->m_opts.m_fAutoScaling)
					{
					// Don't allow sizing larger than framebuffer
//					Beep(100,100);
					RECT wrect,crect;
					GetWindowRect(hwnd,&wrect);
					GetClientRect(hwnd,&crect);
					int dx=(wrect.right-wrect.left)-(crect.right-crect.left);
					int dy=(wrect.bottom-wrect.top)-(crect.bottom-crect.top);
					RECT *lprc = (LPRECT) lParam;
						switch (wParam) {
						case WMSZ_RIGHT: 
							lprc->bottom = ((lprc->right - lprc->left-dx) * _this->m_si.framebufferHeight  / _this->m_si.framebufferWidth) + lprc->top+dy;
							break;
						case WMSZ_LEFT:
							lprc->bottom = ((lprc->right - lprc->left-dx) * _this->m_si.framebufferHeight  / _this->m_si.framebufferWidth) + lprc->top+dy;
							break;
						case WMSZ_TOPRIGHT:
							lprc->right = ((lprc->bottom - lprc->top-dy) * _this->m_si.framebufferWidth  / _this->m_si.framebufferHeight) + lprc->left+dx;
							break;
						case WMSZ_TOPLEFT:
							lprc->left = (-(lprc->bottom - lprc->top-dy) * _this->m_si.framebufferWidth  / _this->m_si.framebufferHeight) + lprc->right-dx;
							break;

						case WMSZ_BOTTOMRIGHT:
							lprc->bottom = ((lprc->right - lprc->left-dx) * _this->m_si.framebufferHeight  / _this->m_si.framebufferWidth) + lprc->top +dy;
							break;
						case WMSZ_BOTTOMLEFT:
							lprc->bottom = ((lprc->right - lprc->left-dx) * _this->m_si.framebufferHeight  / _this->m_si.framebufferWidth) + lprc->top+dy; 
							break;			
						case WMSZ_TOP:
							lprc->right = ((lprc->bottom - lprc->top-dy) * _this->m_si.framebufferWidth  / _this->m_si.framebufferHeight) + lprc->left+dx ;
							break;

						case WMSZ_BOTTOM:
							lprc->right = ((lprc->bottom - lprc->top-dy) * _this->m_si.framebufferWidth  / _this->m_si.framebufferHeight) + lprc->left+dx;
							break;
						}
					}
					else
					{
						// Don't allow sizing larger than framebuffer
//						Beep(100,10);
						RECT *lprc = (LPRECT) lParam;
						switch (wParam) {
						case WMSZ_RIGHT: 
						case WMSZ_TOPRIGHT:
						case WMSZ_BOTTOMRIGHT:
							lprc->right = min(lprc->right, lprc->left + _this->m_fullwinwidth+1 );
							break;
						case WMSZ_LEFT:
						case WMSZ_TOPLEFT:
						case WMSZ_BOTTOMLEFT:
							lprc->left = max(lprc->left, lprc->right - _this->m_fullwinwidth);
							break;
						}
						
						switch (wParam) {
						case WMSZ_TOP:
						case WMSZ_TOPLEFT:
						case WMSZ_TOPRIGHT:
//							if (_this->m_opts.m_ShowToolbar)
//								lprc->top = max(lprc->top, lprc->bottom - _this->m_fullwinheight);
//							else
								lprc->top = max(lprc->top, lprc->bottom - _this->m_fullwinheight);
							break;
						case WMSZ_BOTTOM:
						case WMSZ_BOTTOMLEFT:
						case WMSZ_BOTTOMRIGHT:
//							if (_this->m_opts.m_ShowToolbar)
//								lprc->bottom = min(lprc->bottom, lprc->top + _this->m_fullwinheight);
//							else
								lprc->bottom = min(lprc->bottom, lprc->top + _this->m_fullwinheight);
							break;
						}
					}

					return 0;
				}
					break;
			case 	WM_SIZE:
						if (wParam==SIZE_RESTORED) 
						if (!fullscreen){
							DWORD style = GetWindowLong(hwnd, GWL_STYLE);
							style |= WS_DLGFRAME | WS_THICKFRAME|WS_SYSMENU|WS_BORDER ;
							SetWindowLong(hwnd, GWL_STYLE, style);
						}
						if (wParam==SIZE_MAXIMIZED)

							if (fullscreen)
						{

								DWORD style = GetWindowLong(hwnd, GWL_STYLE);
								style &= ~(WS_DLGFRAME | WS_THICKFRAME |WS_SYSMENU|WS_BORDER  );
								SetWindowLong(hwnd, GWL_STYLE, style);
								RECT rect;
								GetWindowRect(m_hwndMain,&rect);
						//		SetWindowPos(hwnd, HWND_TOPMOST, 0,0, rect.right-rect.left, rect.bottom-rect.top-28, SWP_NOZORDER);
						}
//						Beep(200,10);
						if (_this->m_opts.m_fAutoScaling) _this->SizeWindowNoSet();
						else
						{
						bool bMaximized;
						SendMessage(GetParent(hwnd), WM_MDIGETACTIVE, 0, (LPARAM)&bMaximized);
						if(bMaximized == TRUE)
						{
							_this->SizeWindow();
							RECT rect;
							GetClientRect(hwnd, &rect);
							if ((rect.right - rect.left)>(_this->m_si.framebufferWidth * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den))
							{
								RECT rectm,rectc;
								int dx=0;
								int dy=0;
								int dxv=0;
								GetWindowRect(m_hwndMain,&rectm);
								GetClientRect(m_hwndMain,&rectc);
								dx=(rectm.right-rectm.left)-(rectc.right-rectc.left);
								dy=(rectm.bottom-rectm.top)-(rectc.bottom-rectc.top);
								if ((GetWindowLong(hwnd, GWL_STYLE) & WS_VSCROLL))
								{
									dxv=20;
								}
								SetWindowPos(m_hwndMain,NULL,0,0,
									_this->m_si.framebufferWidth * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den+dx+dxv,
									rectm.bottom-rectm.top,
									SWP_NOZORDER | SWP_NOMOVE); 
							}
							if ((rect.bottom - rect.top)>(_this->m_si.framebufferHeight * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den))
							{
								RECT rectm,rectc;
								int dx=0;
								int dy=0;
								int dyh=0;
								GetWindowRect(m_hwndMain,&rectm);
								GetClientRect(GetParent(hwnd),&rectc);
								dx=(rectm.right-rectm.left)-(rectc.right-rectc.left);
								dy=(rectm.bottom-rectm.top)-(rectc.bottom-rectc.top);
								if ((GetWindowLong(hwnd, GWL_STYLE) & WS_HSCROLL))
								{
									dyh=20;
								}
								SetWindowPos(m_hwndMain,NULL,0,0,
									rectm.right-rectm.left,
									_this->m_si.framebufferHeight * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den+dy+dyh,
									SWP_NOZORDER | SWP_NOMOVE); 
							}


						}
						
						RECT rect;
						GetClientRect(hwnd, &rect);
						
						_this->m_cliwidth = min( (int)(rect.right - rect.left), 
							                     (int)(_this->m_si.framebufferWidth * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den));
//						if (_this->m_opts.m_ShowToolbar)
//							_this->m_cliheight = min( (int)rect.bottom - rect.top ,
//							                          (int)_this->m_si.framebufferHeight * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den);
//						else
							_this->m_cliheight = min( (int)(rect.bottom - rect.top) ,
							                          (int)(_this->m_si.framebufferHeight * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den));
						
						_this->m_hScrollMax = (int)_this->m_si.framebufferWidth * _this->m_opts.m_scale_num / _this->m_opts.m_scale_den;
//						if (_this->m_opts.m_ShowToolbar)
//							_this->m_vScrollMax = (int)(_this->m_si.framebufferHeight *
//							                            _this->m_opts.m_scale_num / _this->m_opts.m_scale_den)
//														;
//						else 
							_this->m_vScrollMax = (int)(_this->m_si.framebufferHeight*
							                           _this->m_opts.m_scale_num / _this->m_opts.m_scale_den);
						
						int newhpos, newvpos;
						newhpos = max(0, 
							          min(_this->m_hScrollPos, 
					                      _this->m_hScrollMax - max(_this->m_cliwidth, 0)
										 )
									 );
						newvpos = max(0,
							          min(_this->m_vScrollPos, 
							              _this->m_vScrollMax - max(_this->m_cliheight, 0)
										 )
								     );
						
						ScrollWindowEx(_this->m_hwnd,
							           _this->m_hScrollPos - newhpos,
									   _this->m_vScrollPos - newvpos,
							           NULL, &rect, NULL, NULL,  SW_INVALIDATE);
						
						_this->m_hScrollPos = newhpos;
						_this->m_vScrollPos = newvpos;
						_this->UpdateScrollbars();
						}

						break;
			case WM_EXITSIZEMOVE:
//				Beep(1000,10);
				_this->SizeWindow();
				break;

			case WM_HSCROLL:
					{
						int dx = 0;
						int pos = HIWORD(wParam);
						switch (LOWORD(wParam)) {
						case SB_LINEUP:
							dx = -2; break;
						case SB_LINEDOWN:
							dx = 2; break;
						case SB_PAGEUP:
							dx = _this->m_cliwidth * -1/4; break;
						case SB_PAGEDOWN:
							dx = _this->m_cliwidth * 1/4; break;
						case SB_THUMBPOSITION:
							dx = pos - _this->m_hScrollPos;
						case SB_THUMBTRACK:
							dx = pos - _this->m_hScrollPos;
						}
						_this->ScrollScreen(dx,0);
						
						return 0;
					}
					
				case WM_VSCROLL:
					{
						int dy = 0;
						int pos = HIWORD(wParam);
						switch (LOWORD(wParam)) {
						case SB_LINEUP:
							dy = -2; break;
						case SB_LINEDOWN:
							dy = 2; break;
						case SB_PAGEUP:
							dy = _this->m_cliheight * -1/4; break;
						case SB_PAGEDOWN:
							dy = _this->m_cliheight * 1/4; break;
						case SB_THUMBPOSITION:
							dy = pos - _this->m_vScrollPos;
						case SB_THUMBTRACK:
							dy = pos - _this->m_vScrollPos;
						}
						_this->ScrollScreen(0,dy);
						
						return 0;
					}
				// RealVNC 335 method
				case WM_MOUSEWHEEL:
					if (!_this->m_opts.m_ViewOnly)
						_this->ProcessMouseWheel((SHORT)HIWORD(wParam));
					return 0;
///////////////////////////////////////////////////////////////////////////////////////////////////								
			case WM_DRAWCLIPBOARD:
				_this->ProcessLocalClipboardChange();
				return 0;
				
			case WM_CHANGECBCHAIN:
				{
					// The clipboard chain is changing
					HWND hWndRemove = (HWND) wParam;     // handle of window being removed 
					HWND hWndNext = (HWND) lParam;       // handle of next window in chain 
					// If next window is closing, update our pointer.
					if (hWndRemove == _this->m_hwndNextViewer)  
						_this->m_hwndNextViewer = hWndNext;  
					// Otherwise, pass the message to the next link.  
					else if (_this->m_hwndNextViewer != NULL) 
						::SendMessage(_this->m_hwndNextViewer, WM_CHANGECBCHAIN, 
						(WPARAM) hWndRemove,  (LPARAM) hWndNext );  
					return 0;
					
				}
///////////////////////////////////////////////////////////////////////////////////////////////////

			// Modif VNCon MultiView support
			// Messages used by VNCon - Copyright (C) 2001-2003 - Alastair Burr
			case WM_GETSCALING:
				{
					WPARAM wPar;
					wPar = MAKEWPARAM(_this->m_hScrollMax, _this->m_vScrollMax);
					SendMessage((HWND)wParam, WM_GETSCALING, wPar, lParam);
					return TRUE;
					
				}
				
			case WM_SETSCALING:
				{          
					_this->m_opts.m_scaling = true;
					_this->m_opts.m_scale_num = wParam;
					_this->m_opts.m_scale_num_v = wParam;
					_this->m_opts.m_scale_den = lParam;
					if (_this->m_opts.m_scale_num ==  _this->m_opts.m_scale_den )
						_this->m_opts.m_scaling = false;
					_this->SizeWindow();
					InvalidateRect(hwnd, NULL, TRUE);
					return TRUE;
					
				}
///////////////////////////////////////////////////////////////////////////////////////////////////				
			case WM_SETVIEWONLY:
				{
					_this->m_opts.m_ViewOnly = (wParam == 1);
					return TRUE;
				}
			// End Modif for VNCon MultiView support

			case WM_MDIACTIVATE:
				{
				  HMENU hMenu;
					UINT uEnableFlag;

					hMenu = GetMenu(m_hwndMain);
					if(hwnd == (HWND)lParam)                               //being activated
					{
						uEnableFlag = MF_ENABLED;
					}
					else
					{
						uEnableFlag = MF_GRAYED;                           //being de-activated
						_this->sleep=2000;
						if (_this->dormantTimer != 0) KillTimer(_this->m_hwnd, _this->dormantTimer);
						_this->dormantTimer=SetTimer(_this->m_hwnd,999, _this->sleep, NULL);
					}
					EnableMenuItem(hMenu, 1, MF_BYPOSITION | uEnableFlag);
					EnableMenuItem(hMenu, 2, MF_BYPOSITION | uEnableFlag);
					EnableMenuItem(hMenu, 3, MF_BYPOSITION | uEnableFlag);
					EnableMenuItem(hMenu, 4, MF_BYPOSITION | uEnableFlag);

					DrawMenuBar(m_hwndMain);
					InvalidateRect(m_hwndMain,NULL,FALSE);
//					Beep(4000,100);
					int iLoop;
							int iThisChild;
							for (iLoop = 0; iLoop < (g_iNumChild); iLoop++)
							{
								if (hwnd==g_child[iLoop].hWnd)
									iThisChild=iLoop;
							}
					TabCtrl_HighlightItem(m_hwndTab,iThisChild,0);
					if (((HWND)lParam==hwnd) && !IsIconic(hwnd)) 
					{
						SetForegroundWindow(hwnd);
						SetFocus(hwnd);
						TabCtrl_HighlightItem(m_hwndTab,iThisChild,1);
						TabCtrl_SetCurSel(m_hwndTab,iThisChild);
						if (_this->sleep!=0)_this->SendIncrementalFramebufferUpdateRequest();
						_this->sleep=0;
						if (_this->dormantTimer != 0) KillTimer(_this->m_hwnd, _this->dormantTimer);
						_this->dormantTimer = 0;
					}
					if (((HWND)lParam==hwnd) && IsIconic(hwnd)) 
					{
						_this->sleep=250000;
						if (_this->dormantTimer != 0) KillTimer(_this->m_hwnd, _this->dormantTimer);
						_this->dormantTimer=SetTimer(_this->m_hwnd,999, _this->sleep, NULL);
					}
					SendMessage(GetParent(hwnd),WM_MDIREFRESHMENU ,0,0);
					DrawMenuBar(m_hwndMain);


				}
				break;
			case WM_MDITILE:
//				Beep(1000,100);
				break;


			case WM_SYSCOMMAND:
//				Beep(100,100);
				switch(LOWORD(wParam)) 
					{
						case 61458:
							{
//								 Beep(2000,100);
								 SetFocus(hwnd);
								 //SendMessage(m_hwndMain,WM_MDIACTIVATE,(WPARAM)hwnd,NULL);
							}
							break;

						case SC_MAXIMIZE:
							if (fullscreen)
							{
								DWORD style = GetWindowLong(hwnd, GWL_STYLE);
								style &= ~(WS_DLGFRAME | WS_THICKFRAME |WS_SYSMENU|WS_BORDER  );
								SetWindowLong(hwnd, GWL_STYLE, style);
							}
							break;
						case SC_MINIMIZE:
							

							_this->SetDormant(true);
							if (_this->m_hwndStatus)ShowWindow(_this->m_hwndStatus,SW_MINIMIZE);
							break;
							
						case SC_RESTORE:
							_this->SetDormant(false);
							if (_this->m_hwndStatus)ShowWindow(_this->m_hwndStatus,SW_NORMAL);
							break;

						case ID_SW:
							if (_this->m_fServerKnowsFileTransfer)
							{
								if (!_this->m_SWselect)
								{
									_this->m_SWselect=true;
								}
							}
							else
							{
								MessageBox(NULL, "Option not supported", "Error",MB_ICONEXCLAMATION | MB_OK);
							}
							break;
						
						case ID_DESKTOP:
							if (_this->m_fServerKnowsFileTransfer)
							{
								if (!_this->m_SWselect)
								{
									_this->m_SWselect=true;
									_this->SendSW(9999,9999);
								}
							}
							else
							{
								MessageBox(NULL, "Option not supported", "Error",MB_ICONEXCLAMATION | MB_OK);
							}
							break;
						
						// Toggle toolbar & toolbar menu option
						case ID_DBUTTON:
//							_this->m_opts.m_ShowToolbar = !_this->m_opts.m_ShowToolbar;
//							CheckMenuItem(GetSystemMenu(m_hwndMain, FALSE),
//										ID_DBUTTON,
//										MF_BYCOMMAND | (_this->m_opts.m_ShowToolbar ? MF_CHECKED :MF_UNCHECKED));
//							_this->SizeWindow();
	//						_this->SetFullScreenMode(_this->InFullScreenMode());
							break;
						case ID_OUTFULLSCREEN:
							_this->m_opts.m_fAutoScaling=_this->old_autoscale;
							_this->old_autoscale=2;
							break;

						case ID_FULLSCREEN:
							{
							if (_this->old_autoscale==2)_this->old_autoscale=_this->m_opts.m_fAutoScaling;
							if (g_autoscale) _this->m_opts.m_fAutoScaling=true;
							CheckMenuItem(GetSystemMenu(m_hwndMain, FALSE),
										ID_AUTOSCALING,
										MF_BYCOMMAND | (_this->m_opts.m_fAutoScaling ? MF_CHECKED :MF_UNCHECKED));

							InvalidateRect(hwnd, NULL, TRUE);
							RECT rect;
							GetWindowRect(GetParent(hwnd),&rect);
							DWORD style = GetWindowLong(hwnd, GWL_STYLE);
							style &= ~WS_VSCROLL;
							style &= ~WS_HSCROLL;
							SetWindowLong(hwnd, GWL_STYLE, style);
							SetWindowPos(hwnd,NULL,0,0,rect.right-rect.left,rect.bottom-rect.top,SWP_FRAMECHANGED | SWP_NOZORDER); 
							break;
							}
						case ID_AUTOSCALING:
							{
							_this->m_opts.m_fAutoScaling = !_this->m_opts.m_fAutoScaling;
							CheckMenuItem(GetSystemMenu(m_hwndMain, FALSE),
										ID_AUTOSCALING,
										MF_BYCOMMAND | (_this->m_opts.m_fAutoScaling ? MF_CHECKED :MF_UNCHECKED));

							InvalidateRect(hwnd, NULL, TRUE);
							RECT rect;
							GetWindowRect(hwnd,&rect);
							DWORD style = GetWindowLong(hwnd, GWL_STYLE);
							style &= ~WS_VSCROLL;
							style &= ~WS_HSCROLL;
							SetWindowLong(hwnd, GWL_STYLE, style);
							SetWindowPos(hwnd,NULL,0,0,rect.right-rect.left,rect.bottom-rect.top,SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE); 

							}
							break;
	
						case ID_DINPUT:
							_this->m_remote_mouse_disable = true;
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendServerInput(true);
							break;
							
						case ID_INPUT:
							_this->m_remote_mouse_disable = false;
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendServerInput(false);
							break;
							
						case ID_CONN_SAVE_AS:
							_this->SaveConnection();
							return 0;
						
						case ID_CONN_SAVE_ASFAV:
							_this->Save_auto_Connection();
							return 0;

						case ID_CONN_SAVE_ASFAV_ALL:
							_this->Save_auto_Connection_all();
//							SendMessage(hwnd,WM_CLOSE,0,0L);
							return 0;
							
						case IDC_OPTIONBUTTON: 
							{
								if (_this->m_fOptionsOpen) return 0;
								_this->m_fOptionsOpen = true;
								
								// Modif sf@2002 - Server Scaling
								int nOldServerScale = _this->m_nServerScale;
								int prev_scale_num = _this->m_opts.m_scale_num;
								int prev_scale_den = _this->m_opts.m_scale_den;
//								bool fOldToolbarState = _this->m_opts.m_ShowToolbar;
								int nOldAutoMode = _this->m_opts.autoDetect;
								
								if (_this->m_opts.DoDialog(true))
								{
									
									
									// Modif sf@2002 - Server Scaling
									_this->m_nServerScale = _this->m_opts.m_nServerScale;
									if (_this->m_nServerScale != nOldServerScale)
									{
										_this->SendServerScale(_this->m_nServerScale);
									}
									else
									{
										if (prev_scale_num != _this->m_opts.m_scale_num ||
											prev_scale_den != _this->m_opts.m_scale_den)
										{
											// Resize the window if scaling factors were changed
											_this->SizeWindow();
											InvalidateRect(hwnd, NULL, TRUE);
											// Make the window corresponds to the requested state
	//										_this->RealiseFullScreenMode();
										}
//										if (fOldToolbarState != _this->m_opts.m_ShowToolbar)
//											_this->SizeWindow();
										_this->m_pendingFormatChange = true;
									}
								}		
								if (nOldAutoMode != _this->m_opts.autoDetect)
									_this->m_nConfig = 0;
								_this->OldEncodingStatusWindow = -2; // force update in status window
								_this->m_fOptionsOpen = false;
								return 0;
							}
							
						case ID_CONN_ABOUT:
							_this->ShowConnInfo();
							return 0;
	
						case ID_VIEWONLYTOGGLE: 
							// Toggle view only mode
							_this->m_opts.m_ViewOnly = !_this->m_opts.m_ViewOnly;
							// Todo update menu state
							return 0;
							
						case ID_REQUEST_REFRESH: 
							// Request a full-screen update
							_this->SendFullFramebufferUpdateRequest();
							return 0;
		
						case ID_VK_LWINDOWN:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Super_L, true);
							return 0;
						case ID_VK_LWINUP:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Super_L, false);
							return 0;
						case ID_VK_RWINDOWN:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Super_R, true);
							return 0;
						case ID_VK_RWINUP:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Super_R, false);
							return 0;
						case ID_VK_APPSDOWN:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Menu, true);
							return 0;
						case ID_VK_APPSUP:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Menu, false);
							return 0;
	
							
						// Send START Button
						case ID_CONN_CTLESC:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Control_L,true);
							_this->SendKeyEvent(XK_Escape,true);
							_this->SendKeyEvent(XK_Control_L,false);
							_this->SendKeyEvent(XK_Escape,false);
							return 0;
							
						// Send Ctrl-Alt-Del
						case ID_CONN_CTLALTDEL:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Control_L, true);
							_this->SendKeyEvent(XK_Alt_L,     true);
							_this->SendKeyEvent(XK_Delete,    true);
							_this->SendKeyEvent(XK_Delete,    false);
							_this->SendKeyEvent(XK_Alt_L,     false);
							_this->SendKeyEvent(XK_Control_L, false);
							return 0;
							
						case ID_CONN_CTLDOWN:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Control_L, true);
							return 0;
							
						case ID_CONN_CTLUP:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Control_L, false);
							return 0;
							
						case ID_CONN_ALTDOWN:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Alt_L, true);
							return 0;
							
						case ID_CONN_ALTUP:
							if (_this->m_opts.m_ViewOnly) return 0;
							_this->SendKeyEvent(XK_Alt_L, false);
							return 0;
							
							// Modif sf@2002 - FileTransfer
						case ID_FILETRANSFER: 
							if (_this->m_fServerKnowsFileTransfer)
							{
								// Check if the Server knows FileTransfer
								if (!_this->m_fServerKnowsFileTransfer)
								{
									MessageBox(NULL, sz_L77, 
									sz_L78, 
									MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
									return 0;
								}
								// Don't call FileTRansfer GUI is already open !
								if (_this->m_pFileTransfer->m_fFileTransferRunning)
								{
									_this->m_pFileTransfer->ShowFileTransferWindow(true);
									return 0;
									
								}
								if (_this->m_pTextChat->m_fTextChatRunning)
								{
									_this->m_pTextChat->ShowChatWindow(true);
									MessageBox(	NULL,
												sz_L86, 
												sz_L88, 
												MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
									return 0;
								}

								// Call FileTransfer Dialog
								_this->m_pFileTransfer->m_fFileTransferRunning = true;
								_this->m_pFileTransfer->m_fFileCommandPending = false;
								_this->m_pFileTransfer->DoDialog();
								_this->m_pFileTransfer->m_fFileTransferRunning = false;
								// Refresh Screen
								// _this->SendFullFramebufferUpdateRequest();
								if (_this->m_pFileTransfer->m_fVisible || _this->m_pFileTransfer->m_fOldFTProtocole)
									_this->SendAppropriateFramebufferUpdateRequest();
							}
							else
							{
								MessageBox(NULL, "Option not supported", "Error",MB_ICONEXCLAMATION | MB_OK);
							}
							return 0;
						
							// sf@2002 - Text Chat
						case ID_TEXTCHAT:
							if (_this->m_fServerKnowsFileTransfer)
							{
								// We use same flag as FT for now
								// Check if the Server knows FileTransfer
								if (!_this->m_fServerKnowsFileTransfer)
								{
									MessageBox(NULL, sz_L81, 
										sz_L82, 
										MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
									return 0;
								}
								if (_this->m_pTextChat->m_fTextChatRunning)
								{
									_this->m_pTextChat->ShowChatWindow(true);
									return 0;
									
								}
								if (_this->m_pFileTransfer->m_fFileTransferRunning)
								{
									_this->m_pFileTransfer->ShowFileTransferWindow(true);
									MessageBox(NULL,
												sz_L85, 
												sz_L88, 
												MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST);
									return 0;
								}
								_this->m_pTextChat->m_fTextChatRunning = true;
								_this->m_pTextChat->DoDialog();
							}
							else
							{
								MessageBox(NULL, "Option not supported", "Error",MB_ICONEXCLAMATION | MB_OK);
							}
							return 0;
						
							// sf@2002
						case ID_MAXCOLORS: 
							if (_this->m_opts.m_Use8Bit)
							{
								_this->m_opts.m_Use8Bit = rfbPFFullColors; //false;
								_this->m_pendingFormatChange = true;
								InvalidateRect(hwnd, NULL, TRUE);
							}
							return 0;
							
							// sf@2002
						case ID_256COLORS: 
							// if (!_this->m_opts.m_Use8Bit)
							{
								_this->m_opts.m_Use8Bit = rfbPF256Colors; //true;
								_this->m_pendingFormatChange = true;
								InvalidateRect(hwnd, NULL, TRUE);
							}
							return 0;
							
							// Modif sf@2002
						case ID_HALFSCREEN: 
							{
								// Toggle halfSize screen mode (server side)
								int nOldServerScale = _this->m_nServerScale;
								
								// Modif sf@2002 - Server Scaling
								_this->m_opts.m_fAutoScaling = false;
								_this->m_nServerScale = 2;
								_this->m_opts.m_nServerScale = 2;
								_this->m_opts.m_scaling = true;
								_this->m_opts.m_scale_num = 100;
								_this->m_opts.m_scale_num_v = 100;
								_this->m_opts.m_scale_den = 100;
								
								if (_this->m_nServerScale != nOldServerScale)
								{
									_this->SendServerScale(_this->m_nServerScale);
									// _this->m_pendingFormatChange = true;
								}
								else
								{
									_this->SizeWindow();
									InvalidateRect(hwnd, NULL, TRUE);
	//								_this->RealiseFullScreenMode();	
									_this->m_pendingFormatChange = true;
								}
								return 0;
							}
							
							// Modif sf@2002
						case ID_FUZZYSCREEN: 
							{
								// Toggle fuzzy screen mode (server side)
								int nOldServerScale = _this->m_nServerScale;
								
								// We don't forbid AutoScaling if selected
								// so the viewer zoom factor is more accurate
								_this->m_nServerScale = 2;
								_this->m_opts.m_nServerScale = 2;
								_this->m_opts.m_scaling = true;
								_this->m_opts.m_scale_num = 200;
								_this->m_opts.m_scale_num_v = 200;
								_this->m_opts.m_scale_den = 100;
								
								if (_this->m_nServerScale != nOldServerScale)
								{
									_this->SendServerScale(_this->m_nServerScale);
									// _this->m_pendingFormatChange = true;
								}
								else
								{
									_this->SizeWindow();
									InvalidateRect(hwnd, NULL, TRUE);
	//								_this->RealiseFullScreenMode();	
									_this->m_pendingFormatChange = true;
								}
								
								return 0;
							}
							
							// Modif sf@2002
						case ID_NORMALSCREEN: 
							{
								// Toggle normal screen
								int nOldServerScale = _this->m_nServerScale;
								
								_this->m_opts.m_fAutoScaling = false;
								_this->m_nServerScale = 1;
								_this->m_opts.m_nServerScale = 1;
								_this->m_opts.m_scaling = false;
								_this->m_opts.m_scale_num = 100;
								_this->m_opts.m_scale_num_v = 100;
								_this->m_opts.m_scale_den = 100;
								
								if (_this->m_nServerScale != nOldServerScale)
								{
									_this->SendServerScale(_this->m_nServerScale);
									// _this->m_pendingFormatChange = true;
								}
								else
								{
									_this->SizeWindow();
									InvalidateRect(hwnd, NULL, TRUE);
	//								_this->SetFullScreenMode(false);	
									_this->m_pendingFormatChange = true;
								}
								DWORD style = GetWindowLong(hwnd, GWL_STYLE);
								style |= WS_VSCROLL | WS_HSCROLL;
								SetWindowLong(hwnd, GWL_STYLE, style);
								SetWindowPos(hwnd,NULL,0,0,0,0,SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE); 

								return 0;
							}



						case ID_BUTTON_INFO:
								{
									if (IsWindow(_this->m_hwndStatus)){
										if (_this->m_hwndStatus)SetForegroundWindow(_this->m_hwndStatus);
										if (_this->m_hwndStatus)ShowWindow(_this->m_hwndStatus, SW_NORMAL);
									}else{
										SECURITY_ATTRIBUTES   lpSec;
										DWORD				  threadID;
										_this->m_statusThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE )ClientConnection::GTGBS_ShowStatusWindow,(LPVOID)_this,0,&threadID);
									}
									return 0;
								}

						case 9998:
								{
									vnclog.Print(0,_T("CLICKK %d\n"),HIWORD(wParam));
									switch (HIWORD(wParam)) {
										case 0:
												{
												int port;
												TCHAR fulldisplay[256];
												TCHAR display[256];
												GetDlgItemText(hwnd, 9999, display, 256);
												_tcscpy(fulldisplay, display);
												vnclog.Print(0,_T("CLICKK %s\n"),fulldisplay);
												ParseDisplay(fulldisplay, display, 256, &port);
												_this->m_pApp->NewConnection(display,port);
												}
										}
									break;
									return TRUE;
								}

				
						case ID_BUTTON_SEP:
								{
									UINT Key;
									//_this->SendKeyEvent(XK_Execute,     true);
									//_this->SendKeyEvent(XK_Execute,     false);
									Key = DialogBox(_this->m_pApp->m_instance,MAKEINTRESOURCE(IDD_CUSTUM_KEY),NULL,(DLGPROC)ClientConnection::GTGBS_SendCustomKey_proc);
									if (Key>0){
										vnclog.Print(0,_T("START Send Custom Key %d\n"),Key);
										if ( (Key & KEYMAP_LALT) == KEYMAP_LALT){
											_this->SendKeyEvent(XK_Alt_L,true);
											_this->SendKeyEvent(Key ^ KEYMAP_LALT,true);
											_this->SendKeyEvent(Key ^ KEYMAP_LALT,false);
											_this->SendKeyEvent(XK_Alt_L,false);
										}else if ( (Key & KEYMAP_RALT) ==KEYMAP_RALT){
											_this->SendKeyEvent(XK_Alt_R,true);
											_this->SendKeyEvent(XK_Control_R,true);
											_this->SendKeyEvent(Key ^ KEYMAP_RALT,true);
											_this->SendKeyEvent(Key ^ KEYMAP_RALT,false);
											_this->SendKeyEvent(XK_Alt_R,false);
											_this->SendKeyEvent(XK_Control_R,false);
											
										}else if ( (Key &  KEYMAP_RCONTROL) == KEYMAP_RCONTROL){
											_this->SendKeyEvent(XK_Control_R,true);
											_this->SendKeyEvent(Key ^ KEYMAP_RCONTROL,true);
											_this->SendKeyEvent(Key ^ KEYMAP_RCONTROL,false);
											_this->SendKeyEvent(XK_Control_R,false);
										}else{
											_this->SendKeyEvent(Key,true);
											_this->SendKeyEvent(Key,false);
										}
										
										
										vnclog.Print(0,_T("END   Send Custom Key %d\n"),Key);
									}
									SetForegroundWindow(_this->m_hwnd);
									
									return 0;
								}

						case ID_BUTTON_END:
								{
									SendMessage(hwnd,WM_CLOSE,(WPARAM)0,(LPARAM)0);
									return 0;
								}
				
						case ID_BUTTON_DINPUT:
								{
									if (_this->m_fServerKnowsFileTransfer)
									{
										if (_this->m_remote_mouse_disable)
										{
											_this->m_remote_mouse_disable=false;
											SendMessage(hwnd,WM_SYSCOMMAND,(WPARAM)ID_INPUT,(LPARAM)0);
											SendMessage(hwnd,WM_SIZE,(WPARAM)ID_DINPUT,(LPARAM)0);
										}
										else
										{
											_this->m_remote_mouse_disable=true;
											SendMessage(hwnd,WM_SYSCOMMAND,(WPARAM)ID_DINPUT,(LPARAM)0);
											SendMessage(hwnd,WM_SIZE,(WPARAM)ID_DINPUT,(LPARAM)0);
										}
									}
									else
									{
										MessageBox(NULL, "Option not supported", "Error",MB_ICONEXCLAMATION | MB_OK);
									}
									return 0;
								}
						case ID_NEWCONN:
							_this->m_pApp->NewConnection();
							break;
						case IDC_GLOBALOPTIONBUTTON:
							_this->m_pApp->NewConnection(true,true,true);
							break;
						case ID_NEWCONNF:
							_this->m_pApp->NewConnection(g_FileName);
							break;
////////////////////////////////////////////////////////////////////////////////////////////////////////			


				}//end switch (iMsg) 
			}//end syscommand
			if ((iMsg == FileTransferSendPacketMessage) && (_this->m_pFileTransfer != NULL))
			{
				if (LOWORD(wParam) == 0)
				{
//					if (_this->m_FTtimer != 0)_this->m_FTtimer=SetTimer(hwnd,11, 100, 0);//
					_this->m_pFileTransfer->SendFileChunk();
				}
				else
					_this->m_pFileTransfer->ProcessFileTransferMsg();
				return 0;
			}
			
			return DefMDIChildProc(hwnd,  iMsg, wParam, lParam);
}