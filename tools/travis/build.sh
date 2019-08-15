#!/bin/bash -eux

case "$TRAVIS_OS_NAME" in
	linux)
	mkdir build 
	cd build 
	cmake ..
	make -j8