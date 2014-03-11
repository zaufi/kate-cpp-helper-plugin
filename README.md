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

Since version 0.6 I've added a code completion based on clang API and decided to rename _Kate Include Helper_
into _Kate C++ Helper Plugin_.

Version 1.0 has a source code indexer and powerful search facility.


Requirements
------------

NOTE: Actually I don't know a real _minimum_ requirements (I'm a gentoo user and have everything fresh in
my system). So I call to users and distro-makers to provide some feedback, so I can update a list below.

* C++11 compatible compiler (gcc >= 4.8)
* [clang](http://clang.llvm.org) >= 3.3
* [cmake](http://cmake.org) >= 2.8
* [Kate](http://kate-editor.org) editor version >= 3.8
* [KDE](http://kde.org) >= 4.8
* [boost](http://boost.org) library >= 1.49 required since version 0.8.7
* [xapian](http://xapian.org) library >= 1.2.12 required since version 1.0

NOTE: Attention distro guys! GNU Autogen, searched by CMake script, needed only for new code development
and it is Ok to have it NOT-FOUND for binary package! So, no need to add it to dependencies ;-)


Installation
------------

* Clone sources into (some) working dir
* To install into your home directory you have to specify a prefix like this::

        $ cd <plugin-sources-dir>
        $ mkdir build && cd build
        $ cmake -DNO_DOXY_DOCS=ON -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=~/.kde4 .. && make && make install

* To make a system-wide installation, set the prefix to `/usr` and become a superuser to `make install`
* After that, you have to enable it from `Settings->Configure Kate...->Plugins`, configure the include paths
  globally and/or per session and other options. Some details could be found at [home page]

Note: One may use `kde4-config` utility with option `--localprefix` or `--prefix` to get
user or system-wide prefix correspondingly.



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
* Use `KUrl` for files and dirs instead of `QStrings` [code review required]
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
* Add ptr/ref/const/etc to a type under cursor (by a hot-key). maybe better to implement as a Python plugin for kate?
* <del>Show a real type of typedefs (as a tooltip?)</del> In symbol details pane
* Render class layout according sizeof/align of of all bases and members
* Provide Python bindings to indexing and C++ parsing, so they can be used from kate/pate plugins
* Group `#include` completion items by directory
* Sort `#include` directives according configurable rules and type (project specific, third party libs or system)
* <del>Upgrade plugin configuration (at least internal struct) to .kcfg</del> -- BAD IDEA!
  `kate` plugins can not use this feature cuz application class (plugins manager to be precise)
  do not designed for that... `xsltproc` still can be used :)
* Unfortunately `KCompletion` can't be used to complete search query (cuz it is designed to complete
  only one, very first, term) -- it would be nice to have terms completer anyway...
* Index comments from source code
* Add terms for overloads
* Add terms for symbols' linkage kind
* Add value slot for `offsetof(member)`
* Add terms for arrays (and possible value slots)
* Add terms for variadic functions
* Add terms for function result type
* Add terms for calling conversion
* Handle attributes
* Add terms for 'noexcept'
* How to deal w/ 'redeclarations' of namespaces?
* Add terms for deprecated symbols (and platform availability)
* <del>Find crash after some DB gets enabled after reindexing</del>
* <del>Use actions w/ states (see KXMLGUIClient)</del>
* <del>Normalize file names when add to the cache on indexing</del>
* Split `#include` completions into a __system__ and __session__? (controlled via config option)
* Add search options
* Add more indexing threads and control their count
* <del>Add <em>Go to declaration/definition</em> to the context menu</del>
* Add _Go to semantic/lexical parent_ to context menu
* Do incremental reindexing on view change and/or inactivity timeout


See Also
========

* project [home page](http://zaufi.github.io/kate-cpp-helper-plugin.html)
* sources [repository](https://github.com/zaufi/kate-cpp-helper-plugin)
