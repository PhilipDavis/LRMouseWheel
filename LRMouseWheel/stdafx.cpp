//
// Copyright 2010 (c) Philip Davis
//

// stdafx.cpp : source file that includes just the standard includes
// LRMouseWheel.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include <stdio.h>

//
// Helper method to write a log message to the debug console
//
void log( LPCWSTR wszFormat, ... )
{	
	va_list args;
	va_start( args, wszFormat );

	WCHAR wszBuffer[ 256 ];
	_vsnwprintf_s( wszBuffer, _TRUNCATE, wszFormat, args );

	va_end( args );

	OutputDebugString( wszBuffer );
}