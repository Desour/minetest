# Policy for Upgrading Dependency Versions

We want to support a large range of platforms for building and running Minetest.
This also includes Linux distributions that don't ship the newest release versions
in their software packages.


## Guidelines for choosing minimal Dependency Versions

Note: If do not officially support a platform, it does not mean that it won't work
but that we do not try to ensure that it keeps working.

If you add a new dependency and need to decide what minimum version you should require,
apply the heuristics below, ordered by importance.

### Available for testing

If we can't test with a version with a reasonable amount of effort, e.g. it's too
old for the CI we use, we don't support that version.

### Supported by LTS

If a feature is available in a dependency's version that is shipped in the LTS
distributions, that feature can be used.

### Prefer minimum

Don't bump a version for the sake of bumping.

If there's an older version available that already supports all the features we
use, choose the older version.
