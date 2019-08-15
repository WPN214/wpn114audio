#!/bin/bash -eux

	if [ $TRAVIS_OS_NAME == linux ]; then
		sudo chmod -R a+rwx /opt
		sudo apt-get update -qq
		sudo apt-get install wget 
		wget -nv https://cmake.org/files/v3.14/cmake-3.14.3-Linux-x86_64.tar.gz -O cmake-linux.tgz

		sudo apt-get install -qq --force-yes \
			libasound-dev jack2 portaudio19-dev \
			libgl1-mesa-dev \
			qt513base qt512quickcontrols2 

		wait wget || true

		tar xaf cmake-linux.tgz
    	mv cmake-*-x86_64 cmake-latest
	fi

	if [ $TRAVIS_OS_NAME == osx ]; then
		brew update
		brew install qt5 
		brew install portaudio 
	fi




