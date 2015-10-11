/*****************************************************************************
 * sapispeechsynthesizer.cpp: Simple text to Speech renderer for Windows
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#include <winsock2.h>
#define poll WSAPoll
#endif
#define __PLUGIN__

#include <stdlib.h>

/* VLC core API headers */
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>
#include <vlc_charset.h>

static int Create (vlc_object_t *);
static void Destroy(vlc_object_t *);
static int RenderText(filter_t *,
                      subpicture_region_t *,
                      subpicture_region_t *,
                      const vlc_fourcc_t *);

vlc_module_begin ()
set_description("Speech synthesis for Windows")
set_category(CAT_VIDEO)
set_subcategory(SUBCAT_VIDEO_SUBPIC)

set_capability("text renderer", 0)
set_callbacks(Create, Destroy)
add_integer("sapi-voice", -1, "Voice", "SAPI voice index", false)
vlc_module_end ()


#include <Windows.h>
#include <sapi.h>
#include <sphelper.h>
#include <string.h>

static int TryEnterMTA(vlc_object_t *obj)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (unlikely(FAILED(hr)))
    {
        msg_Err (obj, "cannot initialize COM (error 0x%lx)", hr);
        return -1;
    }
    return 0;
}
#define TryEnterMTA(o) TryEnterMTA(VLC_OBJECT(o))

static void EnterMTA(void)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (unlikely(FAILED(hr)))
        abort();
}

static void LeaveMTA(void)
{
    CoUninitialize();
}

struct filter_sys_t
{
    ISpVoice* cpVoice;
    char* lastString;
};

static int  Create (vlc_object_t *p_this)
{
    filter_t *p_filter = (filter_t *)p_this;
    filter_sys_t *p_sys;
    HRESULT hr;

    p_filter->p_sys = p_sys = (filter_sys_t*) malloc(sizeof(filter_sys_t));
    if (!p_sys)
        return VLC_ENOMEM;

    if (TryEnterMTA(p_this))
        goto error;

    p_sys->cpVoice = nullptr;
    hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_INPROC_SERVER, IID_ISpVoice, (void**) &p_sys->cpVoice);
    if (SUCCEEDED(hr)) {
        ISpObjectToken*        cpVoiceToken = nullptr;
        IEnumSpObjectTokens*   cpEnum = nullptr;
        ULONG ulCount = 0;

        hr = SpEnumTokens(SPCAT_VOICES, nullptr, nullptr, &cpEnum);
        if (SUCCEEDED(hr)) {
            // Get the number of voices.
            hr = cpEnum->GetCount(&ulCount);
            if (SUCCEEDED (hr))
            {
                int voiceIndex = var_InheritInteger(p_this, "sapi-voice");
                if (voiceIndex > - 1)
                {
                    if (voiceIndex <= ulCount) {
                        hr = cpEnum->Item(voiceIndex, &cpVoiceToken);
                        if (SUCCEEDED(hr)) {
                            hr = p_sys->cpVoice->SetVoice(cpVoiceToken);
                            if (SUCCEEDED(hr)) {
                                msg_Dbg(p_this, "Selected voice %d", voiceIndex);
                            }
                            else {
                                msg_Err(p_this, "Failed to set voice %d", voiceIndex);
                            }
                            cpVoiceToken->Release();
                            cpVoiceToken = nullptr;
                        }
                    }
                    else
                        msg_Err(p_this, "Voice index exceeds available count");
                }
            }
            cpEnum->Release();
            cpEnum = nullptr;
        }

        if (SUCCEEDED(hr)) {
            hr = p_sys->cpVoice->SetOutput(nullptr, TRUE);
        }
    }
    else
    {
        msg_Err(p_filter, "Could not create SpVoice");
    }

    LeaveMTA();

    p_filter->pf_render_text = RenderText;

    return VLC_SUCCESS;

error:
    free(p_sys);
    return VLC_EGENERIC;
}

static void Destroy(vlc_object_t *p_this)
{
    filter_t *p_filter = (filter_t *)p_this;
    filter_sys_t *p_sys = p_filter->p_sys;

    if (p_sys->cpVoice) {
        p_sys->cpVoice->Release();
        p_sys->cpVoice = nullptr;
    }

    if (p_sys->lastString) {
        free(p_sys->lastString);
        p_sys->lastString = nullptr;
    }

    free(p_sys);
}

static int RenderText(filter_t *p_filter,
                      subpicture_region_t *p_region_out,
                      subpicture_region_t *p_region_in,
                      const vlc_fourcc_t *p_chroma_list)
{
    // TODO: update for current VLC API

    filter_sys_t *p_sys = p_filter->p_sys;
    //text_segment_t *p_segment = p_region_in->p_text;

    // if (!p_segment)
    //     return VLC_EGENERIC;
    bool once = true;

    //for ( const text_segment_t *s = p_segment; s != nullptr; s = s->p_next ) {
    for (auto s = p_region_in; once; once = false) {
        if ( !s->psz_text )
            continue;

        if (strlen(s->psz_text) == 0)
            continue;
        try {
            if (p_sys->lastString && !strcmp(p_sys->lastString, s->psz_text))
                continue;

            if (!strcmp(s->psz_text, "\n"))
                continue;

            p_sys->lastString = strdup(s->psz_text);
            if (p_sys->lastString) {
                msg_Dbg(p_filter, "Speaking '%s'", s->psz_text);

                EnterMTA();
                wchar_t* wideText = ToWide(s->psz_text);
                HRESULT hr = p_sys->cpVoice->Speak(wideText, SPF_ASYNC, nullptr);
                free(wideText);
                if (!SUCCEEDED(hr)) {
                    msg_Err(p_filter, "Speak() error");
                }
                LeaveMTA();
            }
        }
        catch (...) {
            msg_Err(p_filter, "Caught an exception!");
        }
    }

    return VLC_SUCCESS;
}