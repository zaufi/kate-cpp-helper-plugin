Changes
=======

Version 1.0.3
-------------

* improve `#include` helper to support local (i.e. in quotes) headers
* introduce preprocessor directives completions after `'#'` pressed
  at line start. Also it will substitute directive if user continue to
  type characters and there is no ambiguity -- yep, like it was before
  this release if user types `'#in'`.
* ATTENTION C++14 used since this version! So, you'd better to have
  GCC >= 4.9 or Clang >= 3.5

Version 1.0.2
-------------

* add import/export completions sanitizer rules
* provide sample, but quite useful, sanitizer rules
* keep current _include set_ selected after store it
* add action to step back after lookup declaration/definition
* set default shortcuts for all lookup action same as ctags plugin has.
* eliminate compile error w/ gcc 4.9. fix issue #18
* few cleanups in UI
* a bunch of improvements in build system (not interested to end-users ;-)

Version 1.0.1
-------------

* use KDE palette to colorize diagnostic messages and `#include` explorer
* add _Go to Declaration/Definition_ to context menu
* refactorings to control actions via `ui.rc` file
* few refactorings planned after 1.0.0 release


Version 1.0.0
-------------

* introduce source code indexer and full featured search facility. It allows to form
  various queries to the indexed collection and do flexible search. For example, to find
  all non templated public members-functions of `some` class, one may type:

        scope:some access:public kind:method NOT template:y
* extract doxygen comments for completion items and show it as a tip, when `Alt` modifier pressed
* use `KTextEditor::TemplateInterface2` when substitute completion item into a document
* improve completion for unsaved files
* A LOT of refactorings to improve stability/maintainability
* allow to move tabs in a tool view, save/restore tool view layout


Version 0.9.6
-------------

* fix for typo in completion results layout introduced in 0.9.5
* few internal refactorngs to simplify code (before next big feature ;-)


Version 0.9.5
-------------

* add (a little) option to control completions list layout
* translation unit and completion flags are reviewed (benchmarks still needed)
* fix LLVM/Clang detection for Debian
* add target to produce a `.deb` package (make `make kate-cpp-helper-plugin-deb-package`)
* close issue #10: no package w/ required headers in recent Debian/Ubuntu


Version 0.9.4
-------------

* close issue #8


Version 0.9.3
-------------

* close [issue #6](https://github.com/zaufi/kate-cpp-helper-plugin/issues/6): allow `RPATH`
  for LLVM directory. Possible this is gentoo specific issue, so further investigation needed
  for other distros... (feedback welcome).
* move proprocessor macros into a separate completion group.
* get rid of hardcoded completion item sanitizers -- now they can be configured via plugin
  settings dialog.
* configuration pages refactored, the new one has been introduced: _Clang Completion Settings_.
* a bunch of other internal refactorings.


Version 0.9.2
-------------

* fix (avoid actually) _pure virtual function_ crash with clang 3.3 on
  attempts to get a source code location for _Note_ diagnostic messages.
* diagnostic messages tab in the tool view was transformed into a (clickable) list view


Version 0.9.1
-------------

* add a quick search (filter) to the `#include` explorer
* fixed build w/ gcc 4.6.x (patch from Alexander Mezin)


Version 0.9
-----------

* add a list of file extesnsions to be ignored by #include autocompleter (per session)
* fix regression with header file presence checker


Version 0.8.8
-------------

* copy `#include` to clipboard improved (and recent bug has fixed)
* disable C++ specific actions for non C++ documents
* tree builder refactored in `#include` explorer, now everything is Ok w/ tree structure


Version 0.8.7
-------------

* tooltips over highlighted `#include` lines displaying a reason
* there is no ambiguity if user types `#in` -- it can be only `#include` directive
  (`#include_next` is a really rare case, and I wrote it about 5 times in my life),
  so automatic completer added for this case.
* speedup getting code completions
* enable automatic completions popup when member accessed
* `#include` explorer has been added. It can hihglight "redundand" headers included in a translation unit.


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


Version 0.4
-----------

* fixed a bug w/ reading a global config after the plugin gets enabled
* a bunch of refactorings since 0.3


Version 0.3
-----------

* check if #included files are really available in configured paths. If some doesn't,
  then mark such lines w/ an error background. In case of multiple matches, mark it w/
  a warning backgorund color.
* configuration dialog extended w/ new options


Version 0.2
-----------

* push `#include` directive w/ current file name into the clipboard, so it would be easy
  to switch to another document and just paste the whole `#include` line...
* finally it has auto-completer for path and file name for strings w/ `#include` on a line!
  Yeah, now it's really easy to type and find a required header...


Version 0.1
-----------

* open header files using F10 key
