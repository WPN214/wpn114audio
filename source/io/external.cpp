#include "external.hpp"

#ifdef __linux__
#include "io_alsa.hpp"
#endif

#ifdef WPN114AUDIO_JACK
#include "io_jack.hpp"
#endif

#ifdef __ANDROID__
#include "io_qt.hpp"
#endif

//-------------------------------------------------------------------------------------------------
void
External::componentComplete()
//-------------------------------------------------------------------------------------------------
{
#ifdef WPN114AUDIO_JACK
    if (m_backend_id == Backend::Jack) {
        m_backend = new JackExternal(this);
    }
#endif
#ifdef __ANDROID__
    if (m_backend_id == Backend::Qt ||
        m_backend_id == Backend::Default)
        m_backend = new QtAudioBackend(*this);
#endif
#ifdef __APPLE__
    if (m_backend_id == Backend::CoreAudio ||
        m_backend_id == Backend::Default)
        m_backend = new CoreAudioBackend(*this);
#endif
}
