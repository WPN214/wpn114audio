#include "external.hpp"

#ifdef WPN114AUDIO_JACK
#include "io_jack.hpp"
#endif

//-------------------------------------------------------------------------------------------------
void
External::register_input(Input* input)
//-------------------------------------------------------------------------------------------------
{
    switch(input->type()) {
    case Input::Type::Audio: m_audio_inputs.push_back(input); break;
    case Input::Type::Midi: m_midi_inputs.push_back(input);
    }
}

//-------------------------------------------------------------------------------------------------
void
External::register_output(Output* output)
//-------------------------------------------------------------------------------------------------
{
    switch(output->type()) {
    case Output::Type::Audio: m_audio_outputs.push_back(output); break;
    case Output::Type::Midi: m_midi_outputs.push_back(output);
    }
}

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
}
