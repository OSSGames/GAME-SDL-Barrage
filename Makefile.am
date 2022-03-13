SUBDIRS = src

AUTOMAKE_OPTIONS = foreign
 
RPMBUILD = rpmbuild
rpm: dist
	$(RPMBUILD) -ta $(distdir).tar.gz

desktopdir = $(datadir)/applications
desktop_DATA = barrage.desktop
icondir = $(datadir)/icons
icon_DATA = barrage48.png

EXTRA_DIST = BUGS README COPYING AUTHORS ChangeLog INSTALL NEWS TODO \
	$(PACKAGE).spec $(PACKAGE).spec.in barrage.desktop.in barrage48.png

# We can't use static desktop file because of icon location. Is this 
# the right way to handle it? At least it works... as long as $datadir
# does not contain plus signs. :-)
barrage.desktop:
	@sed -e "s+Icon=+Icon=$(datadir)/icons/barrage48.png+" \
	< barrage.desktop.in > barrage.desktop

ACLOCAL_AMFLAGS = -I m4