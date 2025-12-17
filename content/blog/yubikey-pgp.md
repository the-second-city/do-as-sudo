+++
title = "Fixing Yubico’s PGP Documentation"
date = "2025-12-16T23:07:45-08:00"

#
# description is optional
#
# description = "An optional description for SEO. If not provided, an automatically created summary will be used."

tags = []
+++

If you have a Yubikey, reading [Yubico’s PGP documentation](https://support.yubico.com/s/article/Using-Your-YubiKey-with-OpenPGP) is an excellent way to drive yourself up a wall.

The docs have several problems, from not explaining things to confusing phrasing to not following best practices to *being just flat-out wrong*. Make no mistake: **if you follow their documentation to the letter, your setup will not work properly.** So, in the hopes it spares at least *one* person the pain I just endured, I’ve written this post.

## Versions

I used the following software and hardware versions while writing this post:

* macOS Tahoe 26.1
* gpg 2.4.8 (installed via [Homebrew](https://brew.sh))
* Yubikey 5C Nano, FW 5.4.3

## Primary key creation

We’ll start with a new GPG key. To create it:

```sh
gpg --full-generate-key
```

After running, you will see output similar to the following:

```
gpg (GnuPG) 2.4.8; Copyright (C) 2025 g10 Code GmbH
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Please select what kind of key you want:
   (1) RSA and RSA
   (2) DSA and Elgamal
   (3) DSA (sign only)
   (4) RSA (sign only)
   (7) DSA (set your own capabilities)
   (8) RSA (set your own capabilities)
   (9) ECC (sign and encrypt) *default*
  (10) ECC (sign only)
  (11) ECC (set your own capabilities)
  (13) Existing key
  (14) Existing key from card
Your selection?
```

### Key type

Answer the following questions:

**Do you need to support old (>10-years-old) installations?** If yes, then select *(1) RSA and RSA*, then specify a key and sub-key size of *4096*. Continue to the next section.

**Is your Yubikey [>=5.2.3](https://developers.yubico.com/PGP/YubiKey_5.2.3_Enhancements_to_OpenPGP_3.4.html)?** If yes, then select *(9) ECC (sign and encrypt)*[^key-type]. Then, select *(1) Curve 25519*. ECC does not have variable key sizes.

### Wrap-up primary key creation

From here, choose an expiration date (*0 = key does not expire* is usually the most convenient), enter your name, email address, and optionally a comment, then review your information and enter `O` for *okay*.

You will then be prompted for a passphrase. Make it a good one that you can remember or put in a password manager. You (hopefully) know the drill.

Afterward, `gpg` will create your key and present you with its *fingerpint*:

```
gpg: revocation certificate stored as '/Users/tiltowait/.gnupg/openpgp-revocs.d/467B980BE6864919E9D04F0C892C17F9AF179A5A.rev'
public and secret key created and signed.

pub   ed25519 2025-12-17 [SC]
      467B980BE6864919E9D04F0C892C17F9AF179A5A
uid                      foo <foo@foo.com>
sub   cv25519 2025-12-17 [E]
```

Copy the fingerprint. If you somehow lose it, you can get it back with `gpg —list-keys`.

## Sub-key creation

We’ve created a *primary key*, but the Yubikey wants sub-keys. Three, to be exact. We’ll create those with an interactive menu. Enter it with:

```sh
gpg --edit-key <YOUR FINGERPRINT HERE>
```

You will see the following prompt:

```
gpg (GnuPG) 2.4.8; Copyright (C) 2025 g10 Code GmbH
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Secret key is available.

gpg: checking the trustdb
gpg: marginals needed: 3  completes needed: 1  trust model: pgp
gpg: depth: 0  valid:   3  signed:   0  trust: 0-, 0q, 0n, 0m, 0f, 3u
sec  ed25519/892C17F9AF179A5A
     created: 2025-12-17  expires: never       usage: SC
     trust: ultimate      validity: ultimate
ssb  cv25519/DCB5315B8CDE4B95
     created: 2025-12-17  expires: never       usage: E
[ultimate] (1). foo <foo@foo.com>

gpg>
```

### Signing sub-key

Type `addkey`, then select *(11) ECC (set your own capabilities)* if you created an ECC key, or *(8) RSA (set your own capabilities)* if you created an RSA key.

Next, you will be asked to toggle the capabilities for your sub-key. What’s enabled by default depends on the key type. Here’s what it looks like for ECC:

```
Possible actions for this ECC key: Sign Authenticate
Current allowed actions: Sign

   (S) Toggle the sign capability
   (A) Toggle the authenticate capability
   (Q) Finished
```

RSA has additional options. **We only want a signing key**[^typo], so manipulate the toggles until that’s the only capability enabled (on my system, ECC defaults to sign-only). Once done, enter `q` to continue. Then, depending on your key type:

* ECC: Select *(1) Curve 25519)
* RSA: Specify the key size (4096)

Then, set an expiration date, hit `y` twice, and enter your PIN. You should now see something like the following:

```
sec  ed25519/892C17F9AF179A5A
     created: 2025-12-17  expires: never       usage: SC
     trust: ultimate      validity: ultimate
ssb  cv25519/DCB5315B8CDE4B95
     created: 2025-12-17  expires: never       usage: E
ssb  ed25519/7D0D29897E626311
     created: 2025-12-17  expires: never       usage: S
[ultimate] (1). foo <foo@foo.com>
```

Some definitions:

* `sec`: Your primary key
* `ssb`: A sub-key
* `usage`: The key’s capabilities
  * `SC`: Sign + Certify
  * `E`: Encrypt (this was automatically created for us)
  * `S`: Sign (the key we just created)

### Authentication sub-key

The commands are basically the same here as before, so I’ll be brief. Enter `addkey` again and select the appropriate type as before. This time, you want to modify the toggles until *Authenticate* is enabled, then create the key as before.

Once done, your key should look like this:

```
sec  ed25519/892C17F9AF179A5A
     created: 2025-12-17  expires: never       usage: SC
     trust: ultimate      validity: ultimate
ssb  cv25519/DCB5315B8CDE4B95
     created: 2025-12-17  expires: never       usage: E
ssb  ed25519/7D0D29897E626311
     created: 2025-12-17  expires: never       usage: S
ssb  ed25519/DD0683D78AF97DC5
     created: 2025-12-17  expires: never       usage: A
[ultimate] (1). foo <foo@foo.com>
```

Finally, enter the following: `save`. This will save your changes and quit `gpg`.

## Backup

Before we continue, back up your private and public keys:

```sh
gpg --armor --export-key <YOUR FINGERPRINT HERE> > yk-25519-public.asc
gpg --armor --export-secret-key <YOUR FINGERPRINT HERE> > yk-25519-private.asc
```

You can call them whatever you want.

## Moving them to the Yubikey

Now’s the fun[^not-fun] part: we’re going to move our sub-keys (and *just* our sub-keys) to our Yubikey. Make sure it’s plugged in, then enter:

```sh
gpg --edit-key <YOUR FINGERPRINT HERE>
```

We’ll be back at our favorite prompt:

```
sec  ed25519/892C17F9AF179A5A
     created: 2025-12-17  expires: never       usage: SC
     trust: ultimate      validity: ultimate
ssb  cv25519/DCB5315B8CDE4B95
     created: 2025-12-17  expires: never       usage: E
ssb  ed25519/7D0D29897E626311
     created: 2025-12-17  expires: never       usage: S
ssb  ed25519/DD0683D78AF97DC5
     created: 2025-12-17  expires: never       usage: A
[ultimate] (1). foo <foo@foo.com>
```

We’ll use two different commands here: `key N` and `keytocard`. The former is non-obvious; while it looks like it’s just displaying the keys, *it is actually a toggle*. Typing `key N` toggles that key for whatever operation you plan on doing (deleting, moving to the Yubikey, etc.). You can tell which keys are toggled by looking for the asterisk, such as the following (`key 1` was issued):

```
sec  ed25519/892C17F9AF179A5A
     created: 2025-12-17  expires: never       usage: SC
     trust: ultimate      validity: ultimate
ssb* cv25519/DCB5315B8CDE4B95
     created: 2025-12-17  expires: never       usage: E
ssb  ed25519/7D0D29897E626311
     created: 2025-12-17  expires: never       usage: S
ssb  ed25519/DD0683D78AF97DC5
     created: 2025-12-17  expires: never       usage: A
```

**Because you can toggle multiple keys at once, it is critical to follow the directions below *exactly*.**

**Note:** Yubico’s directions for this part are ***completely wrong***. If you follow them, you *will* end up in a weird state. Follow my instructions instead.

### Encryption sub-key

Each of these sections follows the same pattern: select a key, move it to the appropriate slot on the Yubikey, deselect the key.

```
key 1
keytocard
(Select "Encryption key")
key 1
```

**Again, it is critical to deselect the key via `key 1` after running `keytocard`.**

### Signing sub-key

```
key 2
keytocard
(Select "Signing key")
key 2
```

### Authentication sub-key

```
key 3
keytocard
(Select "Authenticate key")
key 3
```

And that’s it. You can type `save`, which will save your changes and dump you to your shell.

## Verifying

To verify your keys, run:

```sh
gpg --card-status
```

You will see something like[^different]:

```
Signature key ....: C5BF E4F9 0D15 B020 1419  198B 94F9 9E25 4DC3 8D77
      created ....: 2025-12-17 03:33:24
Encryption key....: 8719 E819 31BB 49BA 93CD  7CAD CE2A D1D1 94F1 1187
      created ....: 2025-12-17 03:32:07
Authentication key: 0517 B39F 0D42 BC9B 04F8  419F 7666 9895 18F5 CBB4
      created ....: 2025-12-17 03:34:23
General key info..: sub  ed25519/94F99E254DC38D77 2025-12-17 tiltowait (yk-25519) <noreply@tiltowait.dev>
sec   ed25519/92C1DE4CCFF11D6A  created: 2025-12-17  expires: never
ssb>  cv25519/CE2AD1D194F11187  created: 2025-12-17  expires: never
                                card-no: 0006 20672150
ssb>  ed25519/94F99E254DC38D77  created: 2025-12-17  expires: never
                                card-no: 0006 20672150
ssb>  ed25519/7666989518F5CBB4  created: 2025-12-17  expires: never
                                card-no: 0006 20672150
```

The `ssb>` indicates that that sub-key lives on the Yubikey. **If any subkey is missing the `>`, you made a mistake!**[^start-over]

Now, let’s try to use our key.

### Signing

First, we’ll sign something:

```sh
echo "hello, world!" > hello.txt
gpg --sign hello.txt
gpg --verify hello.txt.gpg
```

You’ll need to enter your PIN, after which you should see something like:

```
gpg: using "404AD0B0E4FCD703E8F9D71592C1DE4CCFF11D6A" as default secret key for signing
gpg: Signature made Tue Dec 16 22:43:07 2025 PST
gpg: using EDDSA key C5BFE4F90D15B0201419198B94F99E254DC38D77
gpg: Good signature from "tiltowait (yk-25519) <noreply@tiltowait.dev>" [ultimate]
```

**Then, try it again:** this time, with the Yubikey removed. You should receive a prompt to insert your Yubikey.

### Encryption

Next, let’s test encryption.

```sh
gpg --encrypt --recipient <YOUR EMAIL> --armor hello.txt
gpg --decrypt hello.txt.asc
```

Enter your PIN if it makes you, as well as your passphrase (both depend on your system’s settings). If all goes well, you’ll see something like:

```
gpg: encrypted with cv25519 key, ID CE2AD1D194F11187, created 2025-12-17
      "tiltowait (yk-25519) <noreply@tiltowait.dev>"
gpg: using "404AD0B0E4FCD703E8F9D71592C1DE4CCFF11D6A" as default secret key for signing
hello, world!
```

## Cleaning up

Congratulations! You’ve got your Yubikey set up for PGP—hopefully *without* pulling out your hair. There’s just one thing left to do.

We have to delete our primary key.

I’ll explain why.

When we moved our keys to the Yubikey, we only moved the sub-keys we created[^primary]. We still have our primary key in software. The entire purpose of using a hardware key is to create non-extractable keys that require a piece of hardware to use. If we keep the primary key, we’ve just wasted our time, because *the primary key can sign*.

Worse, *the primary key can create and revoke sub-keys*. If someone exfiltrates our primary key, they can pretend to be us. No good. This is why we backed up the private and public keys earlier[^did-you-back-up].

So, without further ado:

```sh
gpg --delete-secret-keys <FINGERPINT>
```

It will give you several (four, on my system) opportunities to back out, but go boldly and delete that thing. Worst-case, you can re-import it with `gpg —import yk-25519-private.asc`.

Now, if you run `gpg --card-status` again, the `sec` line will say `sec#`. The `#` means the key is not present—exactly what we want!

## Conclusion

If you’ve followed this to the end, you should have a working PGP setup on your Yubikey, plus a backup of your primary and public keys. Keep the public key. Store the private key in a safe. Go full paranoid and print it on paper[^small-keys].

Happy signing.

[^key-type]: Yubico blanket recommends RSA and RSA. RSA is *fine*. It works, but it’s a 1970s scheme vs a 2010s scheme. Ed25519 is usually faster, more secure (though neither is quantum-safe), and produces smaller keys (yes, this can matter).

[^typo]: Yubico’s documentation has a typo in its equivalent section and tells you to create an Authenticate key when it means Sign.

[^not-fun]: Not really.

[^different]: My signatures here differ than in previous examples, because this is real data and not the dummy key I wrote for the earlier steps.

[^start-over]: Worst-case, you have to start over, but you might be able to go back into `gpg --edit-key` and fix it.

[^primary]: Yubico's docs have you send the primary key to the card, but this is a nonstandard practice that confers no benefit. `gpg` will throw warnings at you if you try, though it *will* let you do it.

[^did-you-back-up]: You *did* back up, didn't you?

[^small-keys]: I promised you the small size of ECC keys was an advantage.
