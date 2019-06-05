import QtQuick 2.0
import WPN114.Audio 1.1 as WPN114

Item
{

    WPN114.OSCQueryServer
    {
        id: oscquery_server
        name: "wpn114server"
        port: 5678
    }

    WPN114.Graph
    {
        // these are the default graph property values
        rate: 44100; vector: 512

        // this is implicit, as it is its first and only child (T_T)
        target: io

        // when the graph is complete, start audio processing
        Component.onCompleted: output.run();

        WPN114.External
        {
            id: io

            // the name of the audio device
            // for jack clients etc.
            name: "wpn114audio"

            backend: WPN114.External.Jack
            backend: WPN114.External.Alsa
            backend: WPN114.External.PulseAudio
            backend: WPN114.External.Core
            backend: WPN114.External.VSTHost
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
            numInputs:   0
            numOutputs:  2

            numMidiInputs:   1
            numMidiOutputs:  0

            running: false

            // are midi in/outs connections implicit
            // or should we make them explicit?...
            midiIn: sinetest.midiIn
            midiIn.onNoteOn: frequency = mtof(index);

            WPN114.Sinetest
            {
                id: sinetest

                // frequency socket has different input connections:
                // - o GUI one (directly from QML)
                // - a MIDI one
                // - a OSC one

                frequency: 440.0                                
//                frequency.min = 20;
//                frequency.max = 22050;
//                frequency.range = [20, 22050];
//                frequency.values = [220, 440, 880];
//                frequency.access = WPN114.Access.ReadWrite
//                frequency.clipmode = WPN114.Clipmode.Both

                WPN114.NetNode on frequency
                {
                    device: oscquery_server
                    // not required if the server is in singleDevice mode

                    path: "/sinetest/frequency"
                    interpolation: WPN114.Exponential

                }

                audioOut.routing:
                    [0, 0, 0, 1]

                WPN114.VCA on output
                {
                    id: vca; gain: db(-12);
                    WPN114.Sinetest on gainmod { frequency: 1; level: 0.5 }
                }
            }
        }
    }
}
