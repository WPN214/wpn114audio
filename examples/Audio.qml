import QtQuick 2.0
import WPN114.Audio 1.1 as WPN114
import WPN114.Network 1.1 as WPN214


Item
{    
    WPN214.Network
    {
        hostAddr: "localhost"
        hostPort: 5678

        onConnection
        {

        }
    }

    WPN114.Graph
    {
        // these are the default graph property values
        rate: 44100; vector: 512

        name: "wpn114audio-test-device"
        extern.backend: WPN114.External.Jack
        extern.outAudioTargets: "REAPER"
        extern.outAudioRouting: [0, 1, 1, 0]

        network.exposed: true
        network.udp: 1234
        network.tcp: 5678

        WPN114.Output
        {
            WPN114.Sinetest
            {
                id: sinetest
                name: "sinetest"
                frequency.value: 440.0
                audio_out.routing: [0, 0, 0, 1]
            }
        }
    }
}
