CLEANFILES = tmp* core *.core $(EXTRA_PROGRAMS) *.*out *.log eval.in2

TESTS = $(check_PROGRAMS) $(SEDTESTS)
SEDTESTS =

LDADD = ../lib/libsed.a @INTLLIBS@
noinst_HEADERS = testcases.h ptestcases.h
AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_builddir)/lib

if TEST_REGEX
check_PROGRAMS = bug-regex7 \
                 bug-regex8 bug-regex9 bug-regex10 bug-regex11 bug-regex12 \
                 bug-regex13 bug-regex14 bug-regex15 bug-regex16 bug-regex21 \
		 bug-regex27 bug-regex28 \
                 tst-pcre tst-boost runtests runptests tst-rxspencer tst-regex2

SEDTESTS += space
endif

SEDTESTS += \
        appquit enable sep inclib 8bit newjis xabcx dollar noeol noeolw \
	modulo numsub numsub2 numsub3 numsub4 numsub5 0range bkslashes \
	head madding mac-mf empty xbxcx xbxcx3 recall recall2 xemacs \
	fasts uniq manis khadafy linecnt eval distrib 8to7 y-bracket \
	y-newline allsub cv-vars classes middle bsd stdin flipcase \
	insens subwrite writeout readin insert utf8-1 utf8-2 utf8-3 utf8-4 \
	badenc inplace-hold brackets \
	help version file quiet \
	factor binary3 binary2 binary dc

TESTS_ENVIRONMENT = MAKE="$(MAKE)" VERSION="$(VERSION)" \
	@LOCALCHARSET_TESTS_ENVIRONMENT@ $(srcdir)/runtest


EXTRA_DIST = \
	PCRE.tests BOOST.tests SPENCER.tests \
	runtest Makefile.tests \
	0range.good 0range.inp 0range.sed \
	8bit.good 8bit.inp 8bit.sed \
	8to7.good 8to7.inp 8to7.sed \
	allsub.good allsub.inp allsub.sed \
	appquit.good appquit.inp appquit.sed \
	binary.good binary.inp binary.sed binary2.sed binary3.sed \
	bkslashes.good bkslashes.inp bkslashes.sed \
	brackets.good brackets.inp brackets.sed \
	bsd.good bsd.sh \
	cv-vars.good cv-vars.inp cv-vars.sed \
	classes.good classes.inp classes.sed \
	dc.good dc.inp dc.sed \
	distrib.good distrib.inp distrib.sed distrib.sh \
	dollar.good dollar.inp dollar.sed \
	empty.good empty.inp empty.sed \
	enable.good enable.inp enable.sed \
	eval.good eval.inp eval.sed \
	factor.good factor.inp factor.sed \
	fasts.good fasts.inp fasts.sed \
	flipcase.good flipcase.inp flipcase.sed \
	head.good head.inp head.sed \
	inclib.good inclib.inp inclib.sed \
	insens.good insens.inp insens.sed \
	insert.good insert.inp insert.sed \
	khadafy.good khadafy.inp khadafy.sed \
	linecnt.good linecnt.inp linecnt.sed \
	space.good space.inp space.sed \
	mac-mf.good mac-mf.inp mac-mf.sed \
	madding.good madding.inp madding.sed \
	manis.good manis.inp manis.sed \
	middle.good middle.sed middle.inp \
	modulo.good modulo.sed modulo.inp \
	newjis.good newjis.inp  newjis.sed \
	noeol.good noeol.inp noeol.sed \
	noeolw.good noeolw.1good noeolw.2good noeolw.sed \
	numsub.good numsub.inp numsub.sed \
	numsub2.good numsub2.inp numsub2.sed \
	numsub3.good numsub3.inp numsub3.sed \
	numsub4.good numsub4.inp numsub4.sed \
	numsub5.good numsub5.inp numsub5.sed \
	readin.good readin.inp readin.sed \
	recall.good recall.inp recall.sed \
	recall2.good recall2.inp recall2.sed \
	sep.good sep.inp sep.sed \
	subwrite.inp subwrite.sed  subwrt1.good subwrt2.good \
	uniq.good uniq.inp uniq.sed \
	utf8-1.good utf8-1.inp utf8-1.sed \
	utf8-2.good utf8-2.inp utf8-2.sed \
	utf8-3.good utf8-3.inp utf8-3.sed \
	utf8-4.good utf8-4.inp utf8-4.sed \
	badenc.good badenc.inp badenc.sed \
	version.gin \
	writeout.inp writeout.sed wrtout1.good wrtout2.good \
	xabcx.good xabcx.inp xabcx.sed \
	xbxcx.good xbxcx.inp xbxcx.sed \
	xbxcx3.good xbxcx3.inp xbxcx3.sed \
	xemacs.good xemacs.inp xemacs.sed \
	y-bracket.good y-bracket.sed y-bracket.inp \
	y-newline.good y-newline.sed y-newline.inp

# automake makes `check' depend on $(TESTS).  Declare
# dummy targets for $(TESTS) so that make does not complain.

.PHONY: $(SEDTESTS)
$(SEDTESTS):
