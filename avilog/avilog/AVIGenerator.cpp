// AVIGenerator.cpp: implementation of the CAVIGenerator class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AVIGenerator.h"

/*#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif*/

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CAVIGenerator::CAVIGenerator()
:  m_dwRate(30),
m_pAVIFile(NULL), m_pStream(NULL), m_pStreamCompressed(NULL)
{
	memset(&m_bih,0,sizeof(BITMAPINFOHEADER));
}

#ifdef _AVIGENERATOR_USE_MFC
CAVIGenerator::CAVIGenerator(LPCTSTR sFileName, CView* pView, DWORD dwRate)
: m_sFile(sFileName), m_dwRate(dwRate),
m_pAVIFile(NULL), m_pStream(NULL), m_pStreamCompressed(NULL)
{
		MakeExtAvi();
		SetBitmapHeader(pView);
}
#endif

CAVIGenerator::CAVIGenerator(LPCTSTR sFileName,LPCTSTR sPath, LPBITMAPINFOHEADER lpbih, DWORD dwRate)
: m_dwRate(dwRate),
m_pAVIFile(NULL), m_pStream(NULL), m_pStreamCompressed(NULL)
{
		strcpy(m_sFile,sPath);
		strcpy(mypath,sPath);
		strcat(mypath,"\\");
		strcat(mypath,"codec.cfg");
		strcat(m_sFile,"\\");
		strcat(m_sFile,sFileName);
		MakeExtAvi();
		SetBitmapHeader(lpbih);
}

CAVIGenerator::~CAVIGenerator()
{
	// Just checking that all allocated ressources have been released
	/*ASSERT(m_pStream==NULL);
	ASSERT(m_pStreamCompressed==NULL);
	ASSERT(m_pAVIFile==NULL);*/
	if (tempbuffer) delete []tempbuffer;
	tempbuffer=NULL;
}

void CAVIGenerator::SetBitmapHeader(LPBITMAPINFOHEADER lpbih)
{
	// checking that bitmap size are multiple of 4
	/*ASSERT(lpbih->biWidth%4==0);
	ASSERT(lpbih->biHeight%4==0);*/

	// copying bitmap info structure.
	// corrected thanks to Lori Gardi
	memcpy(&m_bih,lpbih, sizeof(BITMAPINFOHEADER));
	//m_bih.biHeight=-m_bih.biHeight;
	m_bih.biCompression=BI_RGB;
	tempbuffer= new unsigned char[m_bih.biSizeImage];
	m_bih.biSizeImage=0;
}

#ifdef _AVIGENERATOR_USE_MFC
void CAVIGenerator::SetBitmapHeader(CView *pView)
{
	ASSERT_VALID(pView);

	////////////////////////////////////////////////
	// Getting screen dimensions
	// Get client geometry 
	CRect rect; 
	pView->GetClientRect(&rect); 
	CSize size(rect.Width(),rect.Height()); 

	/////////////////////////////////////////////////
	// changing size of image so dimension are multiple of 4
	size.cx=(size.cx/4)*4;
	size.cy=(size.cy/4)*4;

	// initialize m_bih
	memset(&m_bih,0, sizeof(BITMAPINFOHEADER));
	// filling bitmap info structure.
	m_bih.biSize=sizeof(BITMAPINFOHEADER);
	m_bih.biWidth=size.cx;
	m_bih.biHeight=size.cy;
	m_bih.biPlanes=1;
	m_bih.biBitCount=24;
	m_bih.biSizeImage=((m_bih.biWidth*m_bih.biBitCount+31)/32 * 4)*m_bih.biHeight; 
	m_bih.biCompression=BI_RGB;		//BI_RGB means BRG in reality
}
#endif

HRESULT CAVIGenerator::InitEngine()
{
	AVISTREAMINFO strHdr; // information for a single stream 
	AVICOMPRESSOPTIONS opts;
	AVICOMPRESSOPTIONS FAR * aopts[1] = {&opts};

	TCHAR szBuffer[1024]="";
	HRESULT hr;

	strcpy(m_sError,"Ok");

	// Step 0 : Let's make sure we are running on 1.1 
	DWORD wVer = HIWORD(VideoForWindowsVersion());
	if (wVer < 0x010a)
	{
		 // oops, we are too old, blow out of here 
		strcpy(m_sError,"Version of Video for Windows too old. Come on, join the 21th century!");
		return S_FALSE;
	}

	// Step 1 : initialize AVI engine
	AVIFileInit();

	// Step 2 : Open the movie file for writing....
	hr = AVIFileOpen(&m_pAVIFile,			// Address to contain the new file interface pointer
		       (LPCSTR)m_sFile,				// Null-terminated string containing the name of the file to open
		       OF_WRITE | OF_CREATE,	    // Access mode to use when opening the file. 
		       NULL);						// use handler determined from file extension.
											// Name your file .avi -> very important

	if (hr != AVIERR_OK)
	{
		_tprintf(szBuffer,_T("AVI Engine failed to initialize. Check filename %s."),m_sFile);
		strcpy(m_sError,szBuffer);
		// Check it succeded.
		switch(hr)
		{
		case AVIERR_BADFORMAT: 
			strcat(m_sError,"The file couldn't be read, indicating a corrupt file or an unrecognized format.");
			break;
		case AVIERR_MEMORY:		
			strcat(m_sError,"The file could not be opened because of insufficient memory."); 
			break;
		case AVIERR_FILEREAD:
			strcat(m_sError,"A disk error occurred while reading the file."); 
			break;
		case AVIERR_FILEOPEN:		
			strcat(m_sError,"A disk error occurred while opening the file.");
			break;
		case REGDB_E_CLASSNOTREG:		
			strcat(m_sError,"According to the registry, the type of file specified in AVIFileOpen does not have a handler to process it");
			break;
		}

		return hr;
	}

	// Fill in the header for the video stream....
	memset(&strHdr, 0, sizeof(strHdr));
	strHdr.fccType                = streamtypeVIDEO;	// video stream type
	strHdr.fccHandler             = 0;
	strHdr.dwScale                = 1;					// should be one for video
	strHdr.dwRate                 = m_dwRate;		    // fps
	strHdr.dwSuggestedBufferSize  = m_bih.biSizeImage;	// Recommended buffer size, in bytes, for the stream.
	SetRect(&strHdr.rcFrame, 0, 0,		    // rectangle for stream
	    (int) m_bih.biWidth,
	    (int) m_bih.biHeight);

	// Step 3 : Create the stream;
	hr = AVIFileCreateStream(m_pAVIFile,		    // file pointer
			         &m_pStream,		    // returned stream pointer
			         &strHdr);	    // stream header

	// Check it succeded.
	if (hr != AVIERR_OK)
	{
		strcpy(m_sError,"AVI Stream creation failed. Check Bitmap info.");
		if (hr==AVIERR_READONLY)
		{
			strcat(m_sError," Read only file.");
		}
		return hr;
	}


	// Step 4: Get codec and infos about codec
	memset(&opts, 0, sizeof(opts));
	// Poping codec dialog
	bool delete_needed=false;
	{
	FILE *file;
	int size;
	/* Read options from file */
      file = fopen(mypath, "rb");
      if (file == NULL) {
        delete_needed=true;
      }
	  else
	  {
      /* read AVICOMPRESSOPTIONS struct */
      size = fread(&opts, sizeof(AVICOMPRESSOPTIONS), 1, file);
      /* read AVICOMPRESSOPTIONS.cbFormat */
      size = fread(&opts.cbFormat, 4, 1, file);
      /* read AVICOMPRESSOPTIONS.lpFormat */
      opts.lpFormat = (void *) malloc(opts.cbFormat);
      size = fread(opts.lpFormat, opts.cbFormat, 1, file);
      /* read AVICOMPRESSOPTIONS.cbParms */
      size = fread(&opts.cbParms, 4, 1, file);
      /* read AVICOMPRESSOPTIONS.lpParms */
      opts.lpParms = (void *) malloc(opts.cbParms);
      size = fread(opts.lpParms, opts.cbParms, 1, file);
      fclose(file);
	  delete_needed=false;
	  }
	}

	if(delete_needed)
	{
		delete_needed=true;
		if (!AVISaveOptions(NULL, 0, 1, &m_pStream, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
		{
			AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &aopts);
			return S_FALSE;
		}
	}

	// Step 5:  Create a compressed stream using codec options.
	hr = AVIMakeCompressedStream(&m_pStreamCompressed, 
				m_pStream, 
				&opts, 
				NULL);

	if (hr != AVIERR_OK)
	{
		strcpy(m_sError,"AVI Compressed Stream creation failed.");
		
		switch(hr)
		{
		case AVIERR_NOCOMPRESSOR:
			strcat(m_sError," A suitable compressor cannot be found.");
				break;
		case AVIERR_MEMORY:
			strcat(m_sError," There is not enough memory to complete the operation.");
				break; 
		case AVIERR_UNSUPPORTED:
			strcat(m_sError,"Compression is not supported for this type of data. This error might be returned if you try to compress data that is not audio or video.");
			break;
		}

		return hr;
	}

	// Step 6 : sets the format of a stream at the specified position
	hr = AVIStreamSetFormat(m_pStreamCompressed, 
					0,			// position
					&m_bih,	    // stream format
					m_bih.biSize +   // format size
					m_bih.biClrUsed * sizeof(RGBQUAD));

	if (hr != AVIERR_OK)
	{
		strcpy(m_sError,"AVI Compressed Stream format setting failed.");
		// releasing memory allocated by AVISaveOptionFree
		if (delete_needed)AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &aopts);
		return hr;
	}

	// Step 6 : Initialize step counter
	m_lFrame=0;
	if (delete_needed)
	{
		{
			FILE *file;
			int size;
			/* Save options to file*/
			file = fopen(mypath, "wb");
			if (file != NULL) {
				/* write AVICOMPRESSOPTIONS struct */
				size = fwrite(&opts, sizeof(AVICOMPRESSOPTIONS), 1, file);
				/* write AVICOMPRESSOPTIONS.cbFormat */
				size = fwrite(&opts.cbFormat, 4, 1, file);
				/* write AVICOMPRESSOPTIONS.lpFormat */
				size = fwrite(opts.lpFormat, opts.cbFormat, 1, file);
				/* write AVICOMPRESSOPTIONS.cbParms */
				size = fwrite(&opts.cbParms, 4, 1, file);
				/* write AVICOMPRESSOPTIONS.lpParms */
				size = fwrite(opts.lpParms, opts.cbParms, 1, file);
				fclose(file);
				}
		}
		// releasing memory allocated by AVISaveOptionFree
		AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &aopts);
	}
	else
	{
		free(opts.lpFormat);
		opts.lpFormat = NULL;
		free(opts.lpParms);
		opts.lpParms = NULL;

	}
	return hr;
}

void CAVIGenerator::ReleaseEngine()
{
	if (m_pStream)
	{
		AVIStreamRelease(m_pStream);
		m_pStream=NULL;
	}

	if (m_pStreamCompressed)
	{
		AVIStreamRelease(m_pStreamCompressed);
		m_pStreamCompressed=NULL;
	}

	if (m_pAVIFile)
	{
		AVIFileRelease(m_pAVIFile);
		m_pAVIFile=NULL;
	}

	// Close engine
	AVIFileExit();
	if (tempbuffer) delete []tempbuffer;
	tempbuffer=NULL;
}
DWORD oldtime=0;
HRESULT CAVIGenerator::AddFrame(BYTE *bmBits)
{
	DWORD newtime=timeGetTime();
	if ((newtime-oldtime)<(1000/m_dwRate)) return 0;
	oldtime=newtime;
	HRESULT hr;
	int width=m_bih.biWidth*m_bih.biBitCount/8;
	unsigned char *temp;
	temp=tempbuffer;//+(abs(m_bih.biHeight)-1)*(m_bih.biWidth)*m_bih.biBitCount/8;
	for (int i=0;i<(abs(m_bih.biHeight));i++)
	{
		memcpy(temp,bmBits,width);
		temp+=width;
		bmBits+=width;
	}

	// compress bitmap
	hr = AVIStreamWrite(m_pStreamCompressed,	// stream pointer
		m_lFrame,						// time of this frame
		1,						// number to write
		tempbuffer,					// image buffer
		m_bih.biSizeImage,		// size of this frame
		AVIIF_KEYFRAME,			// flags....
		NULL,
		NULL);

	// updating frame counter
	m_lFrame++;

	return hr;
}

void CAVIGenerator::MakeExtAvi()
{
	
	// finding avi
	if( _tcsstr(m_sFile,"avi")==NULL )
	{
		strcat(m_sFile,".avi");
	}
}
