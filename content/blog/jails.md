+++
title = "Jails"
date = "2025-11-06T21:39:43-08:00"

#
# description is optional
#
# description = "An optional description for SEO. If not provided, an automatically created summary will be used."

tags = []
+++

[FreeBSD jails](https://docs.freebsd.org/en/books/handbook/jails/) are lightweight, self-contained virtual hosts running on the host system’s kernel. They’re somewhat analogous to Docker containers, with the following differences:

* It’s FreeBSD all the way down. You can’t make a “Gentoo jail,” for instance.
* They’re not intended to be ephemeral like Docker tends to be (though nothing stops you from running them this way).
* Because they share the host’s kernel, they’re lighter-weight.
* If you use ZFS, you benefit from copy-on-write (COW), making jails extremely disk-efficient as well.

Though FreeBSD comes with native commands for creating and managing jails, I'm using [Bastille](https://bastille.readthedocs.io/en/latest/), which simplifies a lot of the work.

## Current setup

Here is my current jail setup:

```
JID  Name               Boot  Prio  State  Type   IP Address  Published Ports  Release          Tags
 102  blog               on    99    Up     thin   10.1.3.2    -                14.3-RELEASE-p5  web
 114  fc-inconnu         on    99    Up     thin   10.1.69.2   -                14.3-RELEASE-p5  svc
 104  gitea              on    99    Up     thin   10.1.3.3    -                14.3-RELEASE-p5  web
 110  inconnu            on    99    Up     thin   10.1.2.2    -                14.3-RELEASE-p5  bot
 25   nginx              on    99    Up     thin   10.1.1.5    -                14.3-RELEASE-p5  orc
 8    storage            on    99    Up     thin   127.0.0.2   -                14.3-RELEASE-p5  orc
 108  woodpecker-agent   on    99    Up     thin   10.1.1.3    -                14.3-RELEASE-p5  orc
 107  woodpecker-server  on    99    Up     thin   10.1.1.2    -                14.3-RELEASE-p5  orc
```

The `nginx` jail runs a reverse proxy and routes all incoming web requests to the other jails. [Inconnu](https://github.com/tiltowait/inconnu) communicates with the `fc-inconnu` jail, which runs [a small microservice](https://github.com/the-second-city/faceclaimer) for image processing. That microservice has read-write to a mounted directory in the `storage` jail, to which one of the `nginx` sites has readonly access.

Other jails are more experimental. I’ve got [Woodpecker](https://woodpecker-ci.org) running for CI/CD (verdict: I like it), [Gitea](https://about.gitea.com) as a mirror for my GitHub stuff, and this blog. The one you’re reading right now.

## A fresh start

Rather than bloviate about the intricacies of my hyper-specific-to-me jail setup, I think it will be more instructive to start with a fresh jail, and I’ve got just the project for it.

Today (that is, 11/6/25), the Swift team [announced a preview build for FreeBSD](https://forums.swift.org/t/swift-on-freebsd-preview/83064). This isn’t available in the ports tree yet, let alone `pkg`. If you extract the archive, it pushes thousands of files into `/usr`; uninstalling or even updating could be a massive pain[^1].

We don’t want to risk borking our system, so we’ll lock Swift up.

First, we create a jail:

```sh
# We give it any old IP address within our private network
$ doas bastille create -T swift 14.3-RELEASE 10.1.1.150
```

There are two types of jails: thin and thick. Thin jails, the default, are lighter-weight and share some directories (such as `/usr/bin`) with the host over nullfs (readonly). Because the Swift preview puts files into some of these shared directories, we instead need a thick jail, which has completely independent installations at the cost of using a lot more storage. Thus: the `-T` flag.

While this jail will have a simple setup, I want to show off one of Bastille’s features, [Bastillefiles](https://bastille.readthedocs.io/en/latest/chapters/template.html). These are very similar to Dockerfiles, just … for Bastille[^2]. Here’s ours:

```
PKG python3 sqlite3 libuuid curl zlib-ng

CMD curl https://download.swift.org/tmp-ci-nightly/development/freebsd-14_ci_latest.tar.gz -o swift.tar.gz
CMD tar xvzf swift.tar.gz -C /tmp/
CMD cp -R /tmp/usr /
CMD rm -rf /tmp/usr
CMD swift --version
```

Save it as `./swift/Bastillefile` *on the host system*.

Unlike Docker, Bastillefiles are instructions to apply to an *existing* jail, not instructions to *make* a jail. To apply ours:

```sh
$ doas bastille template swift ./swift  # You point it to the dir
```

Bastille then bootstraps the `pkg` database, installs Swift’s dependencies, downloads and installs the Swift preview, and runs `swift —version`. At the end, we are presented with (*drumroll*):

```
Swift version 6.3-dev (LLVM bff1370bd79c983, Swift 57cf4ce563f700b)
Target: x86_64-unknown-freebsd14.3
Build config: +assertions
```

Success! Now, we’ll console[^3] into it and write our first Swift program:

```
$ doas bastille console swift
# cat > hello.swift <<EOF
print("hello, world!")
EOF
# swift hello.swift
hello, world!
```

Simple enough. Let’s try compiling.

```
# swiftc hello.swift
error: link command failed with exit code 1 (use -v to see invocation)
ld-elf.so.1: Shared object "libxml2.so.2" not found, required by "ld"
clang: error: linker command failed with exit code 1 (use -v to see invocation)
```

Hmm, guess the Swift team forgot to bundle it. Let’s install it.

```
# pkg install -y libxml2
# swiftc hello.swift
error: link command failed with exit code 1 (use -v to see invocation)
ld-elf.so.1: Shared object "libxml2.so.2" not found, required by "ld"
clang: error: linker command failed with exit code 1 (use -v to see invocation)
```

Weird. What version *did* we get?

```
# pkg info -l libxml2 | grep '\.so'
        /usr/local/lib/libxml2.so.16
```

Aha. `swiftc` wants an older version of the library. Let’s try to force it:

```
# ln -s /usr/local/lib/libxml2.so.16 /usr/local/lib/libxml2.so.2
# swiftc hello.swift
error: link command failed with exit code 1 (use -v to see invocation)
ld-elf.so.1: /usr/local/lib/libxml2.so.2: version LIBXML2_2.4.30 required by /usr/bin/ld not defined
clang: error: linker command failed with exit code 1 (use -v to see invocation)
```

Maybe we can find `libxml2.so.2` online? I search for “freebsd libxml2.so.2” and find … [an open issue about this very topic](https://github.com/swiftlang/swift/issues/85348)! I don’t think we’re going to solve this one, folks.

It’s about this point when we start to feel pretty smart for running this preview in a jail.  Happily, we didn’t litter a bunch of buggy preview code[^4] into our host system's `/usr`. Until the preview is fixed:

```
$ doas bastille destroy -a swift
```

## Wrapping up

We barely scratched the surface of jails, but I hope that this was educational and at least a little interesting. I didn’t plan for the Swift preview to be so buggy, but its bugginess was in fact a feature. We got to see a non-contrived example of something jails are good for. Without jails, the Swift preview would be too risky to even use. With jails, all we’ve lost is a few minutes of our time.


[^1]: `/usr` is not where these files should really go. `man hier` tells us non-standard tools and libraries belong in `/usr/local`. No doubt this will be fixed before final release, making a jail an even better choice for this experiment.
[^2]: Duh.
[^3]: I recommend aliases for common commands like this, such as `bconsole` for `doas bastille console`.
[^4]: No shade to the Swift team. This kind of thing is expected!
