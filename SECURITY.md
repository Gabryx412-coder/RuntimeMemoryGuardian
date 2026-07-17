# Security Policy

Runtime Memory Guardian is a defensive security library. We take its own security, and the accuracy of its detection logic, seriously.

## Supported Versions

| Version | Supported |
|---------|-----------|
| 0.1.x   | ✅ |

Once RMG reaches 1.0, this table will track the latest major version plus the previous one.

## Reporting a Vulnerability

**Please do not report security vulnerabilities through public GitHub issues.**

Instead, please report them via GitHub's private vulnerability reporting feature (Security tab → "Report a vulnerability") on this repository, or by emailing the maintainers at the address listed in the repository's GitHub profile.

Please include:

- A description of the vulnerability and its potential impact.
- Steps to reproduce (a minimal example is greatly appreciated).
- The RMG version, OS, and compiler used.

## What Counts as a Security Issue Here

Given RMG's purpose, we're especially interested in reports of:

- Memory-safety issues in the library itself (buffer overreads/overwrites, use-after-free) — particularly in the SHA-256/CRC32 implementations and the binary baseline (de)serialization code, which parse untrusted/external input.
- Logic errors that would cause RMG to **fail to report** a genuine integrity violation or hook (a false negative), since this directly undermines the library's purpose.
- Privilege-escalation or sandbox-escape vectors introduced by using RMG's process-attachment APIs.

False positives (RMG flagging benign memory as tampered) are usually not security issues and can be reported as regular bugs.

## Response Process

We aim to acknowledge reports within 5 business days and to provide an initial assessment within 14 days. Coordinated disclosure timelines will be agreed upon with the reporter on a case-by-case basis.