import QtQuick 2.0
import WPN114.Audio 1.1 as WPN114

Item
{
    WPN114.Audiostream
    {
        id: audiostream

        // these are the default
        // stream properties
        rate: 44100
        vector: 512
        feedback: 64

        WPN114.Sinetest
        {
            id: sinetest

            // -------------------------------------------------------
            // here frequency is declared as a WPN_SIGNAL
            // it is the default input signal for the Sinetest module
            // it can be assigned different types of values:
            // -------------------------------------------------------
            // - an int/real
            frequency: 440.0

            // - another pin/signal (this will be feedback)
            frequency: vca.output

            // - an array of pins
            frequency: [ vca.output ]

            // - a node
            frequency: vca

            // ------------------------------------------
            // this dispatch option means
            // that VCA's output will be the audiostream
            dispatch: WPN114.FXChain
            // equivalent to:
            output: vca

            //-------------------------------------------

            WPN114.VCA
            {
                id: vca
                gain: -6

                // output to right channel instread of left
                // connection function will notify backend
                // that the level has to be divided by 4 in this case
                output: connection(audiostream.inputs[1], 0.25);

                // this is implicit
                output: audiostream
            }
        }
    }

    Component.onCompleted: audiostream.start();
}
