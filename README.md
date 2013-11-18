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

* C++11 compatible compiler (gcc >= 4.8 recommended)
* [clang](http://clang.llvm.org) >= 3.3
* [cmake](http://cmake.org) >= 2.8
* [Kate](http://kate-editor.org) editor version >= 3.11
* [boost](http://boost.org) library >= 1.53 required since version 0.8.7
* [xapian](http://xapian.org) library >= 1.2.12 required since version 1.0

Installation
------------

* Clone sources into (some) working dir
* To install into your home directory you have to specify a prefix like this::

        $ cd <plugin-sources-dir>
        $ mkdir build && cd build
        $ cmake -DNO_DOXY_DOCS=ON -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=~/.kde4 .. && make && make install

* To make a system-wide installation, set the prefix to `/usr` and become a superuser to `make install`
* After that you have to enable it from `Settings->Configure Kate...->Plugins` and configure the include paths
  globally and/or per session...

Note: One may use `kde4-config` utility with option `--localprefix` or `--prefix` to get
user or system-wide prefix correspondingly.


Some Features in Details
========================

Open Header/Implementation: How it works
----------------------------------------

Kate shipped with a plugin named *Open Header*, but sooner after I started to use it I've found
few cases when it can't helps me. Nowadays I have 2 "real life" examples when it fails:

Often one may find a source tree with separate `${project}/src/` and `${project}/include` dirs.
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

**ATTENTION** In case of changing clang version used you have to recompile this plugin!

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


Completion Results Sanitizer
----------------------------

Since version 0.9.3 the plugin has configurable rules to sanitize completion results.
A rule consist of two parts: _find regex_ and _replace text_. The first one can be used
to match some part of a completion item and capture some pieces of text. Latter can be used in
_replace text_ part for text substitution. If a second part is empty, and the first is matched,
that completion item will be removed from a result list. This can be used to filter out undesired
items. For example, a lot of Boost libraries (especially Boost Preprocessor) has a bunch of internally 
used macros, which are definitely shouldn't appear to the end-user. To filter them, just add the 
following rule:

    BOOST_(PP_[A-Z_]+_(\d+|[A-Z])|.*_HPP(_INCLUDED)?$|[A-Z_]+_AUX_)

This rule remove `#include` guards and internal macros used by various libs (like PP, MPL and TTI).
Few other helpful rules can be found in unit tests 
[`sanitize_snippet_tester.cpp`](https://github.com/zaufi/kate-cpp-helper-plugin/blob/master/src/test/sanitize_snippet_tester.cpp).



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



Known Bugs
==========

* It seems recently released clang 3.2 has a bug with optional parameters: completer return only first one.
* clang 3.3 has a bug w/ _pure virtual function call_ when accessing some completion diagnostic messages.
  If you see a call to `clang_getSpellingLocation` in a backtrace, do not report a bug to me ;-)


TODO
====

* <del>Add autocompleter for `#include` files</del> [done]
* <del>Handle multiple matches</del> [done]
* <del>Passive popups if nothing found</del> [done]
* Handle #include files w/ relative path (investigate the "problem")
* Use `Shift+F10` to go back in stack (?)
* <del>Form an `#include` directive w/ filename currently active in a clipboard</del> [done]
* <del>List of currently `#included` files in a dialog and/or menu</del> [done]
* _OpenFile_ dialog for current `#include` line to choose (another) header
* <del>Is it possible to use annotations iface somehow to indicate 'not-found' `#include` file?</del> [done]
* Add quick open dialog -- like quick document switcher, but allows to find a file to open
  based on configured include paths by partial name match...
* <del>Add view to explore a tree of `#included` files</del> [done someway]
* <del>Add option(s) to include/exclude files from completion list</del> [exclusion list of extensions done]
* Issue a warning if `/proc/sys/fs/inotify/max_user_watches` is not high enough
* Use `KUrl` for files and dirs instead of `QStrings` [code review requried]
* <del>Clean `std::enable_if` and `boost::enable_if` from return value and parameters</del> [use sanitizers]
* Use compilation database if possible. [what to do w/ headers which are not in there?]
* Auto generate doxygen documentation for functions from definition -- just skeleton
  w/ parameters and return type. (maybe better to implement as Python plugin for kate?)
* Enable code autocompletion (configurable by checkbox)... but how to deal w/ really heavy project?
  Nowadays for one of my current project it took ~8sec to show completions (even with PCH) :-(
  <del>So, definitely there should be possible to turn code autocompleter off</del> [done]
* Need to introduce index database to lookup for declarations/definitions/references. It
  also can be used for code refactorings (like rename smth & etc.)
* Give a context hint to code completer
* Give a priority boost for code completion items depending on lexical/semantic context
* <del>Not quite related to C++, but it would be nice to have a CMake autocompleter.
  It can complete variables, functions, properties, `include()` or `find_package()` files.
  Also it can retrieve `help` screen for particular module.</del> [done in kate.git and KDE SC 4.11]
* Collect sanitizer stats (rule hits) -- it can help to understand what rules must be first in
  a configured list (to speedup completer + sanitizer).
* Add a bunch of default sanitizer rules
* Add import/export sanitizer rules
* Show some diagnostic from sanitizer
* Colorize and group diagnostic messages [partially done]
* <del>Add icons to completion types</del> [done for prefix-less layout]
* Get/show list of possible exceptions in particular function call
* Highlight interior of user specified `#ifdefs` (like `__linux__`, `__WIN32__`, etc) w/ a user specified color
* Try to get a location for completion item and show it <del>as suffix in a completion list</del>
  in the expandable part of completion item
* Add ptr/ref/const/etc to a type under cursor (by a hot-key). maybe better to implement as Python plugin for kate?
* <del>Show a real type of typedefs (as a tooltip?)</del> In symbol details pane
* Render class layout according sizeof/align of of all bases and members
* Provide Python bindings to indexing and C++ parsing, so they can be used from kate/pate plugins
* Group #include completion items by directory
* Sort #include directives according configurable rules and type (project specific, third party libs or system)
* <del>Upgrade plugin configuration (at least internal struct) to .kcfg</del> -- BAD IDEA!
  `kate` plugins can not use this feature cuz application class (plugins manager to be precise)
  do not designed for that...
* Unfortunately `KCompletion` can't be used to complete search query (cuz it is designed to complete
  only one, very first, term) -- it would be nice to have terms completer anyway...
* Index comments from source code
* Add terms for overloads
* Add terms for symbols' linkage kind
* Add value slot for offsetof(member)
* Add terms for arrays (and possible value slots)
* Add terms for variadic functions
* Add terms for function result type
* Add terms for calling conversion
* Handle attributes
* Add terms for 'noexcept'
* How to deal w/ 'redeclarations' of namespaces?
* Add terms for deprecated symbols (and platform availability)
* Find crash after some DB gets enabled after reindexing
* <del>Use actions w/ states (see KXMLGUIClient)</del>

See Also
========

* project [home page](http://zaufi.github.io/kate-cpp-helper-plugin.html)
* sources [repository](https://github.com/zaufi/kate-cpp-helper-plugin)
