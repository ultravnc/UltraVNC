/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://ultravnc.sourceforge.net/
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(DISPL)
#define DISPL
typedef HRESULT (CALLBACK *P_DwmIsCompositionEnabled) (BOOL *pfEnabled); 
typedef HRESULT (CALLBACK *P_DwmEnableComposition) (BOOL   fEnable); 

class VNC_OSVersion
{
public:
	static VNC_OSVersion* getInstance() {

		return (!vnc_OSVersion) ?
			vnc_OSVersion = new VNC_OSVersion :
			vnc_OSVersion;
	}
	static void releaseInstance() {
		if (vnc_OSVersion)
			delete vnc_OSVersion;
	}

	VNC_OSVersion();
	virtual ~VNC_OSVersion();
	void SetAeroState();
	bool CaptureAlphaBlending();
	void DisableAero(VOID);
	void ResetAero(VOID);
	bool OS_WIN10;
	bool OS_WIN10_TRANS;
	bool OS_WIN8;
	bool OS_WIN7;
	bool OS_VISTA;
	bool OS_XP;
	bool OS_W2K;
	bool OS_AERO_ON;
	bool OS_LAYER_ON;
	bool OS_NOTSUPPORTED;
	bool OS_BEFOREVISTA;
	bool OS_MINIMUMVISTA;
	bool AeroWasEnabled;
	bool isWINPE(VOID);
	void removeAlpha();
	bool OS_WINPE;
protected:	
	HMODULE DMdll; 
	void UnloadDM(VOID);
	bool LoadDM(VOID);
	P_DwmIsCompositionEnabled pfnDwmIsCompositionEnabled;
	P_DwmEnableComposition pfnDwmEnableComposition; 
	static VNC_OSVersion* vnc_OSVersion;
};
#endif