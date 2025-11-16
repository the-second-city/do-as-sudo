+++
title = "Fastfetch and Images"
date = "2025-11-16T09:46:20-08:00"

#
# description is optional
#
# description = "An optional description for SEO. If not provided, an automatically created summary will be used."

tags = []
+++

[`fastfetch`](https://github.com/fastfetch-cli/fastfetch) is a tool for displaying system information, such as memory use, uptime, etc. It’s often faster than pulling up `htop` or various `sysctl` invocations.

It is also [endlessly customizable](https://m3tozz.github.io/FastCat-Themes/).

If your terminal supports it, `fastfetch` can display an [image logo](https://github.com/fastfetch-cli/fastfetch/wiki/Logo-options) instead of ASCII art.

This was not working for me.

Did some digging. `fastfetch —show-errors` gave me something useful:

> Logo: Image Magick library not found

Odd, since `pkg info ImageMagick7` showed it installed.

Hmm …

One lengthy ChatGPT conversation later, and we have the culprit! The FreeBSD ImageMagick7 installation was missing Q16HDRI libraries. The solution: a new file called `/usr/local/etc/libmap.d/fastfetch-imagemagick.conf`:

```
libMagickCore-7.Q16HDRI.so libMagickCore-7.so.10
libMagickWand-7.Q16HDRI.so libMagickWand-7.so.10
libMagickCore-7.Q16.so    libMagickCore-7.so.10
libMagickWand-7.Q16.so    libMagickWand-7.so.10
```

Voila!
