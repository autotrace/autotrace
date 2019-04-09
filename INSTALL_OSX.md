At https://github.com/autotrace/autotrace/releases we provide binary packages for some systems. Please check them out first. If that works, you can skip all the rest of the build instructions here.


Build on OSX
============

On OSX, gettext gets installed into /usr/local/opt/gettext, so we need to add
that to the path or autoreconf and intltoolize would fail due to missing
`autopoint`.

On OSX, imagemagick as installed with brew does not provide the magick/api.h we use GraphicsMagick instead,
which is supposed to be compatible. We need to have imagemagick installed only to keep configure happy.

for a minimal make, you can run build without ImageMagic or pstoedit support.
Which is probably a good idea anyway, as long as we have known security issues when using pstoedit.
For this, use `configure --without-magick --without-pstoedit`

```
export PATH="/usr/local/opt/gettext/bin:$PATH"
brew install gettext intltool glib libtool autoconf automake pkg-config
brew install imagemagick graphicsmagick pstoedit
git clone https://github.com/autotrace/autotrace.git
cd autotrace
autoreconf -ivf
intltoolize --force
aclocal
test -e /usr/local/lib/pkgconfig/libffi.pc || ln -s ../../Cellar/libffi/3.2.1/lib/pkgconfig/libffi.pc /usr/local/lib/pkgconfig/
sh ./configure MAGICK_CFLAGS="$(pkg-config graphicsmagick --cflags)" MAGICK_LIBS="$(pkg-config graphicsmagick --libs)"
make
make install
```

