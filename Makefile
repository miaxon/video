# Main Makefile
#

DIRS = 	src

BUILD_DIRS = ${DIRS}

.PHONY: all clean

all: 
	echo ${BUILD_DIRS}
	@ for dir in ${BUILD_DIRS}; do (cd $${dir}; ${MAKE}) ; done

clean: 
	@ for dir in ${BUILD_DIRS}; do (cd $${dir}; ${MAKE} clean) ; done
