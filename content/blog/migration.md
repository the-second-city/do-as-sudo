+++
title = "Migration"
date = "2025-10-26T10:09:55-07:00"

#
# description is optional
#
# description = "An optional description for SEO. If not provided, an automatically created summary will be used."

tags = []
+++

[Railway](https://railway.com) is a great service. I’ve been using it for ~2 years now. It’s got a clean interface, good performance, and possibly the best hosted Postgres prices in the business.

But it’s not without quirks. Railway instances share resources, such as compute time, memory, and IP. This is standard in the [PaaS industry](https://en.wikipedia.org/wiki/Platform_as_a_service), but for Discord bots, it kind of sucks.

Discord bans IPs of bots that violate the terms of service. On a service like Railway, you might share the same IP with multiple Discord bots. If one of them is a bad actor, then Discord kicks them *all* offline.

(Thankfully, this only happens a few times a year in practice, and fixing it is just a button click in the dashboard.)

More, while Railway is a good price, they still need to make money. But when you could run most Discord bots on a [Raspberry Pi](https://www.raspberrypi.com), why spend $35/mo?

We can do cheaper.

Though Railway wasn’t affected, [the October 2025 AWS outage](https://www.pcgamer.com/software/the-multi-billion-dollar-15-hour-aws-outage-that-brought-the-internet-to-its-knees-last-week-was-apparently-caused-by-a-single-software-bug/) had me missing the days of self-hosting. When I was a teen, I’d run *nix boxen in my bedroom, wrestle with WiFi, wrestle with ATI drivers, and marvel that Doom 3 ran natively.

That’s two reasons to switch. I don’t need a third. It’s time to go back.
