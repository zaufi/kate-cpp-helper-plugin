Kate C++ Helper Plugin
======================

Information
-----------

This plugin intended to simplify the hard life of C/C++ programmers who use Kate to write code :-)

First of all, I tired to use the file browser to open mine (or system) header files. With version
0.1 of the plugin one may press `F10` to open a header file that has its name under cursor.
Actually, you are not even required to move a cursor to a file name if the current line starts with
`#include` directive...

Later `#include` autocompletion was implemented to help you to type (long) paths to header files used
in your sources. Here is also few little cute things:

* `Edit->Copy #include to Clipboard` to add `#include <current-file.h>` to a clipboard
* `File->Open Header/Implementation` a replacement for official _Open Header_ plugin with few enhancments
  (see below for details)
* highlight `#include` lines if header not found in configured locations or found in multiple times
* monitor configured directories for header files

Since version 0.6 I've added a code completion based on clang API and decide to rename _Kate Include Helper_
into _Kate C++ Helper Plugin_.


Requirements
------------

* C++11 compatible compiler (gcc >= 4.7 recommended)
* [clang](http://clang.llvm.org) >= 3.1
* [cmake](http://cmake.org) >= 2.8
* [Kate](http://kate-editor.org) editor version >= 2.9.


Installation
------------

* Clone sources into (some) working dir
* To install into your home directory you have to specify a prefix like this::

        $ cd <plugin-sources-dir>
        $ mkdir build && cd build
        $ cmake -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=~/.kde4 .. && make && make install

* To make a system-wide installation, set the prefix to /usr and become a superuser to `make install`
* After that u have to enable it from `Settings->Configure Kate...->Plugins` and configure the include paths
  globally and/or per session...

Note: One may use `kde4-config` utility with option `--localprefix` or `--prefix` to get
user or system-wide prefix correspondingly.


Some Features in Details
========================

Open Header/Implementation: How it works
----------------------------------------

Kate shipped with a plugin named *Open Header*, but sooner after I started to use it I've found
few cases when it can't helps me. Nowadays I have 2 "real life" examples when it fails:

Often one may find a source tree separate `${project}/src/` and `${project}/include` dirs.
So, when you are at some header from `include/` dir, that plugin never will find your source file.
And vise versa.

The second case: sometimes you have a really big class defined in a header file
(let it be `my_huge_application.hh`). It may consist of few dickers of methods each of which is
hundred lines or so. In that case I prefer to split implementation into several files and name them
after a base header like `my_huge_application_cmd_line.cc` (for everything related to command line parsing),
`my_huge_application_networking.cc` (for everything related to network I/O), and so on. As you may guess
_Open Header_ plugin will fail to find a corresponding header for that source files.

Starting from version 0.5 _C++ Helper Plugin_ can deal with both mentioned cases!
So, you don't need an _Open Header_ anymore! It is capable to "simple" toggle between header and source files,
like _Open Header_ plugin did, but slightly smarter :-)

TBD more details


Clang based code completion
---------------------------

**NOTE** Code completion works in manual mode **only** (at least nowadays). So you have to press `Ctrl+Space`
to complete the code.

**NOTE** This is preliminary version without much of flexibility (a lot of hardcode).

**ATTENTION** Do not forget to add `-x c++` to clang options if you want to complete a C++ code.
(TODO: I think this option should be turned ON by default).

All configured paths (global and session) will be added as `-I` options to clang API by default.
Other options (like `-x c++`, `-std=c++11`, `-D` or other `-I`) can be added as well.
Completion has a toolview at bottom to display errors/warnings from clang. So if you expect some
completions, but don't see them, try to resolve that errors first. Most of the time it's about
unable to `#include` some header, that can be solved by adding one (or) more `-I` option to clang
configuration tab.

If you experience some latency, one may configure a PCH file to speedup completion.
For cmake based projects I've got a one helper macro to produce a PCH.h file:

    include(UpdatePCHFile)
    update_pch_header(
        PCH_FILE ${CMAKE_BINARY_DIR}/most_included_files.h
      )

this will add a target (`update-pch-header`) to produce the `most_included_files.h` header file
with `#include` directives of all used headers in a project. This file can be configured as PCH header
file in plugins' configuration dialog. It will be _precompiled_ and used by code completer.


Some (other) important notes
----------------------------

* monitoring too much (nested) directories, for example in `/usr/include` configured as
  system directory, may lead to high resources consumption, so `inotify_add_watch` would
  return a `ENOSPC` error (use `strace` to find out and/or check kate's console log for
  **strange** messages from `DirWatch`).
  So if your system short on resources just try to avoid live `#include` files status updates.
  Otherwise one may increase a number of available files/dirs watches by doing this::

        # echo 16384 >/proc/sys/fs/inotify/max_user_watches

  To make it permanent add the following to `/etc/sysctl.conf` or `/etc/sysctl.d/inotify.conf`
  (depending on system)::

        fs.inotify.max_user_watches = 16384


Update Config files
-------------------

Unfortunately after renaming from _Kate Include Helper_ all configured data (global and session)
will be lost (configuration groups were renamed as well). To avoid reconfigure everything one may use
`sed` to do the following:

    $ cd ~/.kde4/share/apps/kate/sessions
    $ sed -i 's,kateincludehelperplugin,katecpphelperplugin,g' *
    $ sed -i 's,:include-helper,:cpp-helper,g' *
    $ cd ~/.kde4/share/config
    $ sed -i 's,IncludeHelper,CppHelper,g' *


Known Bugs
==========

It seems recently released clang 3.2 has a bug with optional parameters: completer return only first one.


TODO
====

* Add autocompleter for `#include` files (done)
* Handle multiple matches (done)
* Passive popups if nothing found (done)
* Handle #include files w/ relative path
* Use `Shift+F10` to go back in stack (?)
* Form an `#include` directive w/ filename currently active in a clipboard (done)
* List of currently `#included` files in a dialog and/or menu (done)
* _OpenFile_ dialog for current `#include` line
* Is it possible to use annotations iface somehow to indicate 'not-found' `#include` file?
* Add quick open dialog -- like quick document switcher, but allows to find file to open
  based on configured include paths by partial name match...
* Add view to explore a tree of `#included` files
* Add option(s) to include/exclude files from completion list
* Issue a warning if /proc/sys/fs/inotify/max_user_watches is not high enough
* Use `KUrl` for files and dirs instead of `QStrings`
* Clean `std::enable_if` and `boost::enable_if` from return value and parameters
* Use compilation database if possible.
* Auto generate doxygen documentation for functions from definition -- just skeleton
  w/ parameters and return type.
* Enable code autocompletion (configurable by checkbox)... but how to deal w/ really heavy project?
  Now one of my current project took ~8sec to show completions (even with PCH) :-(
  So, definitely there should be possible to turn code autocompleter off.
* Need to introduce index database to lookup for declarations/definitions/references. It
  also can be used for code refactorings (like rename smth & etc.)

Changes
=======

Version 0.8.7
-------------

* tooltips over highlighted `#include` lines displaying a reson
* there is no ambiguity if user types `#in` it can be only `#include` directive
  (`#include_next` is a really rare case, and I wrote it about 5 times in my life),
  so automatic completer added for this case.
* speedup getting completions
* enable automatic completions popup when member accessed

Version 0.8.6
-------------

* insert kate templates when execute completion item, so user may see and edit parameters required
  for a function call.

Version 0.8.5
-------------

* add completion items highlighting
* add completion items sanitizer: some complex template types can be replaced with shorter equivalent.
  For example `std::basic_string<char>` to `std::string`. This feature help to reduce width of completion
  popup for STL types. Also some boost types (`boost::variant` and MPL sequences) are supported.
* completers registration refactored and bug with absent completers for C++ code from file templates
  is gone finally

Version 0.8
-----------

* added session `#include` sets. Now it is possible to save session's `#inclue` paths and reuse in other
  sessions, so configuring sessions will takes less time. Also one perdefined set shipped: Qt4 --
  based on `#include` paths detected when this plugin get compiled.
* added UI to ask `g++` or `clang++` compiler (if found in `PATH`) about predefined `#include` paths,
  and append them to _System Paths_ tab.
* other UI cleaned up a little as well
* display optional parameters in completions list

Version 0.7.1
-------------

* added cmake script to detect clang C API
* fix cmake configuration for unit tests building: check if `-DBUILD_TESTING` present
* fix a bug when parse incorrect `#include` directive

Version 0.6-0.7
---------------

* add clang based code completion
* rename this plugin from _Kate Include Helper_ to _Kate C++ Helper_ (cuz it became smth bigger, than
  just `#include` helper :-)

Version 0.5
-----------

* add an action to switch between header and implementation file, just like an official *Open Header*
  plugin but smarter ;-) See details above.

Version 0.4.5
-------------

* fix nasty bug w/ path remove from config dialog

Version 0.4.4
-------------

* if file going to open is inaccessible for writing, open it in RO mode, so implicit modifications
  (like TAB to space conversions or trailing spaces removal) wouldn't annoy on close

Version 0.4.3
-------------

* make directory monitoring optional and configured via plugin's *Other Settings* configuration page

Version 0.4.2
-------------

* watch configured directories for changes and update `#include` files status
* add support to create source tarball

Version 0.4.1
-------------

* open dialog w/ currently `#included` files, if unable to open a file under cursor
  or cursor not on a word at all
* remove duplicates from completion list: for out of source builds and if both, source
  and binary dirs are in the search list, it led to duplicates
