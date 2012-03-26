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

/*
 * Simple soundboard application that will work with whatever .wav files
 * are placed in the data folder.
 */
#include "ExamplesMain.h"
#include <stdio.h>
#include <dirent.h>
#include <malloc.h>
#include <memory.h>

#include "s3eSound.h"
#include "s3eSoundPool.h"

#include "IwGx.h"

// The header for the output wave file
struct RiffHeader
{
    char m_ChunkID[4];
    int32 m_ChunkSize;
    char m_RIFFType[4];

    RiffHeader()
    {
        memcpy(m_ChunkID, "RIFF", 4);
        memcpy(m_RIFFType, "WAVE", 4);
    }
};

struct Chunk
{
    char m_ChunkID[4];
    uint32 m_ChunkSize;
};

struct FormatChunk
{
    Chunk  m_Chunk;
    uint16 m_CompressionCode;
    uint16 m_NumberOfChannels;
    uint32 m_SampleRate;
    uint32 m_BytesPerSecond;
    uint16 m_BlockAlign;
    uint16 m_SignificantBits;
};

static bool g_UseSoundPool = true;

#define MAX_SAMPLES 9

static const char* g_Buttons[MAX_SAMPLES];
static int16* g_SampleData[MAX_SAMPLES];
static int g_SampleDataLen[MAX_SAMPLES];
static int g_Samples[MAX_SAMPLES];
static int g_SampleState[MAX_SAMPLES];

int16* LoadWav(const char* filename, int* sizeOut)
{
    RiffHeader header;
    Chunk chunk;

    int16* rtn = NULL;
    FILE* f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    s3eDebugTracePrintf("filesize = %d", size);
    fseek(f, 0, SEEK_SET);
    fread(&header, 1, sizeof(header), f);
    s3eDebugTracePrintf("%d %.4s %.4s %u", sizeof(header), header.m_ChunkID, header.m_RIFFType, header.m_ChunkSize);

    while (1)
    {
        fread(&chunk, 1, sizeof(chunk), f);
        if (!strncmp(chunk.m_ChunkID, "data", 4))
        {
            rtn = (int16*)malloc(chunk.m_ChunkSize);
            fread(rtn, 1, chunk.m_ChunkSize, f);
            *sizeOut = chunk.m_ChunkSize;
            break;
        }
        fseek(f, chunk.m_ChunkSize, SEEK_CUR);
    }

    fclose(f);
    return rtn;
}

int32 SampleEnded(s3eSoundPoolEndSampleInfo* pInfo, void* userData)
{
    s3eDebugTracePrintf("sample ended = %d", pInfo->m_SampleId);

    g_SampleState[pInfo->m_SampleId] = 0;

    return 1;
}

int32 ChannelEnded(s3eSoundEndSampleInfo* pInfo, void* userData)
{
    s3eDebugTracePrintf("channel ended = %d", pInfo->m_Channel);
    
    g_SampleState[pInfo->m_Channel] = 0;
    
    return 1;
}

void RegisterCallbacks()
{
    if (g_UseSoundPool)
    {
        s3eSoundPoolRegister(S3E_SOUNDPOOL_STOP_AUDIO, (s3eCallback)SampleEnded, 0);
    }
    else
    {
        for (int i=0; i<MAX_SAMPLES; ++i)
        {
            s3eSoundChannelRegister(i, S3E_CHANNEL_STOP_AUDIO, (s3eCallback)ChannelEnded, 0);
        }
    }
}

void Load(int i, const char* pPath)
{
    if (g_UseSoundPool)
        g_Samples[i] = s3eSoundPoolSampleLoad(pPath);
    else
        g_SampleData[i] = LoadWav(pPath, &g_SampleDataLen[i]);
}

s3eResult Play(int i, int repeat)
{
    if (g_UseSoundPool)
        return s3eSoundPoolSamplePlay(g_Samples[i], repeat, 0);
    else
        return s3eSoundChannelPlay(i, g_SampleData[i], g_SampleDataLen[i]/2, repeat, 0);
}

s3eResult Pause(int i)
{
    if (g_UseSoundPool)
        return s3eSoundPoolSamplePause(g_Samples[i]);
    else
        return s3eSoundChannelPause(i);
}

s3eResult Resume(int i)
{
    if (g_UseSoundPool)
        return s3eSoundPoolSampleResume(g_Samples[i]);
    else
        return s3eSoundChannelResume(i);
}

void ExampleInit()
{
    g_UseSoundPool = s3eSoundPoolAvailable() == S3E_TRUE;
    
    // Read in sound data
    // s3eSoundSetInt(S3E_SOUND_DEFAULT_FREQ, 8000);
    DIR* d = opendir(".");
    int count = 0;
    struct dirent* ent;
    while ((ent = readdir(d)))
    {
        int len = strlen(ent->d_name);
        if (len < 4 || stricmp(ent->d_name+len-4, ".wav"))
            continue;

        Load(count, ent->d_name);
        s3eDebugTracePrintf("loaded sound %d (%d)", g_Samples[count], g_SampleDataLen[count]);

        ent->d_name[len-4] = '\0';
        g_Buttons[count] = strdup(ent->d_name);
        AddButton(g_Buttons[count], 20, 20 + 70 * count, 300, 50, (s3eKey)(s3eKey1 + count));
        if (++count == MAX_SAMPLES)
            break;
    }

    RegisterCallbacks();
}

void ExampleShutDown()
{
    for (int i=0; i<MAX_SAMPLES; ++i)
    {
        free(g_SampleData[i]);
        g_SampleData[i] = NULL;
    }
}

bool ExampleUpdate()
{
    for (int i = 0; i < MAX_SAMPLES; i++)
    {
        if (!g_Buttons[i])
            break;
        if (CheckButton(g_Buttons[i]) & S3E_KEY_STATE_RELEASED)
        {
            s3eDebugTracePrintf("pressed button %d", i);
            
            if (i % 2)
            {
                if (!Play(i, 1))
                    g_SampleState[i] = 1;
            }
            else
            {
                switch(g_SampleState[i])
                {
                case 0:
                    if (!Play(i, 3))
                        g_SampleState[i] = 1;
                    break;
                case 1:
                    if (!Pause(i))
                        g_SampleState[i] = 2;                        
                    break;
                case 2:
                    if (!Resume(i))
                        g_SampleState[i] = 1;                        
                    break;
                }
            }
        }
    }

    return true;
}

void ExampleRender()
{
    int y = 150;
    IwGxPrintString(30, y, g_UseSoundPool ? "Using Sound Pool" : "Using Sound Streaming");
    y += 20;

    for (int i = 0; i < MAX_SAMPLES; i++)
    {
        if (!g_Buttons[i])
            break;

        char buffer[0x100];
        const char* pState;
        switch (g_SampleState[i])
        {
        case 1:
            pState = "Playing";
            break;
        case 2:
            pState = "Paused";
            break;
        default:
            pState = "Stopped";
            break;
        }
        sprintf(buffer, "Sample: %d State: %s", i, pState);

        IwGxPrintString(30, y, buffer);
        y += 20;
    }

    IwGxFlush();
    IwGxSwapBuffers();
}
