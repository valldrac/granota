PREFIX := $(HOME)/fuzz/root
DISTDIR := ${HOME}/fuzz/dist

GRANOTA_LIBDIR := /usr/local/lib

CLAMAV := 0.98.4
ZLIB := 1.2.8
BZIP2 := 1.0.6

all: download extract install setup

download:
	wget -c -P ${DISTDIR} https://www.clamav.net/downloads/production/clamav-${CLAMAV}.tar.gz
	wget -c -P ${DISTDIR} http://zlib.net/zlib-${ZLIB}.tar.gz
	wget -c -P ${DISTDIR} http://www.bzip.org/${BZIP2}/bzip2-${BZIP2}.tar.gz

extract: clamav-extract zlib-extract bzip2-extract

clamav-extract:
	tar xzf ${DISTDIR}/clamav-${CLAMAV}.tar.gz

zlib-extract:
	tar xzf ${DISTDIR}/zlib-${ZLIB}.tar.gz

bzip2-extract:
	tar xzf ${DISTDIR}/bzip2-${BZIP2}.tar.gz

clean:
	rm -rf zlib-${ZLIB} bzip2-${BZIP2} clamav-${CLAMAV}

install: zlib-install bzip2-install clamav-install

clamav-install:
	cd clamav-${CLAMAV} \
	  && CC=granota-gcc \
	     CXX=granota-g++ \
	     LD_LIBRARY_PATH=${PREFIX}/lib:${GRANOTA_LIBDIR} \
	     ./configure \
	       --disable-clamav \
	       --with-libbz2-prefix=${PREFIX} \
	       --with-zlib=${PREFIX} \
	       --prefix=${PREFIX} \
	  && make -j 4 \
	  && make install

zlib-install: zlib-patch
	cd zlib-${ZLIB} \
	  && CC=granota-gcc \
	     CFLAGS="-fPIC" \
	     ./configure \
	       --static \
	       --prefix=${PREFIX} \
	  && make install

bzip2-install:
	cd bzip2-${BZIP2} \
	  && make install \
	       PREFIX=${PREFIX} \
	       CC=granota-gcc \
	       CFLAGS="-fPIC"

zlib-patch:
	-cd zlib-${ZLIB} \
	  && patch -N -p1 < ../zlib-nocrc.patch

setup:
	echo -n 'MZ' > ${PREFIX}/foobar.exe
	cp test.ndb ${PREFIX}
	ln -fs ${PWD}/clamav-${CLAMAV}/test ${PREFIX}/seeds
	mkdir -p ${PREFIX}/{testcases,crashes}
