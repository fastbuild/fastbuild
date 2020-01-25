# FASTBuild

## Branch policy

| Branch | Purpose | Status |
| :----- | :----- | :----- |
| master | Stable branch containing snapshot of latest release | |
| dev    | Development branch for integration of pull requests | [![Build Status](https://travis-ci.com/fastbuild/fastbuild.svg?branch=dev)](https://travis-ci.com/fastbuild/fastbuild) |
 
Patches will only be accepted into the "dev" branch.**

## Change Integration

The canonical repo for FASTBuild is in perforce. Patches accepted into "dev" will be integrated into this Perforce depot.
When a new version is released, the stable (master) branch will be updated with a snapshot of the new released version.

"dev" -> Perforce -> "master"
