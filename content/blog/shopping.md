+++
title = "Shopping"
date = "2025-10-28T09:10:56-07:00"

#
# description is optional
#
# description = "An optional description for SEO. If not provided, an automatically created summary will be used."

tags = []
+++

Before we can start our little adventure, we need to choose a host.

## Server types

First, a quick digression for the uninitiated. For this type of project, servers fall into three categories:

1. Dedicated server
2. VPS (dedicated vCPU)
3. VPS (shared vCPU)

### VPS (shared vCPU)

Shared-CPU virtual private servers (VPS) have the lowest starting cost (as low as $2/mo). As the name suggests, you share your compute time with other users. These can be a gamble. There’s always the risk of “noisy neighbors”—other users on the system who eat up a lot of CPU cycles, causing performance degradation on your end.

This sounds bad, but we’re hosting Discord bots. They spend most of their time waiting around. Unless our neighbors are throwing a block party, we’ll be fine. [Railway](https://railway.com), the bots’ current provider, uses shared vCPUs, and my only complaint has been [the IP issue](/blog/migration/). That problem isn’t inherent to VPSes, so we won’t count them out.

### VPS (dedicated CPU)

This is the next step up. A VPS with dedicated CPU(s) doesn’t have the noisy neighbor problem. You’re still sharing resources, however, so your VPS might have one dedicated vCPU and only 2GB of memory. They are also much more expensive than their shared vCPU cousins.

### Dedicated server

Also called “bare metal” in marketing materials, dedicated servers are yours and yours alone. No noisy neighbors. You get everything: all the CPUs, all the RAM, all the storage space. But they cost more. A lot more. Dedicated servers are firmly in the pipe dream category. I’d *love* to need a dedicated server.

## Hosting providers

Still with me? Now that we have an idea of the types of server available, we have to pick a host. And there are tons. I’ll just name a few:

1. AWS
2. GCP
3. Azure
4. Vultr
5. Linode
6. Hetzner
7. Cloudzy
8. OVH

### Big Cloud: AWS, GCP, and Azure

We’re trying to get *away* from Big Cloud, so I’ll just use GCP’s e2-standard-2 as a baseline: 2 dedicated vCPUs and 8GB RAM for $50/mo.

Surely we can do better.

### Vultr

Inconnu’s original host was [Vultr](https://vultr.com). It ran well on a $6/mo shared vCPU with 1GB RAM. These days, however, unless I’m willing to purchase multiple VPSes (I’m not), we have to go bigger. Unfortunately, they’re even more expensive than GCP! Pass.

![Vultr’s a good service provider, but a 2.8 config is $10/mo more than GCP](/images/vultr.png)

### Linode

[Linode](https://www.linode.com) [was purchased by Akamai in 2022](https://www.akamai.com/newsroom/press-release/akamai-completes-acquisition-of-linode), but we won’t hold that against them. We *will* hold their prices against them. Pass.

![I don’t want to pay $72/mo for 8GB of RAM](/images/linode.png)

### Hetzner

A European host, [Hetzner](https://www.hetzner.com) gets a lot of buzz. They have very low prices, offering two dedicated vCPUs and 8GB RAM for only $15/mo! Now, we’re talking.

![Hetzner is a dream too good to be true](/images/hetzner.png)

Oops: [they don’t offer FreeBSD images](https://docs.hetzner.com/robot/dedicated-server/operating-systems/standard-images/). You can use a custom image, but only on their bare metal line, which starts at $42/mo—more than we pay for Railway now.

A (reluctant) pass.

### Cloudzy

We’re running out of providers! Most VPS hosts only offer Linux, and I kept coming up empty-handed until I found [Cloudzy](https://cloudzy.com). Finally! Two dedicated vCPUs and 4GB RAM for only $15/mo. I’d have liked more RAM, but <pick your idiom about beggars and choosers, ports and storms, etc.>. I signed up.

And immediately hit obstacles. SSH kept disconnecting after about 30 seconds. Some staccato troubleshooting later, I discovered the kernel saw only 30MB of RAM, only 13MB was free, and there was no swap partition.

(I’d love to show you a screenshot of the issues, but I don’t want to undo my fixes and endure yet more 30-second connections, so use your imagination.)

The culprit: ZFS’s ARC was reserving too much memory. Putting a cap of 1GB fixed it, but now we were down to only 3GB, and that’s pretty dinky. We want room to grow.

### OVH to the rescue

Our last host is [OVH](https://us.ovhcloud.com). Not only are their prices competitive (8 shared vCPU + 24GB RAM for only $13/mo? Yes, please), they also offer FreeBSD. No ZFS, but that’s not the end of the world. Our prayers are answered. I provisioned one, and …

![OVH’s VPSes were a rowdy bunch](/images/ovh-vps.png)

Immediately met the neighbors. And boy, were they partying. We’re talking 30-second hangs during basic SSH tasks. These were consistent across multiple days, too. Discord penalizes you if you take more than three seconds to respond to a command invocation, and I don’t want to add `defer()` calls (they give you 15 minutes to reply, but at the cost of making every interaction noticeably slower).

Woe and despair. At this point, I’m close to giving up. Out of desperation, I take a look at their dedicated servers. Their VPSes are cheap, so maybe their bare metal is also a good deal?

![Shockingly, they are](/images/ovh-dedicated.png)

Surprise! Yeah, so long story short, we’re using a KS-5. It’s an older-generation Xeon, but we have four dedicated cores, hyperthreading, 450GB storage, and (since the base model wasn’t available) 64GB of RAM.

Some thoughts:

* This is comically overprovisioned
* We are only saving $5/mo
* The time cost of setting everything up obliterates that $5

But we’re doing it anyway, because it’s fun.
