+++
title = "The Mystery of the Disappearing Disk Space"
date = "2025-11-19T22:36:49-08:00"

#
# description is optional
#
# description = "An optional description for SEO. If not provided, an automatically created summary will be used."

tags = []
+++

Like many users, I have [fastfetch](https://github.com/fastfetch-cli/fastfetch) set to fire on login. Nothing too crazy: memory, swap, disk space, IPv4, and uptime. Plus a little [Beastie](https://en.wikipedia.org/wiki/BSD_Daemon). It looks like this:

```
/\,-'''''-,/\    Memory: 53.38 GiB / 63.65 GiB (84%)
\_)       (_/    Swap: 0 B / 1.00 GiB (0%)
|           |    Disk (/): 18.92 GiB / 380.10 GiB (5%) - zfs
|           |    Local IP (igb0): XX.XX.XXX.XX/24
 ;         ;     -------------
  '-_____-'      Uptime: 14 days, 21 hours, 28 mins
  ```
  
The most useful metrics are memory use and IP, and I have the others because I may as well.

Recently, I noticed something odd: my disk capacity was decreasing. I have a 450GB SSD. Week 1, `fastfetch` said 418GB. Last week, it said 396GB. This week, 380GB. Needless to say, I’m not *thrilled* to see a shrinking drive. Time to investigate!

```sh
$ df -h | head -n 2
Filesystem            Size    Used   Avail Capacity  Mounted on
zroot/ROOT/default    380G     21G    359G     6%    /
```

`df` agrees. Hmm. Let’s make sure I’m not just misremembering.

```sh
$ doas geom disk list
doas geom disk list
Geom name: nda0
Providers:
1. Name: nda0
   Mediasize: 450098159616 (419G)
   Sectorsize: 512
   Mode: r3w3e7
   descr: INTEL SSDPE2MX450G7
   ident: CVPF723300HN450RGN
   rotationrate: 0
   fwsectors: 0
   fwheads: 0

Geom name: nda1
Providers:
1. Name: nda1
   Mediasize: 450098159616 (419G)
   Sectorsize: 512
   Mode: r1w1e2
   descr: INTEL SSDPE2MX450G7
   ident: BTPF8256003U450RGN
   rotationrate: 0
   fwsectors: 0
   fwheads: 0
```

Okay, good. Drive manufacturers ~~lie~~ report space in SI units even though most operating systems[^1] report in base-2 units. 450 billion bytes is 419GB[^2], which is about what I remember.

So, my drives report the expected size, but my system doesn’t. Maybe there’s some corruption?

```sh
$ zpool status
  pool: zroot
 state: ONLINE
  scan: resilvered 5.41G in 00:00:15 with 0 errors on Wed Oct 22 22:48:54 2025
config:

        NAME        STATE     READ WRITE CKSUM
        zroot       ONLINE       0     0     0
          mirror-0  ONLINE       0     0     0
            nda0p4  ONLINE       0     0     0
            nda1p4  ONLINE       0     0     0

errors: No known data errors
```

Hey, that’s good. The [resilver](https://blocksandfiles.com/2022/06/20/resilvering/) was when I set up RAID1, so it’s nothing to be concerned about. So where’s my missing data?

```sh
$ zpool get size
NAME   PROPERTY  VALUE  SOURCE
zroot  size      418G   -
```

Oh. It’s there.

It turns out, this is a [common](https://forums.freebsd.org/threads/df-and-zpool-list-showing-disk-practically-full.87654/) [question](https://old.reddit.com/r/zfs/comments/c0aid1/zfs_list_is_showing_incorrect_size/) [among](https://serverfault.com/questions/1150499/df-reports-zfs-used-space-as-128k-not-the-actual-used-space) [ZFS](https://old.reddit.com/r/zfs/comments/hbryph/zfs_dataset_total_disk_space_displayed_in_df/) [users](https://github.com/openzfs/zfs/issues/3953). Your volumes and snapshots suck up capacity from your root.

In short, this is normal. But what about `fastfetch`?

[Here is the source for `fastfetch`’s BSD disk space calculation.](https://github.com/fastfetch-cli/fastfetch/blob/dev/src/detection/disk/disk_bsd.c) And [on line 168](https://github.com/fastfetch-cli/fastfetch/blob/645bf5c839ee954f368fe3176096c7f86618dabf/src/detection/disk/disk_bsd.c#L168) we see it’s doing the same disk space calculation `df` uses.

While that’s kind of *annoying*, it’s not the biggest deal in the world. As I work, the “total” capacity will gradually shrink, but it will eventually stabilize … somewhere.

So … that’s it. Short of patching `fastfetch`, we’re stuck with buggy capacity calculation on ZFS. Buggy capacity calculation, and …

```sh
Memory: 53.38 GiB / 63.65 GiB (84%)
```

… unexpectedly high memory usage?

## Another head-scratcher enters the fray

I’m running a Discord bot, a couple of nginx jails, a Go-based microservice, Postgres, and a partridge in a pear tree. The largest of those is the Discord bot, at 490MB. How are we using so much memory?

```sh
$ top -d1 | grep Mem
Mem: 347M Active, 8363M Inact, 7848K Laundry, 51G Wired, 912K Buf, 2880M Free
```

> 51G Wired

[According to the FreeBSD wiki](https://wiki.freebsd.org/Memory), the following comprise wired memory (emphasis mine):

* Non-pageable memory: cannot be freed until explicitly released by the owner
* Userland memory can be wired by mlock(2) (subject to system and per-user limits)
* Kernel memory allocators return wired memory
* **Contents of the ARC and the buffer cache are wired**
* Some memory is permanently wired and is never freed (e.g., the kernel file itself)

The ARC, or [Active Replacement Cache](https://openzfs.readthedocs.io/en/latest/performance-tuning.html), is a cache ZFS uses for frequently accessed files. Just how *much* RAM it uses depends on the operating system, but FreeBSD believes[^3] that unused RAM is wasted RAM, so it’s bound to use a lot. But how much, exactly?

```sh
$ sysctl kstat.zfs.misc.arcstats.size
kstat.zfs.misc.arcstats.size: 46874821736
```

46874821736 bytes is ~43.6GB. Or we could just ask `top`:

```sh
$ top -d1 | grep ARC
ARC: 44G Total, 34G MFU, 7122M MRU, 392K Anon, 376M Header, 2019M Other
```

Fully 90% of wired memory is dedicated to ZFS’s cache.

What’s important about ARC is ZFS will relinquish it if there’s [significant memory pressure](https://www.aeanet.org/what-is-memory-pressure/). In other words, though it’s wired memory, it’s definitely reclaimable. So is it really *unavailable*?

## Just what is free memory, anyway?

By the most technical definition, free memory is memory not claimed by any process. The trouble is, in systems like FreeBSD, this measurement can be misleading! `top` reports only 2.8GB free of 64GB. Sounds like I need a bigger server, right?

Well, no.

A better metric, in my opinion, is “available” memory: a combination of free and reclaimable memory. But neither `top` nor `fastfetch` report this!

## A more FreeBSD-accurate `fastfetch`

Using the `golang.org/x/sys/unix` package, we can probe `sysctl`. Here’s a function that does just that:

```go
func getMemory() (string, error) {
	pageSize, err := unix.SysctlUint32("hw.pagesize")
	if err != nil {
		return "", err
	}

	physMem, _ := unix.SysctlUint64("hw.realmem")
	arcSize, _ := unix.SysctlUint64("kstat.zfs.misc.arcstats.size")
	freePages, _ := unix.SysctlUint32("vm.stats.vm.v_free_count")

	freeBytes := uint64(freePages * pageSize)
	available := ByteSize(freeBytes + arcSize)
	used := ByteSize(physMem - (freeBytes + arcSize))
	physMemTotal := ByteSize(physMem)

	return fmt.Sprintf("%s / %.0fG (%s Avail)", used, physMemTotal.ToGB(), available), nil
}
```

When run, we see the following:

```sh
Memory: 17.3G / 64G (46.7G Avail)
```

Hey, that’s more like it! And much more in line with what I expected[^4].

## Fixing the rest

If we’ve got a memory probe, why not just … implement the rest? In the interest of shortening an already long post, [here’s the code](https://github.com/tiltowait/sysinfo). We’ve even got my beloved Beastie.

*For most users*, `fastfetch` is better. It’s ~~more~~ customizable, it’s got image support (if you like slower output), it’s got color schemes, etc.[^5] But if you’re an anal-retentive FreeBSD user who uses ZFS and also wants to see system stats on login, maybe you’ll find it useful.

[^1]: MacOS is a notable exception.
[^2]: No, I won’t put the *i* between the G and the B, [because it’s silly](https://xkcd.com/394/).
[^3]: Correctly.
[^4]: You could argue *inactive* memory is also “available.” I decided to keep it out; while inactive memory *can* be reclaimed, there can be performance consequences if the process decides it needs it again. Similarly, I excluded laundry, because laundry must be written to disk before it can be freed.
[^5]: Fittingly, `fastfetch` is also *faster*. By about 3ms. Because it doesn’t have to shell out to `zpool`. Though I tried, I wasn’t able to utilize libzfs directly.
