Kate C++ Helper Plugin
======================

This plugin intended to simplify the hard life of C/C++ programmers who use Kate to write code :-)

Features
--------

* code auto completion using [clang](http://clang.llvm.org)
* flexible cleanup rules to beautify completion results
* code indexing and navigation (no need to use `ctags` plugin anymore)
* flexible search facility allowing to execute various queries using multiple indices
* preprocessor directives auto completion w/ automatic substitution
* `#include` headers auto completion
* highlight `#include` lines if header not found in configured locations or found in multiple times
* `Edit->Copy #include to Clipboard` to add `#include <current-file.h>` to a clipboard
* `File->Open Header/Implementation` a replacement for official _Open Header_ plugin with few enhancements
* explore and navigate over included files
* monitor configured directories for header files

... and few more neat things. Hope you'll enjoy using it! ;-)

Requirements
------------

NOTE: Actually I don't know a real _minimum_ requirements (I'm a gentoo user and have everything fresh in
my system). So I call to users and distro-makers to provide some feedback, so I can update a list below.

* C++14 compatible compiler (gcc >= 4.9)
* [clang](http://clang.llvm.org) >= 3.3 or >=3.5 if you want to compile it with clang
* [cmake](http://cmake.org) >= 2.8.12
* [Kate](http://kate-editor.org) editor version >= 3.8
* [KDE](http://kde.org) >= 4.8
* [boost](http://boost.org) library >= 1.49 required since version 0.8.7
* [xapian](http://xapian.org) library >= 1.2.12 required since version 1.0

NOTE: Attention distro guys! GNU Autogen, searched by CMake script, do not needed by end users!
No need to add it to package dependencies! It is used (by me) in this particular project to generate
C++ files from skeletons -- i.e. it is _"this package developers only dependency"_!


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
* Handle `#include` files w/ relative and quoted paths (investigate the "problem")
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
* Enable code auto completion (configurable by checkbox)... but how to deal w/ really heavy project?
  Nowadays for one of my current project it took ~8sec to show completions (even with PCH) :-(
  <del>So, definitely there should be possible to turn code autocompleter off</del> [done]
* <del>Need to introduce index database to lookup for declarations/definitions/references. It
  also can be used for code refactorings (like rename smth & etc.)</del>
* Give a context hint to code completer
* Give a priority boost for code completion items depending on lexical/semantic context
* <del>Not quite related to C++, but it would be nice to have a CMake autocompleter.
  It can complete variables, functions, properties, `include()` or `find_package()` files.
  Also it can retrieve `help` screen for particular module.</del> [done in kate.git and KDE SC 4.11]
* Collect sanitizer stats (rule hits) -- it can help to understand what rules must be first in
  a configured list (to speedup completer + sanitizer).
* <del>Add a bunch of default sanitizer rules</del>
* <del>Add import/export sanitizer rules</del>
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
  or even maybe it would be reasonable to have a base dirctory name as a group name.
* Add search options
* Add more indexing threads and control their count
* <del>Add <em>Go to declaration/definition</em> to the context menu</del>
* Add _Go to semantic/lexical parent_ to context menu
* Do incremental reindexing on view change and/or inactivity timeout
* Add a way to analyze and show to a user what exceptions are possible at some particular
  point (function/method body/call) -- so user may add an appropriate `try`/`catch` block
* That (possibly) would be nice to have `#ifdef`/`#else ifdef`/`#else` blocks as inlined tabs
  inside a text buffer/view. So, depending on particular definitions the code (view) will
  activate a corresponding tab automatically (and still allows to switch to others and/or
  add some new branches).
* Turn `#incldue` lines into "links", if `Ctrl` key pressed
* <del>Avoid CSS files (if found) to be indexed as C/C++ sources! (and `*.cc.in` as well) </del>
* Disable "Save Current List" button on _Session Include Dirs_ page if current list is empty
  (to prevent overwriting some saved list occasionally). Introduce confirmation on overwrite?
* When autocomplete preprocessor, do not add a space if it is already here. Make autocompletion configurable?
* BUG: after sanitize refref has no space after parameter
* Strip '.' from ignore extensions list. Maybe it is better to have inclusion list instead, BTW?


See Also
========

* project [home page](http://zaufi.github.io/kate-cpp-helper-plugin.html)
* sources [repository](https://github.com/zaufi/kate-cpp-helper-plugin)
