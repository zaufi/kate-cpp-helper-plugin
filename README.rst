.. contents::

==========================
Kate Include Helper Plugin
==========================

Information
===========

This plugin indendent to simplify a hard life of C/C++ programmers who use Kate to write code :-)
First of all, I tired to use file browser to open mine (or system) header files. With this version
0.1 of the plugin one may press F10 to open header file which name is under cursor. Or u don't even
required to move cursor to a file name if current line starts with #include directive...

Requeriments
============

* `Kate <http://kate-editor.org  />`_ editor version >= 2.9.

Installation
============

* Clone sources into (some) working dir
* To install into your home directory u have to specify prefix like this::

cd <some-work-dir>
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=~/.kde4 ..
make
make install

* To make a system-wide installation, set prefix to /usr and become a superuser to ``make install``
* After that u have to enable it from `Settings->Configure Kate...->Plugins` and configure include path
  global and/or per session...

TODO
====

* Add autocompleter for #include files
