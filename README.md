#FASTBuild

##Branch State

| Branch | Windows CI | Static Analysis (WIP*) |
| :----- | :----- |  :----- |
| master | [![Build](https://ci.appveyor.com/api/projects/status/f5vewk6oqi7i5pi9?svg=true)](https://ci.appveyor.com/project/jairbubbles/fastbuild) | [![Quality Gate](https://sonarqube.com/api/badges/gate?key=fastbuild)](https://sonarqube.com/dashboard/index/fastbuild) |
\* Static Analysis is being configured, so this quality gate is not indicative

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
