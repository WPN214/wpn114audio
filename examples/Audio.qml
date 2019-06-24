import QtQuick 2.0
import WPN114.Audio 1.1 as WPN114

Item
{
    WPN114.Graph
    {
        // these are the default graph property values
        rate: 44100; vector: 512

        // this will expose
        name: "wpn114audio-test-device"
        extern.backend: WPN114.External.Jack
        extern.outAudioTargets: "REAPER"
        extern.outAudioRouting: [0, 1, 1, 0]

        network.exposed: true
        network.tcp: 5678
        network.udp: 1234

        WPN114.Sinetest
        {
            id: sinetest
            name: "sinetest"
            frequency.value: 440.0
            audio_out.routing: [0, 0, 0, 1]
        }
    }
}
