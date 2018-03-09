// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
//#include <stdlib.h>
#include <malloc.h>
//#include <memory.h>
//#include <tchar.h>
#include <Commdlg.h>
#include <stdio.h>

#ifdef _DEBUG
#define ASSERT_DBG(cond) (void)( !! (cond) || (throw #cond, 0) )
#else
#define ASSERT_DBG(cond) ((void)0)
#endif
