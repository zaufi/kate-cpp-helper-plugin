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

* Add autocompleter for ``#include`` files
* Handle multiple matches
* Passive popups if nothing found (done)
* Handle #include files w/ relative path
