import QtQuick 2.12
import QtQuick.Window 2.12
import WPN114.Audio 1.1 as WPN114

Item
{
    WPN114.Graph
    {
        // these are the default graph property values
        rate: 44100; vector: 512
        external.name: "wpn114audio-test"
        external.backend: WPN114.External.Jack
        external.running: true

        WPN114.Output
        {
            name: "audio_out"            
            channels: [0, 1]

            WPN114.Sinetest
            {
                id: sinetest
                name: "sinetest"
                frequency.value: 440.0
                audio_out.mul: 0.15
                audio_out.routing: [0, 0, 0, 1]
            }
        }
    }
}
