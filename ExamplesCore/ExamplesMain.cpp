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
#include "IwGx.h"
#include "IwGxPrint.h"
#include "IwTexture.h"
#include "IwMaterial.h"

#include "ExamplesMain.h"

// Globlal for buttons link list
ExButtons*      g_ButtonsHead = NULL;
ExButtons*      g_ButtonsTail = NULL;
CursorKeyCodes  g_Cursorkey = EXCURSOR_NONE;

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
    CIwSVec2* pCoords = IW_GX_ALLOC(CIwSVec2, 4);
    pCoords[0].x = 0; pCoords[0].y = 0;
    pCoords[1].x = 0; pCoords[1].y = (int16)IwGxGetScreenHeight();
    pCoords[2].x = (int16)IwGxGetScreenWidth(); pCoords[2].y = 0;
    pCoords[3].x = (int16)IwGxGetScreenWidth(); pCoords[3].y = (int16)IwGxGetScreenHeight();

    return pCoords;
}

void RenderSoftkey(const char* text, s3eDeviceSoftKeyPosition pos, void(*handler)())
{
    int width = 7;
    int height = 30;
    width *= strlen(text) * 2;
    int x = 0;
    int y = 0;
    switch (pos)
    {
        case S3E_DEVICE_SOFTKEY_BOTTOM_LEFT:
            y = IwGxGetScreenHeight() - height;
            x = 0;
            break;
        case S3E_DEVICE_SOFTKEY_BOTTOM_RIGHT:
            y = IwGxGetScreenHeight() - height;
            x = IwGxGetScreenWidth() - width;
            break;
        case S3E_DEVICE_SOFTKEY_TOP_RIGHT:
            y = 0;
            x = IwGxGetScreenWidth() - width;
            break;
        case S3E_DEVICE_SOFTKEY_TOP_LEFT:
            x = 0;
            y = 0;
            break;
    }

    CIwMaterial *fadeMat = IW_GX_ALLOC_MATERIAL();
    fadeMat->SetAlphaMode(CIwMaterial::SUB);
    IwGxSetMaterial(fadeMat);

    IwGxPrintString(x + 10, y+10, text, false);

    CIwColour* cols = IW_GX_ALLOC(CIwColour, 4);
    memset(cols, 50, sizeof(CIwColour)*4);

    if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_PRESSED)
    {
        int pointerx = s3ePointerGetX();
        int pointery = s3ePointerGetY();
        if (pointerx >= x && pointerx <= x+width && pointery >=y && pointery <= y+height)
        {
            memset(cols, 15, sizeof(CIwColour)*4);
            handler();
        }
    }

    // Draw button area
    CIwSVec2 XY(x, y-2), dXY(width, height);
    IwGxDrawRectScreenSpace(&XY, &dXY, cols);
}

void RenderSoftkeys()
{
    int back = s3eDeviceGetInt(S3E_DEVICE_BACK_SOFTKEY_POSITION);
    RenderSoftkey("Exit", (s3eDeviceSoftKeyPosition)back, s3eDeviceRequestQuit);
    //int advance = s3eDeviceGetInt(S3E_DEVICE_ADVANCE_SOFTKEY_POSITION);
    //RenderSoftkey("ASK", (s3eDeviceSoftKeyPosition)advance);
}


extern "C" int AddButton(const char* text, int x, int y, int w, int h, s3eKey key, exbutton_handler handler)
{
    ExButtons* newbutton = new ExButtons;

    strncpy(newbutton->name, text, 63);
    newbutton->x = x;
    newbutton->y = y;
    newbutton->w = w;
    newbutton->h = h;
    newbutton->key = key;
    newbutton->key_state = 0;
    newbutton->handler = handler;
    newbutton->next = NULL;

    ExButtons* pbutton = g_ButtonsHead;

    if(g_ButtonsHead)
    {
        while(pbutton->next != NULL)
        {
            pbutton = pbutton->next;
        }
        pbutton->next = newbutton;
    }
    else
    {
        g_ButtonsHead = newbutton;
    }

    return 1;
}

extern "C" int32 CheckButton(const char* text)
{
    ExButtons* pbutton = g_ButtonsHead;

    if(g_ButtonsHead)
    {
        pbutton = g_ButtonsHead;
        while(pbutton != NULL)
        {
            if(strcmp(text, pbutton->name) == 0)
            {
                return pbutton->key_state;
            }
            pbutton = pbutton->next;
        }
    }

    return 0;
}


extern "C" void RenderButtons()
{
    ExButtons* pbutton = g_ButtonsHead;

    if(g_ButtonsHead)
    {
        pbutton = g_ButtonsHead;
        while(pbutton != NULL)
        {
            // Check the key and pointer states.
            pbutton->key_state = s3eKeyboardGetState(pbutton->key);
            if( s3eKeyboardGetState(pbutton->key) & S3E_KEY_STATE_DOWN )
            {
                if(pbutton->handler)
                    pbutton->handler();
            }

            if (!(s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_UP))
            {
                int pointerx = s3ePointerGetX();
                int pointery = s3ePointerGetY();
                if (pointerx >= pbutton->x && pointerx <= pbutton->x+pbutton->w && pointery >=pbutton->y && pointery <= pbutton->y+pbutton->h)
                {
                    if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_DOWN)
                    {
                        pbutton->key_state = S3E_KEY_STATE_DOWN;
                    }
                    if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_PRESSED)
                    {
                        pbutton->key_state = S3E_KEY_STATE_PRESSED;
                    }
                    if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED)
                    {
                        pbutton->key_state = S3E_KEY_STATE_RELEASED;
                    }

                    if(pbutton->handler)
                        pbutton->handler();
                }

            }

            // Draw the text
            IwGxSetScreenSpaceSlot(0);

            if(s3ePointerGetInt(S3E_POINTER_AVAILABLE))
            {
                CIwMaterial *fadeMat = IW_GX_ALLOC_MATERIAL();
                fadeMat->SetAlphaMode(CIwMaterial::SUB);
                IwGxSetMaterial(fadeMat);

                CIwColour* cols = IW_GX_ALLOC(CIwColour, 4);
                if(pbutton->key_state == S3E_KEY_STATE_DOWN)
                    memset(cols, 15, sizeof(CIwColour)*4);
                else
                    memset(cols, 50, sizeof(CIwColour)*4);

                // Draw button area
                CIwSVec2 XY(pbutton->x, pbutton->y-2), dXY(pbutton->w, pbutton->h);
                IwGxDrawRectScreenSpace(&XY, &dXY, cols);
            }

            IwGxPrintString(pbutton->x + 2, pbutton->y + ((pbutton->h - 10)/2), pbutton->name, false);
            pbutton = pbutton->next;
        }
    }
}

extern "C" void DeleteButtons()
{
    ExButtons* pbutton = g_ButtonsHead;
    ExButtons* pbuttonnext = NULL;

    while(pbutton != NULL)
    {
        pbuttonnext = pbutton->next;
        delete pbutton;
        pbutton = pbuttonnext;
        pbuttonnext = NULL;
    }
    g_ButtonsHead = NULL;
}

extern "C" void RemoveButton(const char* text)
{
    ExButtons* button = g_ButtonsHead;
    ExButtons* prevbutton = NULL; 
    while(button != NULL)
    {
       if(strcmp(text, button->name) == 0)
       {
           // break list
           if (prevbutton != NULL)
               prevbutton->next = button->next;
           else 
               g_ButtonsHead = button->next;
           delete button;
           return;
       }
       prevbutton = button;
       button = button->next;
    }
}

extern "C" void RenderCursorskeys()
{
    int height = 20;
    int width = 45;

    int lefty = IwGxGetScreenHeight() - (height * 2);
    int leftx = (IwGxGetScreenWidth() - 220) / 2;
    int upy = IwGxGetScreenHeight() - (height * 3);
    int upx = leftx+width + (width/2);
    int downy = IwGxGetScreenHeight() - height;
    int downx = upx;
    int righty = IwGxGetScreenHeight() - (height * 2);
    int rightx = downx + width + (width/2);

    CIwMaterial *fadeMat = IW_GX_ALLOC_MATERIAL();
    fadeMat->SetAlphaMode(CIwMaterial::SUB);
    IwGxSetMaterial(fadeMat);

    g_Cursorkey = EXCURSOR_NONE;

    if ( (s3eKeyboardGetState(s3eKeyLeft) & S3E_KEY_STATE_DOWN) )
        g_Cursorkey = EXCURSOR_LEFT;
    if ( (s3eKeyboardGetState(s3eKeyRight) & S3E_KEY_STATE_DOWN) )
        g_Cursorkey = EXCURSOR_RIGHT;
    if ( (s3eKeyboardGetState(s3eKeyUp) & S3E_KEY_STATE_DOWN) )
        g_Cursorkey = EXCURSOR_UP;
    if ( (s3eKeyboardGetState(s3eKeyDown) & S3E_KEY_STATE_DOWN) )
        g_Cursorkey = EXCURSOR_DOWN;

    if(s3ePointerGetInt(S3E_POINTER_AVAILABLE))
    {
        if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_DOWN)
        {
            int pointerx = s3ePointerGetX();
            int pointery = s3ePointerGetY();
            // Check left
            if (pointerx >= leftx && pointerx <= leftx+width && pointery >=lefty && pointery <= lefty+height)
                g_Cursorkey = EXCURSOR_LEFT;
            // Check right
            if (pointerx >= rightx && pointerx <= rightx+width && pointery >=righty && pointery <= righty+height)
                g_Cursorkey = EXCURSOR_RIGHT;
            // Check up
            if (pointerx >= upx && pointerx <= upx+width && pointery >=upy && pointery <= upy+height)
                g_Cursorkey = EXCURSOR_UP;
            // Check down
            if (pointerx >= downx && pointerx <= downx+width && pointery >=downy && pointery <= downy+height)
                g_Cursorkey = EXCURSOR_DOWN;
        }

        CIwColour* cols = IW_GX_ALLOC(CIwColour, 4);
        if((s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_DOWN) && (g_Cursorkey != EXCURSOR_NONE))
            memset(cols, 10, sizeof(CIwColour)*4);
        else
            memset(cols, 50, sizeof(CIwColour)*4);

        // draw black rect covering screen
        CIwSVec2 rectdim(width, height);

        CIwSVec2 uXY(upx, upy-2);
        IwGxDrawRectScreenSpace(&uXY, &rectdim, cols);
        IwGxPrintString(upx + 10, upy + 5, "Up", false);

        CIwSVec2 dXY(downx, downy-2);
        IwGxDrawRectScreenSpace(&dXY, &rectdim, cols);
        IwGxPrintString(downx + 10, downy + 5, "Down", false);

        CIwSVec2 lXY(leftx, lefty-2);
        IwGxDrawRectScreenSpace(&lXY, &rectdim, cols);
        IwGxPrintString(leftx + 10, lefty + 5, "Left", false);

        CIwSVec2 rXY(rightx, righty-2);
        IwGxDrawRectScreenSpace(&rXY, &rectdim, cols);
        IwGxPrintString(rightx + 10, righty + 5, "Right", false);
    }
}

extern "C" CursorKeyCodes CheckCursorState()
{
    return g_Cursorkey;
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

    //IwGx can be initialised in a number of different configurations to help the linker eliminate unused code.
    //Normally, using IwGxInit() is sufficient.
    //To only include some configurations, see the documentation for IwGxInit_Base(), IwGxInit_GLRender() etc.
    IwGxInit();

    // Example main loop
    ExampleInit();

    // Set screen clear colour
    IwGxSetColClear(0xff, 0xff, 0xff, 0xff);
    IwGxPrintSetColour(128, 128, 128);
    
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
        IwGxClear(IW_GX_COLOUR_BUFFER_F | IW_GX_DEPTH_BUFFER_F);
        RenderButtons();
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
    DeleteButtons();
    IwGxTerminate();
    return 0;
}
