#FASTBuild

##Branch policy

| Branch | Purpose |
| :----- | :----- |
| master | Stable branch containing snapshot of latest release |
| dev    | Development branch for integration of pull requests |
 
Patches will only be accepted into the "dev" branch.**

## Change Integration

The canonical repo for FASTBuild is in perforce. Patches accepted into "dev" will be integrated into this Perforce depot.
When a new version is released, the stable (master) branch will be updated with a snapshot of the new released version.

"dev" -> Perforce -> "master"

##"dev" Branch State

You can sync the "dev" branch to get the latest in-development features.  The status of this branch is as follows:

| Branch | Windows CI | Static Analysis (WIP*) |
| :----- | :----- |  :----- |
| dev | [![Build](https://ci.appveyor.com/api/projects/status/yqgusnykxs383oa6?svg=true)](https://ci.appveyor.com/project/ffulin/fastbuild) | [![Quality Gate](https://sonarqube.com/api/badges/gate?key=fastbuild)](https://sonarqube.com/dashboard/index/fastbuild) |
\* Static Analysis is being configured, so this quality gate is not indicative
