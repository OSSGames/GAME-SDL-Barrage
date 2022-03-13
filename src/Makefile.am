bin_PROGRAMS = barrage

barrage_SOURCES = main.c particle.c shots.c units.c \
	data.c chart.c bfield.c menu.c SFont.c

EXTRA_DIST = defs.h particle.h shots.h units.h \
	data.h bfield.h menu.h SFont.h chart.h empty.hscr

SUBDIRS = gfx sounds

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datdir)
	$(mkinstalldirs) $(DESTDIR)$(hiscoredir)
	if ! test -f $(DESTDIR)$(hiscoredir)/barrage.hscr; then \
	  $(INSTALL_DATA) -m 666 empty.hscr $(DESTDIR)$(hiscoredir)/barrage.hscr; \
	fi;

uninstall-local:
	rm -rf $(DESTDIR)$(datadir)/barrage
	rm $(DESTDIR)$(hiscoredir)/barrage.hscr

clean:
	rm -f *.o barrage