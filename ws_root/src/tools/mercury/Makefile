#
# @(#) Makefile 96/11/20 1.18
#
SHELL = /bin/sh

MERCLIB = /thdo/ssw/m2/examples_data/MercuryLinkLibs
VER = 01Sep96
LIBDIR = libs
HGRELEASE = release
TOOLSDIR = txttools
LIGHTDIR = lightwave
ALIASDIR = alias/OpenModel/lib
TDSDIR = 3dstosdf
MERCTOOLSDIR = $(TOOLSDIR)/merc_specific

LIBPBM = libpbm.a
LIBPGM = libpgm.a
LIBPPM = libppm.a
LIBTIF = libtiff.a

#DECODE_LIBS = \
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Sun/$(LIBPBM)	$(LIBDIR)/Sun/$(LIBPBM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Sun/$(LIBPGM)	$(LIBDIR)/Sun/$(LIBPGM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Sun/$(LIBPPM)	$(LIBDIR)/Sun/$(LIBPPM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Sun/$(LIBTIF)	$(LIBDIR)/Sun/$(LIBTIF)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/SGI/$(LIBPBM)	$(LIBDIR)/SGI/$(LIBPBM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/SGI/$(LIBPGM)	$(LIBDIR)/SGI/$(LIBPGM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/SGI/$(LIBPPM)	$(LIBDIR)/SGI/$(LIBPPM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/SGI/$(LIBTIF)	$(LIBDIR)/SGI/$(LIBTIF)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Mac/$(LIBPBM)	$(LIBDIR)/Mac/$(LIBPBM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Mac/$(LIBPGM)	$(LIBDIR)/Mac/$(LIBPGM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Mac/$(LIBPPM)	$(LIBDIR)/Mac/$(LIBPPM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Mac/$(LIBTIF)	$(LIBDIR)/Mac/$(LIBTIF)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Dos/$(LIBPBM)	$(LIBDIR)/Dos/$(LIBPBM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Dos/$(LIBPGM)	$(LIBDIR)/Dos/$(LIBPGM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Dos/$(LIBPPM)	$(LIBDIR)/Dos/$(LIBPPM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Dos/$(LIBTIF)	$(LIBDIR)/Dos/$(LIBTIF)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/LinuxIntel/$(LIBPBM)	$(LIBDIR)/LinuxIntel/$(LIBPBM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/LinuxIntel/$(LIBPGM)	$(LIBDIR)/LinuxIntel/$(LIBPGM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/LinuxIntel/$(LIBPPM)	$(LIBDIR)/LinuxIntel/$(LIBPPM)	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/LinuxIntel/$(LIBTIF)	$(LIBDIR)/LinuxIntel/$(LIBTIF)	\
#	${SLN} $(MERCLIB)/$(VER)/$(TOOLSDIR)/tools.rsrc		$(TOOLSDIR)/tools.rsrc	\
#	${SLN} $(MERCLIB)/$(VER)/$(LIGHTDIR)/tools.rsrc		$(LIGHTDIR)/tools.rsrc	\
#	${SLN} $(MERCLIB)/$(VER)/$(ALIASDIR)/libalias_api.a	$(ALIASDIR)/libalias_api.a	\
#	${SLN} $(MERCLIB)/$(VER)/$(ALIASDIR)/libapi_image.a	$(ALIASDIR)/libapi_image.a	\
#	${SLN} $(MERCLIB)/$(VER)/$(ALIASDIR)/libapi_memory.a	$(ALIASDIR)/libapi_memory.a	\
#	${SLN} $(MERCLIB)/$(VER)/$(TDSDIR)/3dstosdf.rsrc	$(TDSDIR)/3dstosdf.rsrc	\
#	${SLN} $(MERCLIB)/$(VER)/$(MERCTOOLSDIR)/tools.rsrc	$(MERCTOOLSDIR)/tools.rsrc

# ENCODE_LIBS = ${DECODE_LIBS:%=%.r64}

all:
	[ -d $(HGRELEASE) ] || ${MD} $(HGRELEASE)
	[ -d $(HGRELEASE)/Sun ] || ${MD} $(HGRELEASE)/Sun
	[ -d $(HGRELEASE)/SGI ] || ${MD} $(HGRELEASE)/SGI
	[ -d $(HGRELEASE)/Mac ] || ${MD} $(HGRELEASE)/Mac
	[ -d $(HGRELEASE)/Dos ] || ${MD} $(HGRELEASE)/Dos
	[ -d $(HGRELEASE)/LinuxIntel ] || ${MD} $(HGRELEASE)/LinuxIntel
	[ -d $(LIBDIR)/Sun ] || ${MD} $(LIBDIR)/Sun
	[ -d $(LIBDIR)/SGI ] || ${MD} $(LIBDIR)/SGI
	[ -d $(LIBDIR)/Mac ] || ${MD} $(LIBDIR)/Mac
	[ -d $(LIBDIR)/Dos ] || ${MD} $(LIBDIR)/Dos
	[ -d $(LIBDIR)/LinuxIntel ] || ${MD} $(LIBDIR)/LinuxIntel
	[ -d $(ALIASDIR) ] || ${MD} $(ALIASDIR)
	${RM} $(LIBDIR)/Sun/$(LIBPBM)
	${RM} $(LIBDIR)/Sun/$(LIBPGM)
	${RM} $(LIBDIR)/Sun/$(LIBPPM)
	${RM} $(LIBDIR)/Sun/$(LIBTIF)
	${RM} $(LIBDIR)/SGI/$(LIBPBM)
	${RM} $(LIBDIR)/SGI/$(LIBPGM)
	${RM} $(LIBDIR)/SGI/$(LIBPPM)
	${RM} $(LIBDIR)/SGI/$(LIBTIF)
	${RM} $(LIBDIR)/Mac/$(LIBPBM)
	${RM} $(LIBDIR)/Mac/$(LIBPGM)
	${RM} $(LIBDIR)/Mac/$(LIBPPM)
	${RM} $(LIBDIR)/Mac/$(LIBTIF)
	${RM} $(LIBDIR)/Dos/$(LIBPBM)
	${RM} $(LIBDIR)/Dos/$(LIBPGM)
	${RM} $(LIBDIR)/Dos/$(LIBPPM)
	${RM} $(LIBDIR)/Dos/$(LIBTIF)
	${RM} $(LIBDIR)/LinuxIntel/$(LIBPBM)
	${RM} $(LIBDIR)/LinuxIntel/$(LIBPGM)
	${RM} $(LIBDIR)/LinuxIntel/$(LIBPPM)
	${RM} $(LIBDIR)/LinuxIntel/$(LIBTIF)
	${RM} $(TOOLSDIR)/tools.rsrc
	${RM} $(LIGHTDIR)/tools.rsrc
	${RM} $(ALIASDIR)/libalias_api.a
	${RM} $(ALIASDIR)/libapi_image.a
	${RM} $(ALIASDIR)/libapi_memory.a
	${RM} $(TDSDIR)/3dstosdf.rsrc
	${RM} $(MERCTOOLSDIR)/tools.rsrc
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Sun/$(LIBPBM)	$(LIBDIR)/Sun/$(LIBPBM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Sun/$(LIBPGM)	$(LIBDIR)/Sun/$(LIBPGM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Sun/$(LIBPPM)	$(LIBDIR)/Sun/$(LIBPPM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Sun/$(LIBTIF)	$(LIBDIR)/Sun/$(LIBTIF)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/SGI/$(LIBPBM)	$(LIBDIR)/SGI/$(LIBPBM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/SGI/$(LIBPGM)	$(LIBDIR)/SGI/$(LIBPGM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/SGI/$(LIBPPM)	$(LIBDIR)/SGI/$(LIBPPM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/SGI/$(LIBTIF)	$(LIBDIR)/SGI/$(LIBTIF)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Mac/$(LIBPBM)	$(LIBDIR)/Mac/$(LIBPBM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Mac/$(LIBPGM)	$(LIBDIR)/Mac/$(LIBPGM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Mac/$(LIBPPM)	$(LIBDIR)/Mac/$(LIBPPM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Mac/$(LIBTIF)	$(LIBDIR)/Mac/$(LIBTIF)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Dos/$(LIBPBM)	$(LIBDIR)/Dos/$(LIBPBM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Dos/$(LIBPGM)	$(LIBDIR)/Dos/$(LIBPGM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Dos/$(LIBPPM)	$(LIBDIR)/Dos/$(LIBPPM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/Dos/$(LIBTIF)	$(LIBDIR)/Dos/$(LIBTIF)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/LinuxIntel/$(LIBPBM)	$(LIBDIR)/LinuxIntel/$(LIBPBM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/LinuxIntel/$(LIBPGM)	$(LIBDIR)/LinuxIntel/$(LIBPGM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/LinuxIntel/$(LIBPPM)	$(LIBDIR)/LinuxIntel/$(LIBPPM)
	${SLN} $(MERCLIB)/$(VER)/$(LIBDIR)/LinuxIntel/$(LIBTIF)	$(LIBDIR)/LinuxIntel/$(LIBTIF)
	${SLN} $(MERCLIB)/$(VER)/$(TOOLSDIR)/tools.rsrc		$(TOOLSDIR)/tools.rsrc
	${SLN} $(MERCLIB)/$(VER)/$(LIGHTDIR)/tools.rsrc		$(LIGHTDIR)/tools.rsrc
	${SLN} $(MERCLIB)/$(VER)/$(ALIASDIR)/libalias_api.a	$(ALIASDIR)/libalias_api.a
	${SLN} $(MERCLIB)/$(VER)/$(ALIASDIR)/libapi_image.a	$(ALIASDIR)/libapi_image.a
	${SLN} $(MERCLIB)/$(VER)/$(ALIASDIR)/libapi_memory.a	$(ALIASDIR)/libapi_memory.a
	${SLN} $(MERCLIB)/$(VER)/$(TDSDIR)/3dstosdf.rsrc	$(TDSDIR)/3dstosdf.rsrc
	${SLN} $(MERCLIB)/$(VER)/$(MERCTOOLSDIR)/tools.rsrc	$(MERCTOOLSDIR)/tools.rsrc

default:	$(DECODE_LIBS)

depends:
		@echo "no depends"

# Must be on an SGI for this !
SGI:
	cd ifflib 		; \
	gmake -f Makefile.SGI	; \
	cd .. 			; \
	cd txtlib/src 		; \
	gmake -f Makefile.SGI	; \
	cd ../..		; \
	cd txttools		; \
	gmake -f Makefile.SGI	; \
	cd merc_specific	; \
	gmake -f Makefile.SGI	; \
	cd ../..		; \
	cd lightwave		; \
	gmake -f Makefile.SGI	; \
	cd ..
	cd 3dstosdf		; \
	gmake -f Makefile.SGI	; \
	cd ..
	cd gmerc		; \
	gmake -f Makefile.SGI	; \
	cd ..
	cd alias/altosdf		; \
	make altosdf	; \
	cd ../..
	
Sun:
	cd ifflib 		; \
	gmake -f Makefile.Sun	; \
	cd .. 			; \
	cd txtlib/src 		; \
	gmake -f Makefile.Sun	; \
	cd ../..		; \
	cd txttools		; \
	gmake -f Makefile.Sun	; \
	cd merc_specific	; \
	gmake -f Makefile.Sun	; \
	cd ../..		; \
	cd lightwave		; \
	gmake -f Makefile.Sun	; \
	cd ..
	

encode: $(ENCODE_LIBS)

decode: $(DECODE_LIBS)

clean:
	${RM} $(LIBDIR)/Sun/$(LIBPBM)
	${RM} $(LIBDIR)/Sun/$(LIBPGM)
	${RM} $(LIBDIR)/Sun/$(LIBPPM)
	${RM} $(LIBDIR)/Sun/$(LIBTIF)
	${RM} $(LIBDIR)/SGI/$(LIBPBM)
	${RM} $(LIBDIR)/SGI/$(LIBPGM)
	${RM} $(LIBDIR)/SGI/$(LIBPPM)
	${RM} $(LIBDIR)/SGI/$(LIBTIF)
	${RM} $(LIBDIR)/Mac/$(LIBPBM)
	${RM} $(LIBDIR)/Mac/$(LIBPGM)
	${RM} $(LIBDIR)/Mac/$(LIBPPM)
	${RM} $(LIBDIR)/Mac/$(LIBTIF)
	${RM} $(LIBDIR)/Dos/$(LIBPBM)
	${RM} $(LIBDIR)/Dos/$(LIBPGM)
	${RM} $(LIBDIR)/Dos/$(LIBPPM)
	${RM} $(LIBDIR)/Dos/$(LIBTIF)
	${RM} $(LIBDIR)/LinuxIntel/$(LIBPBM)
	${RM} $(LIBDIR)/LinuxIntel/$(LIBPGM)
	${RM} $(LIBDIR)/LinuxIntel/$(LIBPPM)
	${RM} $(LIBDIR)/LinuxIntel/$(LIBTIF)
	${RM} $(TOOLSDIR)/tools.rsrc
	${RM} $(LIGHTDIR)/tools.rsrc
	${RM} $(ALIASDIR)/libalias_api.a
	${RM} $(ALIASDIR)/libapi_image.a
	${RM} $(ALIASDIR)/libapi_memory.a
	${RM} $(TDSDIR)/3dstosdf.rsrc
	${RM} $(MERCTOOLSDIR)/tools.rsrc
	cd ifflib 		; \
	gmake -f Makefile.SGI clean	; \
	gmake -f Makefile.Sun clean	; \
	cd .. 			; \
	cd txtlib/src 		; \
	gmake -f Makefile.SGI clean	; \
	gmake -f Makefile.Sun clean	; \
	cd ../..		; \
	cd txttools		; \
	gmake -f Makefile.SGI clean	; \
	gmake -f Makefile.Sun clean	; \
	cd merc_specific	; \
	gmake -f Makefile.SGI clean	; \
	gmake -f Makefile.Sun clean	; \
	cd ../..		; \
	cd lightwave		; \
	gmake -f Makefile.SGI clean	; \
	gmake -f Makefile.Sun clean	; \
	cd ..
	cd 3dstosdf		; \
	gmake -f Makefile.SGI clean	; \
	cd ..
	cd gmerc		; \
	gmake -f Makefile.SGI clean	; \
	cd ..
	cd alias/altosdf	; \
	make clean	; \
	cd ../..
	
clobber:	
	cd ifflib 		; \
	gmake -f Makefile.SGI clobber	; \
	gmake -f Makefile.Sun clobber	; \
	cd .. 			; \
	cd txtlib/src 		; \
	gmake -f Makefile.SGI clobber	; \
	gmake -f Makefile.Sun clobber	; \
	cd ../..		; \
	cd txttools		; \
	gmake -f Makefile.SGI clobber	; \
	gmake -f Makefile.Sun clobber	; \
	cd merc_specific	; \
	gmake -f Makefile.SGI clobber	; \
	gmake -f Makefile.Sun clobber	; \
	cd ../..		; \
	cd lightwave		; \
	gmake -f Makefile.SGI clobber	; \
	gmake -f Makefile.Sun clobber	; \
	cd 3dstosdf		; \
	gmake -f Makefile.SGI clobber	; \
	cd ..
	cd gmerc		; \
	gmake -f Makefile.SGI clobber	; \
	cd ..
	cd alias/altosdf		; \
	make altosdf	; \
	cd ..

ifdef CODEMGR_WS

include ${CODEMGR_WS}/ws_root/variables/generic 

#%.a: %.a.r64
#	-${R64DEC} < $< > $@

#%.a.r64: %.a
#	-${R64ENC} < $< > $@

#%.rsrc: %.rsrc.r64
#	-${R64DEC} < $< > $@

#%.rsrc.r64: %.rsrc
#	-${R64ENC} < $< > $@

endif
