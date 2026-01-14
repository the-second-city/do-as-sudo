+++
title = "Password Hashing Shenanigans"
date = "2026-01-13T21:34:02-08:00"

#
# description is optional
#
# description = "An optional description for SEO. If not provided, an automatically created summary will be used."

tags = []
+++

As one does, I’ve been thinking about password hashing algorithms. For the uninitiated, a primer: properly designed authentication systems do not store your actual password. Instead, they calculate and store a [one-way hash](https://en.wikipedia.org/wiki/One-way_function). When you sign in, the system hashes your input and checks if the result is the same as the stored hash.

This may feel like extra steps, and it is ... but it’s for a good reason. By themselves, the hashes aren’t very useful. If a hacker manages to get ahold of all users’ hashes, then they still can’t log in, because there’s no way to derive the password from the hash.[^sneaky-admins]

Not all hashing functions are created equally, however. [Some are flawed and insecure.](https://en.wikipedia.org/wiki/MD5) [Some are so outmoded and easy to compute that a small GPU cluster could crack your password in hours.](https://en.wikipedia.org/wiki/Data_Encryption_Standard) And others, even if they are secure, [still aren’t great for passwords](https://en.wikipedia.org/wiki/SHA-2). The best algorithm these days is [Argon2id](https://en.wikipedia.org/wiki/Argon2). It’s secure, takes a lot of memory to calculate, and is slow. These are all important for a password hash, because it means it’s hard to brute-force.

## FreeBSD’s options

I was curious which algorithm FreeBSD uses. The answer lives in `login.conf(5)`—specifically, the `passwd_format` key. And the answer is ...

```
:passwd_format=sha512:\
```

... which was puzzling to me! The SHA family, while a good, fast, secure cryptographic hash, is not typically spoken of as a password hasher. After some digging into [crypt(3)](https://man.freebsd.org/cgi/man.cgi?crypt%283%29) and some poking around [passlib](https://passlib.readthedocs.io/en/stable/lib/passlib.hash.sha512_crypt.html), I learned there is in fact a `crypt` version of SHA-512!

Let’s take a look.[^deprecated]

```python
import crypt

salt = crypt.mksalt(crypt.METHOD_SHA512)
passwd = crypt.crypt("hiho", salt)
print(passwd)
```

Thanks to the random `salt`, running this program will be different every time it runs. When I ran it just now, I got the following hash:

```
$6$.OVfJthZYlbyV7QI$3lG7RxT/434Os4xngOKV4/Gel6.pv.xRaeAw74rASBW.FR/WoAaicWWmnTNm2S7rDVQTVCgLVaILS24cxvLjQ.
```

A fair bit of data is packed into this string:

* `$6$`: This identifies the hash method used. More on this later.
* `.OVfJthZYlbyV7QI`: This is our [salt](https://en.wikipedia.org/wiki/Salt_(cryptography)). (Dramatically) simplifying things, it’s a random value inserted into our real password so that the hash is unique no matter what our password is. This protects against [rainbow tables](https://en.wikipedia.org/wiki/Rainbow_table), which are tables of precomputed hashes.
* `$3lG7RxT/434Os4xngOKV4/Gel6.pv.xRaeAw74rASBW.FR/WoAaicWWmnTNm2S7rDVQTVCgLVaILS24cxvLjQ.`: This (minus the leading `$`) is our hash.

When you attempt to authenticate, the system looks up your user and grabs the hash. It then extracts the salt and method number and passes them along with your input to `crypt(3)`. If the resulting hash is the same as the stored hash, you’re in.

## Back to my surprise ...

SHA-512 isn’t bad; however, it’s not the best. Remember Argon2id? Well, FreeBSD doesn’t support that. Doesn’t support [scrypt](https://en.wikipedia.org/wiki/Scrypt), either. We get only five options, which incidentally are the "slots" I mentioned earlier:

1.   MD5
2.   Blowfish
3.   NT-Hash
4.   (unused)[^unused]
5.   SHA-256
6.   SHA-512

I say five options, but it’s really only two. [MD5 should never be used](https://www.aikido.dev/code-quality/rules/stop-using-md5-and-sha-1-modern-hashing-for-security). NT-Hash is for some odd Windows compatibility stuff most people don’t need. SHA-256 is a downgrade over the default.

That leaves [Blowfish](https://en.wikipedia.org/wiki/Bcrypt), which *is* more secure than SHA-512![^fips] And we can set `/etc/login.conf` to use it:

```
:passwd_format=blf:\
```

Then, we can run `doas cap_mkdb /etc/login.conf && passwd` and regenerate our password with the new algorithm. Finally, we can check the hash via `doas grep <username> /etc/master.passwd` and find ...

```
$2b$04$l.5CqTYDV1kiQ1aRQYx.uu3gLhSVoTRtSbC7e3Bm0N7l30KzDCsxm
```

**This isn’t right.** `$2b$` is indeed [the correct scheme](https://github.com/BcryptNet/bcrypt.net/blob/main/docs/docs/schemeVersions.md/), but we have a new value before the salt: `04`. This is the hash's “cost factor”—a value indicating how much work to perform when computing the hash.

`04` is very low. On a Xeon E3-1270v6, I can compute >1000 hashes/sec. A GPU cluster can do orders of magnitude more. Importantly, this is easier to compute than SHA-512! Even though Blowfish is supposed to be “better” (if still not ideal nor the most modern), this paltry cost factor prevents us from taking advantage of it.

Ideally, we’d want at least a cost factor of 12. Each addition of two to the cost quarters the speed at which we can hash. At cost factor 12, we’re down to ~4.4 hashes/sec. At 14, we only manage one per second. Past that leads to silly territory[^why-not].

The problem? Nothing in the manpage, nor my searches online, surface any way to set the cost factor. We’re stuck at `04`. Might as well use the default.

## But what if we force the matter?

Then again ... `/etc/master.passwd` is just a text file, right? What if we just pasted in a value with a higher cost factor?

**Warning:** You can lock yourself out of your account if you mess this up. Keep a second session open, just in case.

Let's modify our Python program above. Replace the `salt` calculation with this and compute as normal:

```python
# mksalt() accepts a rounds keyword. Blowfish cost factor
# is an exponent value, e.g. 2^4, but mksalt() wants the
# expanded value. For cost factor 16 (four-second hash),
# we therefore need to give 2^16, or 65536.
#
# Note that this number must always be a power of 2, so
# you can use 32768 but not 40000.
salt = crypt.mksalt(crypt.METHOD_BLOWFISH, rounds=65536)
```

Your result will be something quite unlike `$2b$16$UVR5WOYkP8xSiIiIGyaBHOEAsbA9HT2LmA16YwDJnzBN3M8IiTHpy`. What's important is that it starts with `$2b$16$`.

Then, we find our entry in `/etc/master.passwd` and paste this new hash over the old:

```
tiltowait:$2b$16$UVR5WOYkP8xSiIiIGyaBHOEAsbA9HT2LmA16YwDJnzBN3M8IiTHpy:1001:1001::0:0:User &:/home/tiltowait:/usr/local/bin/fish
```

Finally, we rebuild the password database and regenerate our hash:

```sh
doas pwd_mkdb -p /etc/master.passwd && passwd
```

And you're all set!

## But why?

Well, it *is* more secure. If someone gets their grubby mitts on your password database, then a cost factor of 16 will be *much* harder to crack if you have a halfway decent password.

The thing is, it's all theater. If someone grabs your password database, they already have root access to your system. They don't *need* your password. You've already been pwned.

What you *should* be using is key-based auth, not passwords. In fact, you don't even need a user password at all! Can't crack what doesn't exist.

There are, however, two arguments for this approach:

1. **Weak:** A high-cost root password is, maybe, possibly, a good thing. If someone somehow breaches your user account, slowing their `su` attempts has *some* utility.
2. **Stronger:** If you require a password for `doas`/`sudo`, then a long hash time gives you an opportunity to recognize you accidentally wrote `rm -rf /` and hit `^C` before the damage is done.

So, should you do this? Depends on how insane you think it is to manually edit the password database to accept an officially unsupported configuration.

[^sneaky-admins]: Hashes also protect against sneaky database admins.
[^deprecated]: Note: the `crypt` package [was removed in Python 3.13](https://docs.python.org/3.13/library/crypt.html).
[^fips]: FreeBSD likely defaults to SHA-512 because it is FIPS-compliant, and [Blowfish is not](https://docs.freebsd.org/en/books/handbook/security/#security-passwords).
[^unused]: Yes, slot 4 is nothing. Maybe we’ll get something more modern someday.
[^why-not]: Unless, of course, you *want* to wait 36 hours to log in at cost factor 31.

