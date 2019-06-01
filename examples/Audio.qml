import QtQuick 2.0
import WPN114.Audio 1.1 as WPN114

Item
{
    WPN114.Graph
    {
        // these are the default graph property values
        rate: 44100; vector: 512

        // when the graph is complete, start audio processing
        Component.onCompleted: output.run();

        WPN114.JackIO
        {
            id: output
            // default property values as well
            numInputs: 0
            numOutputs: 2

            WPN114.Sinetest
            {
                id: sinetest
                frequency: 440.0
                output.routing: [0, 1]

                WPN114.VCA on output {
                    id: vca; gain: db(-12);
                    WPN114.Sinetest on gainmod { frequency: 1; level: 0.5 }
                }
            }
        }
    }
}
