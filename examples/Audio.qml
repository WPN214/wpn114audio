import QtQuick 2.0
import WPN114.Audio 1.1 as WPN114

Item
{
    WPN114.Audiostream
    {
        id: audiostream

        // we instantiate an Output module
        // when component is complete
        // we configure the global graph with the following parameters:
        rate: 44100
        vector: 512
        feedback: 64

        // output module looks for selected output device
        // or choose the default one if unspecified
        // the audio thread is created, initialized and started

        WPN114.Sinetest
        {
            id: sinetest

            frequency: 440.0
            WPN114.VCA on output { id: vca; gain: db(-6) }

            WPN114.Connection on vca.output {
                dest: some_effect
                level: db(-6)
                routing: [[0, 1], [1, 0]]
                // would be equivalent to
                pattern: WPN114.Connection.Crossed
            }
        }

//        WPN114.VCA
//        {
//            id: vca
//            gain: -6

//            WPN114.Sinetest
//            {
//                id: sinetest
//                frequency: 440.0
//            }
//        }
    }

    // on audio start
    // graph first initializes all the registered nodes
    // allocating the different i/o pins
    // and then allocates the connection streams

    Component.onCompleted: audiostream.start();
}
