#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
	#include <windows.h>
	#include <GL/gl.h>
#else
#if defined(__APPLE__) || defined(MACOSX)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include "verse.h"
#include "persuade.h"
#include "p_task.h"

void *(*p_gl_GetProcAddress)(const char* proc) = NULL;

void p_extension_init(void *(*gl_GetProcAddress)(const char* proc))
{
	const unsigned char *extension;
	p_gl_GetProcAddress = gl_GetProcAddress;
	extension = glGetString(GL_EXTENSIONS);
	printf("%s\n", extension);
}

void *p_extension_get_address(const char* proc)
{
	if(p_gl_GetProcAddress == NULL)
		return NULL;
	else
		return p_gl_GetProcAddress(proc);
}

boolean p_extension_test(const char *string)
{
	const unsigned char *extension, *a;
	uint i;

	extension = glGetString(GL_EXTENSIONS);
	if(extension != NULL)
	{
		for(a = extension; a[0] != 0; a++)
		{
			for(i = 0; string[i] != 0 && a[i] != 0 && string[i] == a[i]; i++);
			if(string[i] == 0)
				return TRUE;
		}
	}
	return FALSE;
}
