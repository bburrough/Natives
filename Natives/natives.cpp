
#define BLIT_IT_NATIVES_DLL_COMPILE

#include "threadutils.h"


#include "natives.h"

#include <list>
#include <string>
#include <vector>

#include <Windows.h>
#include <roapi.h>

using namespace std;



struct ChooseFileContext
{
    ChooseFileSuccessCallback _chooseFileSuccessCallback;
    ChooseFileCancelledCallback _chooseFileCancelledCallback;
    pthread_t file_picker_thread;
    list<wstring> filetype_names;
    list<wstring> filetype_extensions;
    wchar_t* chosenFilename;
    bool saveAs;
};


void __stdcall SetChooseFileModeSaveAs(void* context)
{
    ChooseFileContext* cfc = (ChooseFileContext*)context;
    cfc->saveAs = true;
}


void __stdcall SetChooseFileModeOpen(void* context)
{
    ChooseFileContext* cfc = (ChooseFileContext*)context;
    cfc->saveAs = false;
}


void* FilePickerThread(void* arg)
{    
    ChooseFileContext* cfc = (ChooseFileContext*)arg;

    const int buf_len = 261;
    cfc->chosenFilename = new wchar_t[buf_len];
    memset(cfc->chosenFilename, 0, buf_len);

    OPENFILENAME ofn;       // common dialog box structure

    //string test("G-code\0*.gcode\0All\0*.*\0");
    wstring filetype_filters;
    list<wstring>::const_iterator ext_itr = cfc->filetype_extensions.begin();
    for (list<wstring>::const_iterator name_itr = cfc->filetype_names.begin(); name_itr != cfc->filetype_names.end(); ++name_itr)
    {
        filetype_filters += *name_itr;
        filetype_filters.push_back('\0');
        filetype_filters += L"*.";
        filetype_filters += *ext_itr;
        filetype_filters.push_back('\0');
        ++ext_itr;
    }
    filetype_filters += L"All";
    filetype_filters.push_back('\0');
    filetype_filters += L"*.*";
    filetype_filters.push_back('\0');
    filetype_filters.push_back('\0');

    //TCHAR filter[64] = TEXT(((const char*)test.c_str()));

    TCHAR* filter = (TCHAR*)filetype_filters.data();

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    //ofn.hwndOwner = hwnd;
    ofn.lpstrFile = cfc->chosenFilename;
    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
    // use the contents of szFile to initialize itself.
    ofn.nMaxFile = buf_len - 1; // use -1 because we're paranoid that Windows will stupidly not include a null terminator
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_READONLY;

    //SetCurrentDirectory(cwd);
    BOOL status;
    if (cfc->saveAs)
        status = GetSaveFileName(&ofn);
    else
        status = GetOpenFileName(&ofn);

    if (status)
    {
        pthread_testcancel();
        if (cfc->_chooseFileSuccessCallback)
        {
            cfc->_chooseFileSuccessCallback(cfc->chosenFilename);
        }
    }
    else
    {
        pthread_testcancel();
        if (cfc->_chooseFileCancelledCallback)
            cfc->_chooseFileCancelledCallback();
    }

    return NULL;
}


void* __stdcall CreateChooseFileContext()
{
    ChooseFileContext* cfc = new ChooseFileContext();
    cfc->_chooseFileCancelledCallback = NULL;
    cfc->_chooseFileSuccessCallback = NULL;
    cfc->chosenFilename = NULL;
    cfc->saveAs = false;
    return cfc;
}


void __stdcall AddChooseFileType(void* context, const wchar_t* name, const wchar_t* extension)
{
    ChooseFileContext* cfc = (ChooseFileContext*)context;
    cfc->filetype_names.push_back(wstring(name));
    cfc->filetype_extensions.push_back(wstring(extension));
}


void __stdcall ChooseFile(void* context, ChooseFileSuccessCallback successCallback, ChooseFileCancelledCallback cancelledCallback)
{
    ChooseFileContext* cfc = (ChooseFileContext*)context;
    cfc->_chooseFileSuccessCallback = successCallback;
    cfc->_chooseFileCancelledCallback = cancelledCallback;
    pthread_create(&cfc->file_picker_thread, NULL, FilePickerThread, cfc);
}


void __stdcall DestroyChooseFileContext(void* context)
{
    ChooseFileContext* cfc = (ChooseFileContext*)context;
    pthread_cancel(cfc->file_picker_thread);
    pthread_join(cfc->file_picker_thread, NULL);
    if (cfc->chosenFilename != NULL)
        delete[] cfc->chosenFilename;
    delete cfc;
}


/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2003-2019 Bobby G. Burrough
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
