#include <iostream>
#include "stream.hpp"
#include "audio.hpp"

using namespace wpn114::signal;

int test_case_1()
{
    graph::configure(44100, 512, 32);

    sinetest sine(440);
    dummy_output out;

    graph::connect(sine, out);
    return graph::run(out);
}

int test_case_2()
{
    graph::configure(44100, 512, 32);
    sinetest sine(440);

    vca vcamp;
    dummy_output out;

    graph::connect(sine, vcamp);
    graph::connect(vcamp, out);

    return graph::run(out);
}

int test_case_3()
{
    graph::configure(44100, 512, 32);
    sinetest sine(440);

    vca vcamp;
    dummy_output out;

    graph::connect(sine, vcamp);
    graph::connect(vcamp, out);

    connection& fb = graph::connect(vcamp, sine.inpin("FREQUENCY"));
    fb *= 200;

    return graph::run(out);
}

int main()
{
    return test_case_1();
}
