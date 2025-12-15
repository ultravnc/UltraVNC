// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


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