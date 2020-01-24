CSCI-442 Project 1: UNIX Shell
==============================

For this project, you will implement a UNIX Shell (similar to shells
like ``bash``) in C or C++.  The shell serves as the user interface of
an operating system, and some shells provide additonal functionality,
such as scripting.

Learning Objectives:

* Understand the system call ABI to UNIX systems by writing a program
  which uses system calls such as ``fork``, ``execve``, and more.

* Become familiar with UNIX file descriptors, both associated to files
  on the filesystem and to pipes, by implementing a shell which
  supports pipelines, file input, file output, and file appending.

* Sharpen systems programming skills in C or C++ by working on a large
  project, and making use of external libraries such as the GNU
  Readline library.

Here are the important deadlines for you to know:

============= ==============================
 Deliverable             Due Date
============= ==============================
Deliverable 1 February 9, 2020, 11:59:59 PM
Deliverable 2 February 16, 2020, 11:59:59 PM
Deliverable 3 February 23, 2020, 11:59:59 PM
============= ==============================

As outlined in the course syllabus, you may extend any of these
deadlines using slip days. However, spending slip days on a single
deliverable does not push back the deadlines of the whole project. For
example, if you spend 4 slip days on Deliverable 1, you will need to
either rush to finish Deliverable 2 on time, or spend more slip days
on Deliverable 2. For this reason, it is recommended you try and stay
ahead and finish early during the first two deliverables.

General Requirements
--------------------

Using the starter code *is not a requirement of this project*. You are
free to discard any amount of the starter code, as long as your final
project conforms to the requirements outlined here.

General requirements for all deliverables:

* You are free to develop your code wherever you like, but the code
  you submit code must function on the ALAMODE machines. Note: this is
  *different* from ``isengard``. Please see the section at the bottom
  of this file if you are not familiar with remotely logging into
  these machines.

* Your code must be written in only C or C++. Do not extend using
  additional languages. The exception is for the Makefile for your
  project, you may write the bulid system for your C/C++ code using
  any available language on the ALAMODE machines.

* To compile your code, the grader should be able to ``cd`` into your
  repository and run ``make``. To run your code, the grader should be
  able to type ``make run-shell``.

* Your code cannot depend on any shells installed on the system, such
  as ``/bin/bash``, ``/bin/sh``, or ``/usr/bin/zsh``, except at build
  time. As a corollary to this, you may not use the ``system(3)`` or
  ``popen(3)`` library functions, as these depend on the availability
  of a system shell.

* Your code should not assume that system calls succeed. **Always,
  always, always, check the return code from system calls**, and print
  an error message where appropriate. The starter code provides an
  optional framework to assist with this.

* You should follow good code formatting and software engineering
  practices. This includes avoiding monolithic functions, freeing heap
  memory you allocate, writing comments where appropriate, and
  following a consistent and well recognized style guide. The starter
  code uses the Linux Kernel Style Guide: you are free to adopt this
  or reformat the code to another style guide of your choice, but you
  must remain consistent.

* Move this ``README`` file somewhere else and include a ``README``
  file of your own with all submissions. This ``README`` must document
  all of the following:

  - Your Name and CWID
  - The names of any students (or anyone else who is not the
    Instructor or TA) you collaborated with, and to what extent you
    collaborated with them. This is an individual project, so you
    should not be looking at others' code other than to give
    assistance with debugging.
  - The approximate number of hours spent on each deliverable. If you
    spend time working on an extra credit feature, report the hours
    but count it separately from the total.
  - (For the final deliverable), any extra credit features you have!

* In addition to the ``README``, please add an ``AUTHOR`` file at the
  base of the repository with just your Mines username in it.

* Submissions of your project will be handled using GitHub
  classroom. From Canvas, find the link to the GitHub classroom, where
  you will associate your GitHub account with your Mines account, and
  the site will create a private repository for your work.

Project Requirements
--------------------

The specific requirements, as well as the associated deliverable, are
outlined below. For each requirement, it should be prefixed with [D1],
[D2], or [D3] to indicate the corresponding deliverable it must be
working by.

The Prompt
~~~~~~~~~~

[D1] When your shell starts, it should prompt your user for a
command. [D1] Your program should run the command, wait for the
command to finish, and finally prompt your user for another
command. [D1] Your shell should continue repeating this until the user
types Ctrl-D at an empty prompt, or uses the ``exit`` builtin to exit,
in which case the shell should exit.

[D1] The prompt for input should be prefixed with a prompt string, as
outlined below:

* The prompt string should contain the username of the user running
  the shell. Do not hardcode the grader's name (nice try!).

* The prompt string should contain some sort of indicator as to
  whether the last command was successful, such as the numeric return
  code, an emoticon (such as ``:)`` or ``:(``), or coloring. You
  should document what the indicator is in your ``README`` file. The
  first prompt when the shell starts should show a successful
  indicator.

* The prompt string should end in ``$``, followed by a space.

An example prompt string, for my username, is below::

  jrosenth :) $

You are free to add additional features to the prompt (such as the
hostname or current working directory), or even the ablity to
customize the prompt string at runtime. If you do this, be sure to
document it in your ``README`` for extra credit!

[D1] The user should be able to use standard Readline (Emacs-style)
keybindings to edit the input. The easiest way to do this is to use
the Readline library to prompt for input.

[D1] Previously inputted commands should be kept track of, and the
user should be able to navigate their history using up and down
arrows, Ctrl-R, Ctrl-S, Ctrl-N, and Ctrl-P.

History Expansion
~~~~~~~~~~~~~~~~~

[D1] Your shell must support history expansion as follows:

* ``!!`` must refer to the previous command.
* ``!n`` (with a numeric argument ``n``) must refer to the nth command
  inputted.
* ``!-n`` (with a numeric argument ``n``) must refer to the nth
  command back from the current command.
* ``!prefix`` must refer to the last command starting with ``prefix``.
* ``!?search?`` must refer to the last command with ``search``
  anywhere in it.
* ``!#`` must refer to the current command.

[D1] In addition, you must support all of the following modifiers
appended to the above references:

* ``:$`` must refer to the last argument of the command.
* ``:^`` must refer to the first argument of the command which is not
  the command name.
* ``:n`` (with non-negative integer ``n``) must refer to the nth
  argument of the command.
* ``:s/find/replace/`` must do a find and replace on the expansion.
* ``:gs/find/replace/`` must do a global find and replace on the
  expansion.
* ``:p`` must print the expansion without running the command (as
  outlined below).

[D1] If an expansion occurs, you should print the expanded command
before running it, unless the ``:p`` modifier was added, in which case
you should print it but not run it.

[D1] If there is an error in expansion, print an error message and do
not run the command.

[D1] You should not print the command if no expansion occurred.

.. note:: The easiest way to support all of the above features is to
          use the GNU History library, a part of Readline. Using just
          a few function calls in GNU History, you get an
          implementation of all of the required features for history
          expansion. You can read the manual for the library by typing
          ``man 3 history``.

Builtin Commands
~~~~~~~~~~~~~~~~

*Builtin commands* are commands supported by the shell which do not
require running an external program.

[D1] For all bulitin commands, if the user provides an invalid input
(such as incorrect number of arguments, provides a non-existent file
or directory, etc.), your shell should print an approprite error
message on ``stderr`` and indicate the command failure status in the
prompt.

exit
^^^^

.. note:: The ``exit`` command is provided in the starter code as an
          example. If you ditch the starter code entirely, you'll need
          to write your own.

[D1] The ``exit`` command takes zero or one arguments. If zero, the shell
should exit successfully. If one argument is passed, it should be a
number indicating the exit code to exit with.

echo
^^^^

[D1] The ``echo`` command takes any amount of arguments and prints
each separated by a single space. The ``echo`` command should output a
newline character at the end of the line.

cd
^^

[D1] The ``cd`` command takes one argument, the directory to change
to, which can be a relative or absolute path.

Many shells offer a feature where if ``cd`` is called with no
arguments, ``cd`` will change to your home directory. You may offer
this in your shell for extra credit, if you wish.

pwd
^^^

[D1] ``pwd`` should print the current working directory, and takes no
arguments.

history
^^^^^^^

[D1] The history command should print each line that has been typed at the
prompt, along with the one-indexed number of the entry.

alias
^^^^^

[D1] Your shell should support command aliases. That is, when a known
alias is typed, the command it aliases to should be run instead of the
typed command. Aliases match and replace the first argument of a
command.

[D1] The ``alias`` command takes any amount of arguments, and each
argument should have the name of an alias, followed by an equals sign,
followed by the command to alias to.

For example, this will alias printit to echo, and bye to exit::

  jrosenth :) $ alias printit=echo bye=exit
  jrosenth :) $ printit hello world
  hello world
  jrosenth :) $ bye

[D1] When ``alias`` is called with no arguments, you should print each
alias known to the shell in ``name=value`` format, one per line.

unalias
^^^^^^^

[D1] ``unalias`` takes any amount of arguments, and removes the alias
defined for the argument if the argument name is known as an alias to
the shell.

Parameters
~~~~~~~~~~

[D2] Your shell should support two kinds of parameters (variables):

* Shell Local Parameters
* Environment Variables

[D2] If an argument starts with ``$``, it should first try to be
replaced by the corresponding shell local parameter. If none is found,
it should be replaced by the corresponding environment
variable. Finally, if there is no corresponding environment variable,
it should be replaced with an empty string.

[D2] To set a parameter, your shell should support the syntax
``NAME=value`` on its own line. For example::

  jrosenth :) $ KITTENS=cute
  jrosenth :) $ echo $KITTENS
  cute

[D2] When setting a parameter, if it exists in the enviroment, it
should be set there first. Otherwise, it should be set in the shell
local variables.

External Commands
~~~~~~~~~~~~~~~~~

[D2] When the user types a command which is not known as a builtin to
the shell, the shell should find the command in the ``PATH`` and
execute the command using ``fork(2)`` and ``execve(2)``. You may use
any of the ``exec*`` family of library functions (such as
``execvp(3)``) to help you find the command in the ``PATH`` before
executing it, if you wish.

[D2] The shell should wait on the external command finishing before
returning to the prompt. As an example, you should be able to type
``gedit``, the editor will open, and you won't get your shell prompt
again until the editor is closed. See ``man 2 wait`` for info on how
to do this.

[D2] Shell local variables should not be visible in the environment to
external commands. To test this, use the ``env`` command to print all
the environment variables.

Pipes
~~~~~

[D3] Your shell should be able to handle an arbitrary number of
commands piped together. For example::

  jrosenth :) $ command1 | command2
  jrosenth :) $ command1 arg1 arg2 | command2
  jrosenth :) $ command1 | command2 | command3 | command4

Pipes do not need to support a builtin command in the pipeline.

You may assume that pipes will be surrounded by spaces on both sides.

For an example of a real piped command, try this (which gives the
number of lines in ``include/error.h`` which contain the word
``void``)::

  jrosenth :) $ cat include/error.h | grep void | wc -l

Maybe try putting a long chain of cats together::

  jrosenth :) $ date | cat | cat | cat | cat | cat | cat | cat

For this command, you should get the current date (assuming your shell
handles pipes properly).

File Redirection
~~~~~~~~~~~~~~~~

[D3] Your code must handle file redirection using ``>`` (overwrite to
a file), ``>>`` (append to a file), or ``<`` (input from a file).

For example::

  $ command > file-to-write-or-overwrite.txt
  $ command >> file-to-append-to.txt
  $ command < file-to-get-input-from-as-stdin.txt

[D3] For ``>`` and ``>>``, you should create the file if it does not
exist.

[D3] You should support ``<`` at the beginning of a pipeline, and
``>>`` or ``>`` at the end of a pipeline.

[D3] Think carefully about the permissions you create files with. With
``open(2)``, you provide the value which gets paired with
``umask``. What number should you use for files then?

An Introduction to the Starter Code
-----------------------------------

The starter code provides some really useful functions for writing the
shell, as well as a Makefile, but does not dictate how you should
structure the code for your shell. This means you are going to have to
still create some of your own software design, make new files, etc. In
other words, this isn't a "fill in the functions and you'll have a
working project" sort of starter code.

If you are just opening the starter code and want to look for
somewhere to start, try this:

1.  Run ``make`` and observe the output directories and where the
    programs end up.
2.  Run ``make run-tests`` and observe the failing unit tests.
3.  Run ``make run-shell``. This will run the (unimplemented) main
    function of your shell.
4.  Run ``make debug-shell`` to see what it's like to start ``gdb`` on
    your shell. Try ``tui enable`` and stepping thru some code.
5.  Open the ``alias.c`` file and implement an alias system.
6.  Get the unit tests passing for aliases.
7.  Use the readline library to write the main input-execute-loop.
8.  Connect the parser (described below).
9.  Serialize the output of the parser to an ``argv`` list.
10. Go make the ``echo`` bulitin command.
11. Finish the rest of D1.

.. note:: You are not graded on unit tests for this project. These are
          for your own convenience only.

Error Handling
~~~~~~~~~~~~~~

The starter code provides two simple macros for error handling. To use them,
``#include "error.h"``:

* ``RAISE(enum error_type t, ...)``: Raise an error of type
  ``t``. Optionally, pass a printf format string after that and and
  any relevant parameters.

* ``GET_ERROR(struct error *e)``: The first time this is called on a
  (stack allocated) error struct, it will return ``false``. Any time
  before ``exit_error_handler(struct error *e)`` is called, if an
  error is raised, the program will jump back to the ``GET_ERROR`` and
  return ``true``.

Using ``reraise(struct error *e)`` on a raised error handler will exit
the error handler and pass it to the next handler up the stack.

For an example, see ``mains/lexview.c``.

Additionally, there are a few more convenience macros:

* ``CHECK(condition)``: Check that ``condition`` is true and return
  it. If false, raise ``ERROR_CHECK_FAILURE``.

* ``CHECKZ(condition)``: Check that ``condition`` is zero (or false)
  and return it. If non-zero, raise ``ERROR_CHECK_FAILURE``.

* ``CHECKP(condition)``: Check that ``condition`` is non-negative and
  return it. If negative, raise ``ERROR_CHECK_FAILURE``.

* ``TODO(...)``: Equivalent to ``RAISE(ERROR_NOT_IMPLEMENTED, ...)``.

Finally, there are checked versions of many system calls and library
functions provided. See ``error.h`` for more details.

Parser
~~~~~~

The starter code provides an advanced input parser. There is only one
function to call::

  struct ast_statement_list *parse_input(const char *input);

Pass the input string and it will allocate memory for a parse tree and
return the corresponding statement list. You should free this
statement list using ``ast_statement_list_free`` once finished.

The parse tree produced by the parser is rather complete. Try running
``make run-parseview`` and typing some commands to view the tree. If
you pair what this syntax tree contains to what are the requirements,
you'll see you can make the following assumptions if you wish:

* There is only one statement in the list, or the statement list is
  ``NULL``.
* You don't have to handle backgrounding statements (unless you want
  extra credit).
* There may be many things in the pipeline for D3, but start out D1
  assuming there's only a single command in the pipeline.
* You only have to handle either assignments or an arglist in the
  command, not both.
* You can ignore the ``input_file``, ``output_file``, and
  ``append_file`` until D3.
* You may assume each argument consists of only a single part.
* You only have to support parameter or string arguments, not
  substitution or globbing.

If you start working on removing these assumptions and adding extra
features, you can get loads of extra credit, pretty easily.

Builtins
~~~~~~~~

There is a little framework provided to make it easier to declare
builtin commands. You are free to use it, or make your own. To see an
example, implementation, see ``src/builtins/exit.c``.

Write functions of the following type::

  int func(struct interpreter_state *state, const char *const argv[],
           int input_fd, int output_fd, int error_fd);

To register this function as a builtin command, use the
``DEFINE_BUILTIN_COMMAND`` macro, like so::

  DEFINE_BUILTIN_COMMAND("commandname", func);

You can then look up these functions in your core interpreter logic
using ``builtin_command_get``.

Arena Allocator
~~~~~~~~~~~~~~~

There is an arena allocator provided for you in the starter code. If
you aren't familiar with the concept of an arena allocator, it's a
fast way to allocate memory, keep track of it all, and free anything
which was allocated in the arena when you are done. An example is
avalible in the ``parseview`` program, or read on.

1. Stack allocate a ``struct arena``, making sure to initialize to
   zeroes::

     struct arena my_arena = { NULL };

2. Do any allocations using ``arena_malloc``, repeating as many times
   as you need::

     char *foo = arena_malloc(&my_arena, sizeof(char), 12);
     ...

   There are also alternative allocation functions such as
   ``arena_calloc`` and ``arena_strdup``.

3. When you are finished with everything you allocated in the arena,
   call ``arena_free(&my_arena)``.

I often use this as "passing an intent to allocate" by passing a
``struct arena *`` to a function. The caller then calls ``arena_free``
after using the function and its results.

String Builder
~~~~~~~~~~~~~~

The string builder provided under ``string_builder.h`` (and
implemented in ``src/lib/string_builder.c``) uses an arena allocator
under the hood to collect up a string and continually append to it.

To use it, see ``string_builder.h``.

Unit Testing Library
~~~~~~~~~~~~~~~~~~~~

If you wish to write unit tests, include ``unit.h`` and write tests
anywhere, like so::

    DEFTEST("some.test.name")
    {
            ...
            ASSERT(condition);
            ...
    }

The provided macros you can use in tests are:

* ``ASSERT(condition)``
* ``EXPECT(condition)``
* ``ASSERT_NULL(ptr)``
* ``EXPECT_NULL(ptr)``
* ``ASSERT_NOT_NULL(ptr)``
* ``EXPECT_NOT_NULL(ptr)``
* ``ASSERT_RAISES(error_type, expr)``
* ``EXPECT_RAISES(error_type, expr)``

The ``ASSERT`` family of macros will stop execution of the test if it
fails, whereas the ``EXPECT`` family of macros will still cause the
test to fail, but will continue to run the test. The ``EXPECT`` family
of macros will return ``true`` if the assertion succeeds, so you can
guard a part of the test from running if the expectation fails.

To run the tests, type ``make run-tests``.

.. note:: Writing (or using) unit tests is **not** a requirement, only
          a suggestion to help with your software practice. We will
          not run the unit tests during grading.

Adding C++ to the starter code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To write your code in C++, simply write files under ``src`` or
``mains`` with the ``.cc`` extension. The Makefile will detect these
automatically and compile and link using ``g++``. You should put your
C++ headers under ``cxxinclude``.

Improving the starter code
~~~~~~~~~~~~~~~~~~~~~~~~~~

If you find a bug or make an improvement to the starter code, please
submit a pull request on GitHub! You can do that here:

https://github.com/jackrosenthal/shell-starter-code

Submitting Your Work
--------------------

To submit your work:

1. Test your code on the ALAMODE machines, in case you were developing
   at home.

2. Make sure all code for your submission is committed.

3. Double check that step 2 is *actually* complete.

4. Tag your deliverable using Git::

      $ git tag d1-submission

   (for later deliverables, substitute ``1`` for the deliverable
   number)

5. Type::

      $ git push origin --tags

If you are spending slip days, do not forget to submit the form on
Canvas for slip days as well.

Grading
-------

* Deliverable 1 is 50% of the grade.
* Deliverable 2 is 25% of the grade.
* Deliverable 3 is 25% of the grade.
* You can earn **up to** 25% extra credit for any extra credit
  features you submit with Deliverable 3. The amount of extra credit
  granted is based on the grader's judgement of the amount of work put
  in to support the extra features.

Additional Resources
--------------------

* Don't forget the man pages! System functions are under ``man 2``,
  and library functions under ``man 3``.

* The book from the reading (Advanced Programming in the UNIX
  Environment) is available in the ALAMODE Lab. It is an excellent
  resource for this project.

* For GNU Readline and GNU History, see ``man 3 readline`` and
  ``man 3 history``.

* Please attend office hours if you find yourself falling behind.

Logging into the ALAMODE Machines Remotely
------------------------------------------

If you wish to develop code from home, you can remotely access the
ALAMODE machines using SSH.

1. If you are off-campus, first SSH into ``imagine.mines.edu``,
   replacing my username with yours::

     $ ssh jrosenth@imagine.mines.edu
     ... type your password

   You may skip this step from on-campus or from the VPN.

2. Then, SSH into ``bb136-XX.mines.edu``, where ``XX`` is a number
   from ``01`` to ``24``. Note: occasionally some machines are
   down; if the first machine fails, please try another.
