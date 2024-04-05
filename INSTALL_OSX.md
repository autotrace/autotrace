At https://github.com/autotrace/autotrace/releases we provide binary packages for some systems. Please check them out first. If that works, you can skip all the rest of the build instructions here.

Build on OSX
============
On OSX only GraphicsMagick is supported.

for a minimal make, you can run build without GraphicsMagick or pstoedit
support.  Which is probably a good idea anyway, as long as we have known
security issues when using pstoedit.  For this, use `configure --without-magick
--without-pstoedit`

```sh
perl -MCPAN -e 'install XML::Parser'
perl -e "require XML::Parser"
brew update
brew install gettext intltool glib libtool autoconf automake pkg-config
brew install graphicsmagick pstoedit libpng
git clone https://github.com/autotrace/autotrace.git
cd autotrace
./autogen.sh
./configure
make -j$(sysctl -n hw.activecpu)
sudo make install
```

