==============================
Retro Language Quick Reference
==============================

-----------------
Displaying Things
-----------------

Numbers
=======

::

  100 putn


Characters
==========

::

  'a putc


Strings
=======

::

  "hello, world" puts


Mixed
=====

::

  1 2 3 "%d - %d = %d\n" puts


-----
Input
-----

Characters
==========

::

  getc

Strings
=======

Whitespace Delimited
--------------------

::

  getToken


Character Delimited
-------------------

::

  'c accept tib


Numbers
=======

::

  getToken toNumber


------------
Conditionals
------------

If / Then
=========

::

  1 2 = [ "true" ] [ "false" ] if
  1 2 = [ "true" ] ifTrue
  1 2 = [ "false" ] ifFalse


Multiple Comparisons
====================

::

  [ @base
    [  8 = ] [ "octal" ] whend
    [ 10 = ] [ "decimal" ] whend
    [ 16 = ] [ "hexadecimal" ] whend
    drop ( default case ) ] do

  [ @base
    [ 8 = ] [ "base is octal: %d" puts ] when
    [ 10 = ] [ "base is decimal: %d" puts ] when
    drop ( default case ) ] do


-------------------
Function Defintions
-------------------

Quotes (Anonymous)
==================

::

  [  ( code )  ]

Quotes can be nested.


Named
=====

::

  : name  ( stack comment )
    ( code ) ;

Quotes can be nested inside a named function.


-----
Loops
-----

Unconditional
=============

::

  repeat ( code ) again


Counted
=======

::

  ( simple, no index on stack )
  10 [ 'a putc ] times

  ( index on stack, counts up )
  10 [ putn ] iter

  ( index on stack, counts down )
  10 [ putn ] iterd


Conditional
===========

::

  10 [ 1- dup putn dup 0 <> ] while


------------
Vocabularies
------------

Creation
========

::

  chain: name'
    ... contents ...
  ;chain


Add To Search Order
===================

::

  with name'
  with| name' and' more' names' |


Remove From Search Order
========================

::

  ( remove the most recently added vocabulary )
  without

  ( remove all vocabularies )
  global


Access a Function in a Vocabulary
=================================

::

  ^vocabulary'function

