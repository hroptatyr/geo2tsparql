# Help the Developers and yourself. Just use the C locale and settings
# for the compilation. They can still be overriden by make LANG=<whatever>
# but that is general a not very good idea
include $(top_builddir)/version.mk

LANG = C
LC_ALL = C

AM_CFLAGS = $(EXTRA_CFLAGS)
AM_CPPFLAGS = -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600 -D_BSD_SOURCE
AM_CPPFLAGS += -DTEST

EXTRA_DIST = $(BUILT_SOURCES) $(TESTS)
TESTS =
TEST_EXTENSIONS =
BUILT_SOURCES =
dt_tests =
bin_tests =
batch_tests =
check_PROGRAMS =
CLEANFILES = $(check_PROGRAMS)

## summon our helper
include clitoris.am
AM_CLIT_LOG_FLAGS = --builddir "$(top_builddir)/src" --verbose

TESTS += $(cli_tests)
cli_tests =

cli_tests += norm_01.clit
cli_tests += norm_02.clit
cli_tests += norm_03.clit
cli_tests += norm_04.clit
cli_tests += norm_05.clit
cli_tests += norm_06.clit
cli_tests += norm_07.clit
cli_tests += norm_08.clit
cli_tests += norm_09.clit
cli_tests += norm_10.clit
cli_tests += norm_11.clit
cli_tests += norm_12.clit
cli_tests += norm_13.clit
cli_tests += norm_14.clit
cli_tests += norm_15.clit
cli_tests += norm_16.clit

## Makefile.am ends here
