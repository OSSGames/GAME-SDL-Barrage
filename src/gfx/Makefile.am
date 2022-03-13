EXTRA_DIST = ammo.bmp crater.bmp ft_red.bmp ft_yellow.bmp ground.bmp \
logo.bmp particles.bmp shots.bmp small_crater.bmp trees.bmp units.bmp \
ft_red14.bmp ft_neon.bmp cursors.bmp icons.bmp gun.bmp

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(datdir)/gfx
	@for file in $(EXTRA_DIST); do\
	  $(INSTALL_DATA) $$file $(DESTDIR)$(datdir)/gfx/$$file;\
	done

