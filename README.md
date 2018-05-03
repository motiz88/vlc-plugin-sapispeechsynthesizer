> NOTE: [A version of this plugin](https://github.com/videolan/vlc/commits/master/modules/text_renderer/sapi.cpp) was merged into VLC itself in 2015. *This repo is now deprecated*. Thanks for having a look! The original README follows below.

---

# vlc-plugin-sapispeechsynthesizer

A VLC plugin that reads subtitles aloud using Microsoft SAPI on Windows.

This repo is very much in its initial stages. I am also new to VLC plugin development, and it probably shows.

Luckily, between the existing [VLC speech synthesis plugin for OS X](https://github.com/videolan/vlc/blob/master/modules/text_renderer/nsspeechsynthesizer.m), and various other plugins that provided solid examples for the use of COM from within VLC's heavily multithreaded guts (which could have been a nightmare otherwise), I managed to put together something that actually works.

# Usage

Put the .dll in VLC's `plugins/text_renderer` directory. Use VLC's configuration dialogs or command line to activate the plugin by name (`sapispeechsynthesizer`), load a video that has a subtitle track and it will (hopefully) start talking.

# Building

Honestly, I barely got this thing to build myself. Other than give some disorganized pointers, the most I can do is point you to the VLC wiki pages about [writing modules](https://wiki.videolan.org/Hacker_Guide/How_To_Write_a_Module) and [compiling out-of-tree modules](https://wiki.videolan.org/OutOfTreeCompile/). I will gladly accept PRs to sort out the build process and/or improve the following ad-hoc documentation.

So, some pointers that worked for me (but YMMV):
* Building with MinGW on Windows works. I downloaded a 32-bit toolchain from [MinGW-w64](http://mingw-w64.org/doku.php).
* The `Makefile` depends on `pkg-config`. I downloaded [`pkg-config-lite`](http://sourceforge.net/projects/pkgconfiglite/) and extracted `pkg-config.exe` to somewhere in the `PATH`.
* `PKG_CONFIG_PATH` should point to VLC's `sdk/lib/pkgconfig` directory. On my machine I had to copy the entire SDK directory elsewhere, because the special characters in `C:\Program Files (x86)` threw some of the tools off.
* The VLC SDK directory doesn't contain all the required headers, apparently. I had to copy some files from the equivalent VLC source version.
* MinGW's linker did not like the `libvlccore.lib` that came with VLC. I copied the actual `libvlccore.dll` to my modified SDK directory and that worked OK.
