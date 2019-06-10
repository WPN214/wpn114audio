import QtQuick 2.0
import WPN114.Audio 1.1 as WPN114

Item
{
    WPN114.Graph
    {
        // these are the default graph property values
        rate: 44100; vector: 512

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
            outAudioTargets: "REAPER"
            outAudioRouting: [0, 1, 1, 0]

            WPN114.Sinetest {
                id: sinetest
                frequency.value: 440.0
                routing: [0, 0, 0, 1]
            }
        }
    }
}
