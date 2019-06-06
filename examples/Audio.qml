import QtQuick 2.0
import WPN114.Audio 1.1 as WPN114

Item
{
    WPN114.Graph
    {
        // these are the default graph property values
        rate: 44100; vector: 512

        // this is implicit, as it is its first and only child (T_T)
        target: io

        // when the graph is complete, start audio processing
        Component.onCompleted:
            io.running = true;

        WPN114.External
        {
            id: io

            // the name of the audio device
            // for jack clients etc.
            name: "wpn114audio-test-device"
            backend: WPN114.External.Jack
//            backend: WPN114.External.Alsa
//            backend: WPN114.External.PulseAudio
//            backend: WPN114.External.Core
//            backend: WPN114.External.VSTHost
            // +windows stuff

            inAudioTargets: "system"
            inAudioRouting: [0, 1, 1, 0]

            outAudioTargets: "REAPER"
            outAudioRouting: [0, 1, 1, 0]

            inMidiTargets: "Ableton Push"
            inMidiRouting: [0, 1, 1, 0]

            outMidiTargets: "Ableton Push"
            outMidiRouting: [0, 1, 1, 0]

            // default property values as well
            numAudioInputs:   0
            numAudioOutputs:  2

            numMidiInputs:   1
            numMidiOutputs:  0

            running: false

            // are midi in/outs connections implicit
            // or should we make them explicit?...
            WPN114.MIDITransposer on midiInputs {
                transpose: semitones(-5)
                outputs: sinetest.midiInput
            }

//            midiInputs.onNoteOn: frequency = mtof(index);

            WPN114.Sinetest
            {
                id: sinetest


                frequency: 440.0                                

                // rule is:
                // audio comes first
                // if Node has no audio output
                // routing applies to midi output
                routing: [0, 0, 0, 1]

                WPN114.VCA on audioOutput {
                    id: vca;
                    gain: db(-12)

                    WPN114.Sinetest on gain {
                        frequency: 1
                        mul: 0.25
                        add: 0.5
                    }
                }
            }
        }
    }
}
