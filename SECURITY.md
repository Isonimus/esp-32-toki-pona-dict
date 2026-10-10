# Security Policy

This is an offline dictionary firmware: it has no network stack, stores no credentials, and
collects no personal data, so its security surface is small. Still, if you find a vulnerability —
for example a memory-safety bug reachable through the input path or the serial debug command — we
want to know.

## Reporting a vulnerability

**Please do not open a public issue for a security problem.** Report it privately through either:

- **GitHub private vulnerability reporting** — the "Report a vulnerability" button under this
  repository's **Security** tab (preferred), or
- a direct message to the maintainer, **[@Isonimus](https://github.com/Isonimus)**.

Please include what you found, how to reproduce it, and the affected board/firmware version. We aim
to acknowledge a report within a few days and will coordinate a fix and disclosure timeline with
you.

## Scope

In scope: the firmware in this repository (`core/`, `hal/`, `app/`, `main/`) and the build tooling
(`tools/`). Out of scope: the third-party libraries and fonts (report those upstream), and the
hardware itself.
