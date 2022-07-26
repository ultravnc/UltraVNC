#include <stdio.h>
#include <windows.h>

#if !defined( _RingBuffer )
#define _RingBuffer

class RingBuffer
{
public:
	 RingBuffer();
     void RingBufferSet( int sizeBytes );
     ~RingBuffer();
     int Read( char* dataPtr, int numBytes);
     int Write( char *dataPtr, int numBytes );
     bool Empty( void );
	 void NewSize(unsigned int size);
     int GetSize( );
     int GetWriteAvail( );
     int GetReadAvail( );
	 HANDLE inputevent;
private:
     char * _data;
     int _size;
     int _readPtr;
     int _writePtr;
     int _writeBytesAvail;
	 CRITICAL_SECTION CriticalSection;
};

#endif