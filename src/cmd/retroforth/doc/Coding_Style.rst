============
Coding Style
============


----------
Formatting
----------
* Use ASCII or UTF8
* Use 2 space indent, no tabs
* Use Unix-style line endings
* If a function is more than one line, start the code on the line
  following the name
* All functions should have stack comments
* Try not to exceed 80 characters per line
* Avoid trailing whitespace
* Closing semicolons should not be on a separate line


------
Naming
------
* Use short names for indexes
* Vocabulary names should end with an apostrophe
* Class names should begin with a period
* Constants should be in UPPERCASE
* Use camelCase with the initial word lowercase. Keep acronyms uppercase.
* Use of dotted notation (e.g., list.size) is acceptable for functions
  operating on a data structure


--------
Comments
--------
* Comments should be enclosed in parenthesis
* Keep comments roughly aligned within a local grouping
* Avoid superfluous comments
* All functions should have stack comments
* A space should preceed the closing parenthesis


--------------
Stack Comments
--------------
Stack comments in Retro are a compact form, using short codes
in place of actual words. These codes are listed in the next
section.

A typical comment for a function that takes two arguments and
leaves one will look like:

::

   ( xy-z )

In a few cases, functions may consume or leave a variable number
of arguments. In this case, we denote it like:

::

   ( n-n || n- )

There are two other modifiers in use. Some functions have different
compile-time and run-time stack use. We prefix the comment with
C: for compile-time, and R: for run-time actions.

If not specified, the stack comments are for runtime effects.
Functions with no C: are assumed to have no stack impact during
compilation.

+------------+------------------------------------+
| Code       | Used for                           |
+============+====================================+
| x, y, z, n | Generic numbers                    |
+------------+------------------------------------+
| q, r       | Quotient, Remainder (for division) |
+------------+------------------------------------+
| ``"``      | Function parses for a string       |
+------------+------------------------------------+
| q          | Quote                              |
+------------+------------------------------------+
| a          | Address                            |
+------------+------------------------------------+
| ``$``      | Zero-terminated string             |
+------------+------------------------------------+
| c          | ASCII character                    |
+------------+------------------------------------+
| f          | Flag                               |
+------------+------------------------------------+
| t          | Type constant                      |
+------------+------------------------------------+
| ...        | Variable number of values on stack |
+------------+------------------------------------+


-------------
Documentation
-------------
* Use reStructuredText for compatibility
* Try to keeps lines to a max of 80 characters
* Use the following section title adornment styles:

::

  ==============
  Document Title
  ==============

  ------------------------------------------
  Document Subtitle, or Major Division Title
  ------------------------------------------

  Section
  =======

  Subsection
  ----------

  Sub-Subsection
  ``````````````

  Sub-Sub-Subsection
  ..................

* Use two blank lines before each section/subsection/etc. title. One blank line
  is sufficient between immediately adjacent titles.
* Documentation should be released under ISC license or CC0
* For libraries: embedded documentation using **doc{** should be placed at the
  end of the file
