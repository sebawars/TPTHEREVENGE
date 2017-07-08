#!/usr/bin/env bash -l
	mv so-commons-library ../
	mv tp-2017-1c-The-Revenge ../
	cp makefile ../
	cd ..
	sudo make all
	export LD_LIBRARY_PATH=/home/utnso/tp-2017-1c-The-Revenge/tr_library/Debug
