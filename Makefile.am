AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -Wl,--no-as-needed

AM_CPPFLAGS += -W  -Wall -g


# bin
bin_PROGRAMS = diyafm
diyafm_SOURCES = fm.c \
	diyafm_app.c \
	diyafm_win.c \
	diyafm_prefs.c \
	diyafm_app.h \
	diyafm_win.h \
	diyafm_prefs.h \
	resources.c
diyafm_CPPFLAGS = $(GTK_CFLAGS)
diyafm_LDFLAGS= $(GTK_LIBS)

BUILT_SOURCES = \
	resources.c \
	gschemas.compiled


resources.c: diyafm.gresource.xml resources/fm.ui resources/app-menu.ui resources/prefs.ui resources/gears-menu.ui
	glib-compile-resources $(srcdir)/diyafm.gresource.xml \
		--target=$@ --sourcedir=$(srcdir) --generate-source

gsettings_SCHEMAS = data/app.iohub.dev.diyafm.gschema.xml

@GSETTINGS_RULES@

gschemas.compiled: data/app.iohub.dev.diyafm.gschema.xml
	$(GLIB_COMPILE_SCHEMAS) ./data

CLEANFILES = data/gschemas.compiled

EXTRA_DIST = README.md \
			diyafm.gresource.xml \
			resources \
			data