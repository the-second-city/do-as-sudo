+++
title = "Security, Practicality, and Convenience"
date = "2025-12-14T14:13:14-08:00"

#
# description is optional
#
# description = "An optional description for SEO. If not provided, an automatically created summary will be used."

tags = []
+++

The most secure system in the world is powered down, sealed in a lead box, and resting on the bottom of the ocean. Unfortunately, it’s also completely unusable.

The most convenient system is sitting on the open internet, accepting any and all connections without any authentication required. Unfortunately, it was pwned before I started writing this post.

Since this blog is about system administration, we’ll focus on SSH.

**Note:** This isn’t a tutorial, so I won’t be covering specific `/etc/sshd_config` options, etc. Instead, I’m going to focus on the why and what I consider minimal best-practices.

## Authentication

Security and convenience are often in contention with one another. *Often*, not *always*. SSH defaults to password login, but typing your password every time is annoying! We can meaningfully increase security while *also* reducing our friction by using [public key authentication](https://www.ssh.com/academy/ssh/public-key-authentication).

### The naïve approach

```
ssh-keygen -t ed25519
Generating public/private ed25519 key pair.
Enter file in which to save the key (/home/tiltowait/.ssh/id_ed25519):
Enter passphrase for "/home/tiltowait/.ssh/id_ed25519" (empty for no passphrase):
Enter same passphrase again:
Your identification has been saved in /home/tiltowait/.ssh/id_ed25519
Your public key has been saved in /home/tiltowait/.ssh/id_ed25519.pub
The key fingerprint is:
SHA256:LOJkFW7E2L87sVji/lcWmzwfV/H4EycKrc6HR/BKwXs tiltowait@tsc
The key's randomart image is:
+--[ED25519 256]--+
|     +o          |
|    .oo.       . |
|      +. . .   .o|
|     o .. = o o.+|
|    + . S. O = o+|
|   + ...+ + E ..o|
|    .. + B B o o.|
|      o + * o .  |
|     ....o o     |
+----[SHA256]-----+
```

You can’t tell from the above, but this key kind of sucks, because I didn’t give it a password. It sucks, because [malware can steal your SSH key](https://www.bleepingcomputer.com/news/security/new-ssh-snake-malware-steals-ssh-keys-to-spread-across-the-network/), as well as your `~/.ssh/config`, and then trivially pwn your system(s).

Even if we're smart and never download any malware, it's vulnerable to the "unattended laptop" attack[^unattended].

### A password-protected key

In [the three factors of authentication](https://rublon.com/blog/what-are-the-three-authentication-factors/), a passwordless SSH key is a single factor: something you have. We can add a second factor by making it a password-encrypted key[^two-factor].

But wait—now we’re back to entering a password every time we sign in! We’ve meaningfully increased security (required “something you have” in addition to “something you know”), but we haven’t reduced friction. Because of this, we’re likely to choose a crappy password that can be quickly brute-forced offline if someone swipes our key.

### Resident keys to the rescue

The upgrade over file-based keys is to use hardware-based keys, such as Yubikey, Solokey, Titan, etc. These keys live entirely on a small piece of hardware you connect to your system and cannot ever be extracted[^extraction][^vulnerability]. When you authenticate to your SSH server, you'll be prompted to confirm:

```
Confirm user presence for key ED25519-SK SHA256:S6oTbQg9X8l+nQdiekWl8WRvu/36zKWOUg5RHMM/Wsc
```

This requires you to physically touch the key, at which point it will release the lock. You can (and should) increase your security posture more by requiring a PIN[^pin-lock]. This is still only 2FA: something you have (key) + something you know (PIN), but it's *significantly stronger* than the password-locked file, because the key isn't copyable.

### PINs are friction

The problem? PINs kind of suck. It's annoying to type 123456 every time you want to use your key. We can do better with the third traditional[^more-factors] factor: "something you are".

[Some keys support biometrics](https://www.yubico.com/products/yubikey-bio-series/). I personally use [Secretive](https://github.com/maxgoedjen/secretive) to use my Mac's TouchID for resident keys[^secretive]. Pressing your finger to a reader is nicer than typing some numbers.

### Backups

A hardware key provides strong security while reducing a lot of operational friction, but it adds significant risk: what happens if you lose your key?

Answer: You’re hosed. Unless you have backups.

In my personal[^personal] setup, I have the following:

1. TouchID-based resident key. If I lose this, I’ve also lost my laptop and have bigger problems.
2. PIN-protected Yubikey locked in a safe[^safe-friction].
3. Password-protected file-based SSH key printed on paper, stored in the safe, and deleted from my system (keeping only the pubkey). I’ve memorized the password. This is the last resort in case the Yubikey breaks.

This is, IMO, the sweet spot for me and for most people: you have redundancy, all friction is front-loaded, and everyday use is convenient enough that you won’t be tempted to compromise.

## Authorization

So far, we’ve covered authentication: proving *who* you are. But what about authorization: proving you’re allowed to do `$whatever`? Since we’re approaching from the perspective of a solo operator, briefly:

* It’s bad to log in as root
* It’s bad to be *able* to log in as root

Instead, use `doas` or `sudo` and set up your policies based on user groups. Again, solo operator POV, so I’m not going to cover all the options available to you, save to say that they’re extensive.

The big question is whether to add the friction of requiring your user password every time you `doas foo`. You might think I’m going to say yes, but you’d be wrong.

**Requiring the password only protects against the unattended laptop attack.** If someone is able to remote into your user shell, then you’ve been pwned, even if they can’t immediately run commands as root. They can trivially set up a key logger, e.g. `alias doas=“tee /tmp/doas-passwd | doas”`, then simply wait.

**There is, however, one real benefit:** operational security. If you enter the wrong command, the password prompt gives you a couple of seconds to realize it before doing something catastrophic:

```
/ $ doas rm -f *
Password:
```

> Wait, that’s the wrong directory ...

## Hard as Stone

Can you harden further? Absolutely. You could set up WireGuard, you could tune `pf` like crazy, you could lock connections down to specific IPs, etc. You quickly hit diminishing returns, however, and each additional safeguard you add increases your risk of accidentally locking out, and then you’re back to diving to the bottom of the Marianas Trench.

[^unattended]: You shouldn't need me to define this.

[^two-factor]: There's [a lot of debate](https://security.stackexchange.com/questions/220385/is-an-ssh-key-with-a-passphrase-a-2fa) over whether this is multi-factor or still single-factor. I'm giving my personal opinion based on practical usage/the user's perspective rather than intellectual purity.

[^extraction]: Some allow you to extract a "stub" key for use in `~/.ssh/config`, but importantly, this is *not* the actual key; you still need the hardware connected.

[^vulnerability]: Assuming there's no [vulnerability](https://ninjalab.io/eucleak/) you can [exploit](https://ninjalab.io/a-side-journey-to-titan/).

[^pin-lock]: The key will lock itself after a few failed attempts, so don't forget it.

[^more-factors]: Some experts list more factors, such as "somewhere you are" and "something you do".

[^secretive]: This is extremely convenient … until I get a new laptop, at which point I'll have to update every service I auth into.

[^personal]: Arguably overboard.

[^safe-friction]: The friction here: any time I use a key on a new service, I must fetch the Yubikey from the safe and set it up on both devices.
