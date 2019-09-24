# NDK Release Process

Beginning with NDK r21, the NDK maintains both a frequent, "rolling" release
that happens on a roughly quarterly basis and a Long Term Support (LTS) release
that releases once a year (typically in Q3). For more information on each
release, see the [LTS Release] and [Rolling Release] sections below.

[LTS Release]: #lts-release
[Rolling Release]: #rolling-release

Note: The support period for the first LTS release (NDK r21) will be compressed
by about a quarter because we want typically hit Q3, but this process was not
put in place in time for us to release NDK r21 in time. NDK r21's support cycle
will begin with its release in late Q4 2019/early Q1 2020 and will last until Q3
2020 when the next LTS release is available.

Each NDK releases will first release to beta and betas will continue to be
released until the bar for promotion has been met, at which time we will ship it
as a release candidate. Release candidates will also be re-spun as needed until
the bar for release to stable has been met. Betas can be turned around as
quickly as we are able. The first RC will be given at least a month of soak time
to gather feedback. Soak time for subsequent RCs depends on the risk of the
fixes in each RC, but will be at least two weeks.

The bar for promotion between release channels depends on the release type. See
the [LTS release criteria] and [Rolling release criteria] sections below for
more information.

[LTS release criteria]: #lts-release-criteria
[Rolling release criteria]: #rolling-release-criteria

## Triage

These triage guidelines describe what bugs gate each release and what will be
considered for backports. Note that these are **not** strict rules. A difficult
or risky fix for a medium severity issue may not receive a backport, and a
trivial backport for a low severity issue might.

Note that features (with the exception of updating the sysroot for new Android
APIs in the LTS) will never be added in point releases.

### High severity issues

* Toolchain crashes or miscompiles with no workaround.
* Security vulnerabilities (either in the NDK itself or in generated code).
* Regression in compatibility with important third-party libraries (OpenSSL,
  cURL, etc).
* Severe performance regressions in generated code.

### Medium severity issues

* Toolchain crashes or miscompiles with a workaround.
* Source compatibility issues.
* Non-contrived, non-severe performance regressions.
* Workarounds for platform bugs.

### Low severity issues

* All other bugs.
* Toolchain driver issues. Any flags needed to make Android compiles work beyond
  `-target` fit into this category.
    * Note that these are not currently blocking, as we have not reached this
      state yet. Once clean, regressions will be considered blockers.

## LTS Release

These releases cater to users that prioritize stability over new features.

One release a year will be designated as the Long Term Support release. These
have a higher bar for stability. This means that we will spend more time fixing
bugs before promoting to the next stage of the release, and also that lower
severity bugs will trigger a point release than would for a rolling release.

The only features that will be backported to the LTS release are sysroot updates
for new Android APIs introduced in the year's OS update.

### LTS release criteria

The following table describes the release criteria for LTS releases. A respin
means that we've already released one release of that type and must release
another to meet our stability goals. We will not ship any type of release until
we have met this bar, but will respin as needed when bugs are reported against
the release.

For example, after we release beta 1 for an LTS release, any high or medium
severity bug, as well as any low severity regression will result in beta 2 being
released before we proceed to RC 1. Once RC 1 is released, any medium or high
severity bug will result in another RC being released before we proceed to
stable. **NB: This means that once we've entered the RC phase, fewer bugs will
be considered for respin. File your bugs as early as possible if you want them
fixed.**

Once we've released to stable, any newly filed medium or high severity bug will
result in a point release. Once begun, a point release may receive fixes for low
severity bugs on a case-by-case basis.

|                            |                          |
|----------------------------|--------------------------|
| Beta respin triggered by   | Low severity regressions |
| RC respin triggered by     | Medium severity          |
| Point release triggered by | Medium severity          |
| Point release accepts      | Low severity             |

## Rolling Release

### Rolling release criteria

The following table describes the release criteria for rolling releases. For a
description of how to interpret this table, see the description accompanying the
[LTS release criteria].

|                            |                 |
|----------------------------|-----------------|
| Beta respin triggered by   | Medium severity |
| RC respin triggered by     | High severity   |
| Point release triggered by | High severity   |
| Point release accepts      | Medium severity |
