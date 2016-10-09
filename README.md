#FASTBuild

##Branch policy

**Patches will only be accepted into the "dev" branch.**

| Branch | Purpose | Windows CI | Static Analysis (WIP*) |
| :----- | :----- | :----- | :----- |
| master | Stable branch containing snapshot of latest release | [![Build status](https://ci.appveyor.com/api/projects/status/yqgusnykxs383oa6?svg=true)](https://ci.appveyor.com/project/ffulin/fastbuild) | |
| dev    | Development branch for integration of pull requests | [![Build status](https://ci.appveyor.com/api/projects/status/yqgusnykxs383oa6/branch/dev?svg=true)](https://ci.appveyor.com/project/ffulin/fastbuild/branch/dev) | [![Quality Gate](https://sonarqube.com/api/badges/gate?key=fastbuild)](https://sonarqube.com/dashboard/index/fastbuild) |
\* Static Analysis is being configured, so this quality gate is not indicative

## Change Integration

The canonical repo for FASTBuild is in perforce. Patches accepted into "dev" will be integrated into this Perforce depot.
When a new version is released, the stable (master) branch will be updated with a snapshot of the new released version.

"dev" -> Perforce -> "master"
