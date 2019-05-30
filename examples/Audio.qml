import QtQuick 2.0
import WPN114.Audio 1.1 as WPN114

Item
{
    WPN114.Graph
    {
        id: graph

        // we instantiate an Output module
        // when component is complete
        // we configure the global graph with the following parameters:
        rate: 44100
        vector: 512

        // output module looks for selected output device
        // or choose the default one if unspecified
        // the audio thread is created, initialized and started

        WPN114.JackIO
        {
            id: output

            WPN114.Sinetest
            {
                id: sinetest

                frequency: 440.0
                WPN114.VCA on outputs { id: vca; gain: db(-12) }
            }
        }
    }

    // on audio start
    // graph first initializes all the registered nodes
    // allocating the different i/o pins
    // and then allocates the connection streams

    Component.onCompleted: output.run("REAPER")
}
