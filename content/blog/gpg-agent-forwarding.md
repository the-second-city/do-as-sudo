+++
title = "GPG Agent Forwarding"
date = "2025-12-19T10:48:45-08:00"

#
# description is optional
#
# description = "An optional description for SEO. If not provided, an automatically created summary will be used."

tags = []
+++

I often do remote dev work on my server, and since I always sign my commits, I need GPG set up. After [the fiasco dealing with Yubico’s GPG documentation](https://blog.tiltowait.dev/fixing-yubicos-pgp-documentation/), I really wanted to get some use out of my hardware-based GPG key, so I decided to set up [GPG agent forwarding](https://wiki.gnupg.org/AgentForwarding).

That wiki page is a bit sparse, however, so let’s take a look at what we need.

## Security implications

### The risks

Before we continue, we need to discuss security. [Specifically, agent forwarding has security risks.](https://security.stackexchange.com/questions/256501/ssh-agent-forwarding-what-are-the-best-practices-and-current-security-issues)

In short: the root user can read your socket and steal your keys. (This is true for both SSH and GPG agents.) Also, if an attacker manages to sign in as *you*, they can also read your keys. Timing is of course a factor—you have to be signed in for the attack to work—but the risk is real.[^partial-mitigation]

**For myself**, as a solo operator with SSH-only auth, using resident keys, I do not believe this to be a significant attack vector. An attacker would need either an OpenSSH zero-day, or else to gain shell access to one of my jails *and then* exploit a kernel zero-day to escape containment. Neither scenario is impossible, but I accept the risk.

### So why do it?

But *why* accept the risks? Because there are benefits! The most obvious is I don’t need GPG keys sitting on my server. If I’m not signed in, the keys don’t exist. They never get backed up to a remote service, either. Another benefit is its less configuration, overall. If I need to revoke one of my subkeys, I don’t have to do it in a bunch of places, just one.

## Setup

That discussion out of the way, let’s look at setting it up[^jumping]. Thankfully, it turns out to be pretty easy.

Before continuing, my versions:

* macOS Tahoe 25.1
* FreeBSD 14.3-RELEASE
* gpg 2.4.8 on both hosts


**On your local machine** (the one doing the forwarding), you need your “extra” socket[^socket]:

```sh
# Local machine
gpgconf --list-dir agent-extra-socket
```

**On the remote server** (the receiver), you need the “regular” socket:

```sh
# Remote server
gpgconf --list-dir agent-socket
```

Then, on the **local machine**, edit `~/.ssh/config` and add the following line to your host entry:

```
RemoteForward <REMOTE AGENT PATH> <LOCAL EXTRA AGENT PATH>
```

If you haven't done it already, you’ll also need to set the `GPG_TTY` variable (on both systems) to the output from `tty`. In my `config.fish`, I have:

```sh
# Your shell config syntax will be different if you don't use fish
set -x GPG_TTY (tty)
```

**On the remote server**, you need to tell the GPG agent not to start and also kill it if it’s currently running. Edit `~/.gnupg/gpg-agent.conf` and add the line:

```
no-autostart
```

Then run:

```sh
# Remote server
gpgconf --kill gpg-agent
```

Almost done. **On the local machine**, restart your GPG agent:

```sh
# Local machine
gpgconf --kill gpg-agent
gpg-agent --daemon
```

We're on the last stretch, now. Install your public key onto the **remote server**. `scp` the file over, then run:

```sh
# Remote server
gpg --import pubkey.asc
```

*Finally*, add `StreamLocalBindUnlink yes`[^sesh] to `/etc/sshd_config` **on the remote server**, then reload (`doas service sshd reload`).

And … that’s it!

## Verifying

Remote into your **server** and run the following:

```sh
# Remote server
echo hi | gpg --clearsign
```

If all goes well, you will be prompted for your PIN. After authenticating, you should receive the signed message:

```
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

hi
-----BEGIN PGP SIGNATURE-----

iHUEARYKAB0WIQTFv+T5DRWwIBQZGYuU+Z4lTcONdwUCaUWblwAKCRCU+Z4lTcON
d/pNAQC2SkWg99Nyl71rY4XIuJiNw0AXz98nL/1MFb39n++WtQD+Jyu0bCqIPBtt
vS/2chEjgESofLPMD6syp8p3cIlBEAo=
=dVdr
-----END PGP SIGNATURE-----
```

## Thoughts

I’m happy both with how this experiment went and with how relatively straightforward it went. I’ve omitted some debugging and errors I encountered, but I was expecting much more frustration than I got. That’s a win.

One unavoidable aspect of agent forwarding is the time penalty: where signing on the remote server was instant, now it takes several seconds. This is a natural consequence of all the extra work being done: your data is shuttled over the SSH connection, signed locally, and then the signature is sent back to the remote. For now, I consider this an acceptable compromise. We’ll see how I feel in a month.

[^partial-mitigation]: A hardware key is only a partial mitigation. While it’s true that the key can't be stolen from your key, an attacker might still be able to sign as you while you’re connected. Whether they can depends on both your PIN settings and how strong your muscle memory to just type your PIN every time the dialog pops up is.

[^jumping]: Apologies for jumping back and forth between local and remote, but there's a bit of a chicken-and-egg problem. You're also getting the instructions more or less in the order I figured them out.

[^socket]: [More information on GPG sockets here.](https://unix.stackexchange.com/questions/605445/why-does-gpg-agent-create-several-sockets)

[^sesh]: Figuring this out was a bit of a rabbit hole. Without setting `StreamLocalBindUnlink yes`, forwarding will only work for a single session unless you delete the remote's `~/.gnupg/S.gpg-agent` socket. I initially wrote a logout script and a [comically optimized C program](/code/sesh.c) to determine if I was the last session and delete the file before realizing I needed to set this config option.
