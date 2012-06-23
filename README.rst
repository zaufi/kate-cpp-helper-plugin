.. contents::

==========================
Kate Include Helper Plugin
==========================

Information
===========

This plugin indendent to simplify the hard life of C/C++ programmers who use Kate to write code :-)
First of all, I tired to use the file browser to open mine (or system) header files. With this version
0.1 of the plugin one may press F10 to open a header file that has its name under cursor.
Actually, you are not even required to move a cursor to a file name if the current line starts with
``#include`` directive...

Requeriments
============

* `Kate <http://kate-editor.org  />`_ editor version >= 2.9.

Installation
============

* Clone sources into (some) working dir
* To install into your home directory you have to specify a prefix like this::

    cd <some-work-dir>
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=~/.kde4 ..
    make
    make install

* To make a system-wide installation, set the prefix to /usr and become a superuser to ``make install``
* After that u have to enable it from `Settings->Configure Kate...->Plugins` and configure the include path
  globally and/or per session...

TODO
====

* Add autocompleter for ``#include`` files (done)
* Handle multiple matches (done)
* Passive popups if nothing found (done)
* Handle #include files w/ relative path
* Use Shift+F10 to go back in stack
* Form an ``#include`` directive w/ filename currently active in a clipboard (done)
* List of currently ``#included`` files in a dialog and/or menu
* OpenFile dialog for current ``#include`` line
* Is it possible to use annotations iface somehow to indicate 'not-found' #include file?
* Add quick open dialog -- like quick document switcher, but allows to find file to open
  based on configured include paths by partial name match...

Changes
=======

Version 0.4.1

* workaround for undefined bug w/ KDE(4.8.3)+Qt(4.8.2): QListWidget::removeItemWidget()
