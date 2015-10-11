PREFIX = /usr/local
LD = ld
CC = g++
PKG_CONFIG = pkg-config
INSTALL = install
CFLAGS = -g -O2 -Wall -Wextra
LDFLAGS =
LIBS =
VLC_PLUGIN_CFLAGS := $(shell $(PKG_CONFIG) --cflags vlc-plugin)
VLC_PLUGIN_CFLAGS_I := $(shell $(PKG_CONFIG) --cflags-only-I vlc-plugin)
VLC_PLUGIN_LIBS := $(shell $(PKG_CONFIG) --libs vlc-plugin)

libdir = $(PREFIX)/lib
plugindir = $(libdir)/vlc/plugins

override CC += -std=gnu99
override CPPFLAGS += -DPIC -I. -Isrc -std=gnu++11 
override CFLAGS += -fPIC
override LDFLAGS += -Wl,-no-undefined -static-libstdc++ -static-libgcc

override CPPFLAGS += -DMODULE_STRING=\"sapispeechsynthesizer\" $(VLC_PLUGIN_CFLAGS_I) -Drestrict=__restrict__
override CFLAGS += $(VLC_PLUGIN_CFLAGS)
override LIBS += $(VLC_PLUGIN_LIBS) -lole32 -lsapi

TARGETS = libsapispeechsynthesizer_plugin.dll

all: libsapispeechsynthesizer_plugin.dll

install: all
		mkdir -p -- $(DESTDIR)$(plugindir)/misc
		$(INSTALL) --mode 0755 libsapispeechsynthesizer_plugin.dll $(DESTDIR)$(plugindir)/misc

install-strip:
		$(MAKE) install INSTALL="$(INSTALL) -s"

uninstall:
		rm -f $(plugindir)/misc/libsapispeechsynthesizer_plugin.dll

clean:
		rm -f -- libsapispeechsynthesizer_plugin.dll src/*.o

mostlyclean: clean

SOURCES = sapispeechsynthesizer.cpp

#$(SOURCES:%.c=src/%.o): %: src/sapispeechsynthesizer.h

libsapispeechsynthesizer_plugin.dll: $(SOURCES:%.cpp=src/%.o)
		$(CC) $(LDFLAGS) -shared -o $@ $^ $(LIBS)

.PHONY: all install install-strip uninstall clean mostlyclean
