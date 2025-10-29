+++
title = "Quirks"
date = "2025-10-29T09:33:42-07:00"

#
# description is optional
#
# description = "An optional description for SEO. If not provided, an automatically created summary will be used."

tags = []
+++

Given FreeBSD isn’t Linux, you may be wondering what hidden gotchas we have to be worried about. The answer is some, but not too many.

FreeBSD’s base installation is very minimal, so our first port of call is to install Python.

```sh
root@inconnu?:~ # python3 --version
-sh: python3: not found
```

The biggest surprise is FreeBSD only supports up to version 3.12, despite 3.13 being a year old and 3.14 already generally available. Not the end of the world, but [there have been some decent performance improvements since](https://blog.miguelgrinberg.com/post/python-3-14-is-here-how-fast-is-it).

Versioning is the first of three Python quirks we’ll encounter.

Second: even after we install `python311`, we don’t have the `sqlite3` module. We have to install `py311-sqlite` for what is probably a very good reason I haven’t bothered to look up. Moving on, we run the following:

```sh
pkg install -y python311 py311-poetry py311-sqlite git
git clone https://github.com/tiltowait/inconnu.git
cd inconnu
poetry install
```

… and get a dozen screens of error messages. The relevant bit is here:

```sh
- Installing maturin (1.9.6)

  PEP517 build of a dependency failed

  Backend subprocess exited when trying to invoke build_wheel

      | Command '['/tmp/tmp2ylvbmcc/.venv/bin/python', '/usr/local/lib/python3.11/site-packages/pyproject_hooks/_in_process/_in_process.py', 'build_wheel', '/tmp/tmppb0r5_j7']' returned non-zero exit status 1.
      |
      | Python reports SOABI: cpython-311
      | Unsupported platform: 311
      | Rust not found, installing into a temporary directory
```

(The error message was so long, I sent an SOS to Claude to tell me what it wanted.)

We need Rust. This is our third and final Python quirk: several of our packages (like maturin above) have [native extensions](https://docs.python.org/3/extending/extending.html), which means they are written in compiled languages, not Python. ([Python is dog-slow compared to something like C.](https://programming-language-benchmarks.vercel.app/c-vs-python).)

In the Linux world, these packages tend to be precompiled for us, and installations are quick. Not so on FreeBSD; we get to compile everything ourselves.

```
Installing the current project: inconnu (8.4.1)
      318.51 real      1207.75 user        41.66 sys
```

Honestly, not too bad! 5.3 minutes for an install that usually takes 5.3 seconds is certainly a hit, but these builds get cached, so we don’t have to do it all the time.

So far, the quirks have been pretty minor. Next post, we’re going to prison.
