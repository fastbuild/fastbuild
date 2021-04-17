# FASTBuild

FASTBuild is a build system for Windows, OSX and Linux, supporting distributed compilation and object caching. It is used by many game developers, from small independent teams to some of the largest studios in the world.

FASTBuild's focus is on fast compile times in both full build and local iteration scenarios.

A large variety of compilers and target architectures are supported. More details, including hosted documentation and Binary downloads can
be found here: http://fastbuild.org

## Branch policy

**Patches will only be accepted into the "dev" branch.**

| Branch | Purpose |
| :----- | :----- |
| main   | Stable branch containing snapshot of latest release |
| dev    | Development branch for integration of pull requests |

## Contribution Guidelines

Improvements and bug fixes are gladly accepted. FASTBuild has been improved immensely by the contributions of many users. To help facilitate ease of integration, please:

**Constrain pull requests to individual changes where possible** - Simple changes are more easily integrated and tested. Pull requests with many changes, where there are issues with one change in particular, will delay the integration of all changes. Pull requests that change or add large amounts of functionality are harder to test and will take much longer to integrate, increasing the likelyhood of conflicts.

**Avoid refactoring and formatting changes mixed with functional changes** - Behavior altering changes (fixes, enhancements and new features) mixed with style changes or refactoring cleanup make changes more difficult to integrate. Please consider splitting these changes so they can more easily be individually integrated.

**Update and extend tests if appropriate** - There are a large set of unit and functional tests. Please update or add new tests if appropriate.

**Update documentation if appropriate** - For changes in behaviour, or addition of new features, please update the documentation.

**Adhere to the coding style** - Please keep variable/function naming, whitespace style and indentation (4 space tabs) consistent. Consistency helps keep the code maintainable.
