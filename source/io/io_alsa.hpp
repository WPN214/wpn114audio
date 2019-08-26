#pragma once

#include <alsa/asoundlib.h>
#include <source/io/external.hpp>

//=================================================================================================
class AlsaExternal : public ExternalBase
//=================================================================================================
{
    snd_pcm_format_t
    m_format;

    snd_pcm_t*
    m_handle;

    snd_pcm_hw_params_t*
    m_hwparams;

    snd_pcm_sw_params_t*
    m_swparams;

    snd_pcm_channel_area_t*
    m_areas;

    QString
    m_device = "plughw:0,0";

public:

    //---------------------------------------------------------------------------------------------
    AlsaExternal()
    //---------------------------------------------------------------------------------------------
    {
        int err = 0;
        m_format = SND_PCM_FORMAT_FLOAT;
        snd_pcm_hw_params_malloc(&m_hwparams);
        snd_pcm_sw_params_malloc(&m_swparams);

        err = snd_pcm_open(&m_handle, CSTR(m_device), SND_PCM_STREAM_PLAYBACK, 0);
        // set hwparams
        // set swparams

        snd_pcm_hw_params_any(m_handle, m_hwparams);
        snd_pcm_hw_params_set_rate_resample(m_handle, m_hwparams, 0);

        // set interleaved read/write format
        // set sample format
        // set channel count
        // set stream rate
        // set buffer time
        // set period time
        // write parameters to device


    }

    //---------------------------------------------------------------------------------------------
    virtual
    ~AlsaExternal() override
    //---------------------------------------------------------------------------------------------
    {
        snd_pcm_hw_free(m_handle);
        snd_pcm_hw_params_free(m_hwparams);
        snd_pcm_sw_params_free(m_swparams);
    }

    //---------------------------------------------------------------------------------------------
    virtual void
    run() override
    //---------------------------------------------------------------------------------------------
    {

    }

    //---------------------------------------------------------------------------------------------
    virtual void
    stop() override
    //---------------------------------------------------------------------------------------------
    {

    }
};
