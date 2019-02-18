import QtQuick 2.0
import WPN114.Audio 1.1 as WPN114

Item
{
    WPN114.Audiostream
    {
        id: audiostream
        rate: 44100
        vector: 512
        feedback: 64

        WPN114.Sinetest
        {
            id: sinetest
            frequency: 440.0

            frequency: connection(vca, 0.5);
            WPN114.VCA on output { gaindB: -6 }
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

    Component.onCompleted: audiostream.start();
}
