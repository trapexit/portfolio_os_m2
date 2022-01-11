# 3DO M2 Portfolio OS v3.0

The 3DO Opera platform ran an OS called Portfolio.  Developed internally at
NTG/3DO by several of the same people who developed the operating system for
the Amiga computer, Portfolio takes many of its design cues from Amiga.
Indeed, Amiga programmers will find Portfolio quite familiar.  However,
Portfolio addresses many of the shortcomings levelled against Amiga, making
it quite advanced for the early 1990's, particularly when compared to
mystifyingly more popular operating systems such as MS-DOS.

Among other features, Portfolio provides:

  - Preemptive threading/multitasking,
  - User/supervisor separation
  - Memory protection (MMU/fences) and process separation,
  - Resource tracking -- memory and I/O resources are released when a
    process dies,
  - Asynchronous, high-level I/O system,
  - Shared libraries,
  - Dynamically loaded libraries and device drivers,
  - Message-based IPC,
  - Custom filesystem,
  - Extensible global error codes,
  - Input handling for joypads, joysticks, mice, and light gun.

Portfolio was created to ease development on the platform, but it also acted
as an abstraction to the hardware so that manufacturers had more flexibility
in their design and provide for compatibility with future systems. The OS
continued to evolve and was used for The 3DO Company's next console: The
[3DO M2](https://en.wikipedia.org/wiki/Panasonic_M2). The M2 was never
released to retail as a game console but was used for a few arcade games
and misc kiosks. The hardware is rare and the software perhaps rarer.

On 2022-01-10 a snapshot of Portfolio OS v3.0 from approximately 1996-12-14
was shared by YouTuber and M2 collector
[Video Game Esoterica](https://www.youtube.com/channel/UCn2pQB4jsCTLUtx2NIkCvUg).
He had acquired a collection of archives from a former M2 engineer some time
ago and thought he had shared them all with other members of the 3DO community.
He hadn't. After doing so it was quickly realized he had been sitting on the
source code to the M2 operating system as well as numerous tools.

## What is This Repo?

The initial checkin represents a snapshot of the Portfolio OS v3.0 source code
which was released by Video Game Esoterica on 2022-01-10.  This repo is, at the
very least, a GitHub hosted backup and reference, but may be a place for
future development.

## Links

- https://3dodev.com : 3DO Development Repo
- https://github.com/trapexit/portfolio_os : Portfolio OS for Opera
- https://discord.gg/kvM9cQG : The 3DO Community Discord
