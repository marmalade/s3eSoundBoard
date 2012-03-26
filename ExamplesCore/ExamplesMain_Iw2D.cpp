/*
 * This file is part of the Marmalade SDK Code Samples.
 *
 * Copyright (C) 2001-2011 Ideaworks3D Ltd.
 * All Rights Reserved.
 *
 * This source code is intended only as a supplement to Ideaworks Labs
 * Development Tools and/or on-line documentation.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 */
// Examples main file
//-----------------------------------------------------------------------------

#include "s3e.h"
#include "IwDebug.h"
#include "Iw2D.h"

#include "ExamplesMain.h"

// Externs for functions which examples must implement
void ExampleInit();
void ExampleShutDown();
void ExampleRender();
bool ExampleUpdate();

// Attempt to lock to 25 frames per second
#define MS_PER_FRAME (1000 / 25)

// Helper function to display message for Debug-Only Examples
void DisplayMessage(const char* strmessage)
{
    uint16* screen = (uint16*)s3eSurfacePtr();
    int32 width     = s3eSurfaceGetInt(S3E_SURFACE_WIDTH);
    int32 height    = s3eSurfaceGetInt(S3E_SURFACE_HEIGHT);
    int32 pitch     = s3eSurfaceGetInt(S3E_SURFACE_PITCH);
    for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++)
        screen[y * pitch/2 + x] = 0;
    s3eDebugPrint(0, 10, strmessage, 1);
    s3eSurfaceShow();
    while (!s3eDeviceCheckQuitRequest() && !s3eKeyboardAnyKey())
    {
        s3eDeviceYield(0);
        s3eKeyboardUpdate();
    }
}

CIwSVec2* AllocClientScreenRectangle()
{
    return NULL;
}

static void RenderSoftkey(const char* text, s3eDeviceSoftKeyPosition pos, void(*handler)())
{
    int width = 7;
    int height = 10;
    width *= strlen(text);
    int x = 0;
    int y = 0;
    switch (pos)
    {
        case S3E_DEVICE_SOFTKEY_BOTTOM_LEFT:
            y = Iw2DGetSurfaceHeight() - height;
            x = 0;
            break;
        case S3E_DEVICE_SOFTKEY_BOTTOM_RIGHT:
            y = Iw2DGetSurfaceHeight() - height;
            x = Iw2DGetSurfaceWidth() - width;
            break;
        case S3E_DEVICE_SOFTKEY_TOP_RIGHT:
            y = 0;
            x = Iw2DGetSurfaceWidth() - width;
            break;
        case S3E_DEVICE_SOFTKEY_TOP_LEFT:
            x = 0;
            y = 0;
            break;
    }
    char buffer[256] = "`x808080";
    strcat(buffer, text);
    s3eDebugPrint(x, y, buffer, false);
    if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_PRESSED)
    {
        int pointerx = s3ePointerGetX();
        int pointery = s3ePointerGetY();
        if (pointerx >= x && pointerx <= x+width && pointery >=y && pointery <= y+height)
            handler();
    }
}

void RenderSoftkeys()
{
    int back = s3eDeviceGetInt(S3E_DEVICE_BACK_SOFTKEY_POSITION);
    RenderSoftkey("Exit", (s3eDeviceSoftKeyPosition)back, s3eDeviceRequestQuit);
}

//-----------------------------------------------------------------------------
// Main global function
//-----------------------------------------------------------------------------
int main()
{
#ifdef EXAMPLE_DEBUG_ONLY
    // Test for Debug only examples
#ifndef IW_DEBUG
    DisplayMessage("This example is designed to run from a Debug build. Please build the example in Debug mode and run it again.");
    return 0;
#endif
#endif

    Iw2DInit();

    // Example main loop
    ExampleInit();
    // Set screen clear colour

    while (1)
    {
        s3eDeviceYield(0);
        s3eKeyboardUpdate();
        s3ePointerUpdate();

        int64 start = s3eTimerGetMs();

        bool result = ExampleUpdate();
        if  (
            (result == false) ||
            (s3eKeyboardGetState(s3eKeyEsc) & S3E_KEY_STATE_DOWN) ||
            (s3eKeyboardGetState(s3eKeyAbsBSK) & S3E_KEY_STATE_DOWN) ||
            (s3eDeviceCheckQuitRequest())
            )
            break;

        // Clear the screen
        Iw2DSurfaceClear(0xffffffff);
        RenderSoftkeys();
        ExampleRender();

        // Attempt frame rate
        while ((s3eTimerGetMs() - start) < MS_PER_FRAME)
        {
            int32 yield = (int32) (MS_PER_FRAME - (s3eTimerGetMs() - start));
            if (yield<0)
                break;
            s3eDeviceYield(yield);
        }
    }
    ExampleShutDown();
    Iw2DTerminate();
    return 0;
}
