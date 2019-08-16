#!/bin/bash -eux

	mkdir build 
	cd build 
	cmake ..
	make -j8
