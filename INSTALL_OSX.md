Build on OSX
============

On OSX, gettext gets installed into /usr/local/opt/gettext, so we need to add
that to the path or autoreconf and intltoolize would fail due to missing 
`autopoint`.

This is a minimal make, without ImageMagic or pstoedit support.
Which is probably a good idea anyway, as long as we have known security issues 
when using pstoedit.

```
export PATH="/usr/local/opt/gettext/bin:$PATH"
brew install intltool
autoreconf -ivf
intltoolize --force
sh ./configure --without-magick --without-pstoedit
make
```

