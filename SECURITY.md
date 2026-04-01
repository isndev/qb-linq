# Security policy

## Supported versions

Security fixes are applied to the **latest minor release** on the **`main`** branch. Older lines are not guaranteed to receive backports unless agreed with maintainers.

| Version | Supported          |
|---------|--------------------|
| 1.x     | Yes                |
| &lt; 1.0 | Best effort only |

## Reporting a vulnerability

**Please do not file a public GitHub issue** for undisclosed security problems.

Instead:

1. **Email or private channel** — Contact the maintainers of the **isndev/qb-linq** repository through a **private** method they publish on the GitHub org or repo (e.g. security advisory, maintainer email).  
2. **GitHub Security Advisories** — If enabled for the repo, use **“Report a vulnerability”** under the **Security** tab so only maintainers see the report.

Include:

- A **short description** of the issue and its impact.
- **Steps to reproduce** or a minimal proof of concept (if safe to share).
- **Affected versions** or commit hashes if known.

We will try to **acknowledge** receipt within a few business days and coordinate **fix and disclosure** timelines with you.

## Scope

**qb-linq** is a **header-only** template library. Typical concerns include:

- Iterator / lifetime misuse documented in API caveats.
- Dependencies pulled only for **tests** and **benchmarks** (GoogleTest, Google Benchmark via FetchContent), not for end users linking **`qb::linq`**.

## Safe harbour

We appreciate responsible disclosure. Please allow maintainers reasonable time to patch before public details.
