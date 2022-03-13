EXTRA_DIST = autocannon.wav click.wav expl1.wav expl2.wav expl3.wav \
gunfire.wav highlight.wav

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datdir)/sounds
	@for file in $(EXTRA_DIST); do\
	  $(INSTALL_DATA) $$file $(DESTDIR)$(datdir)/sounds/$$file;\
	done

