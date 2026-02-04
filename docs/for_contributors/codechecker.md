# Using CodeChecker
The CodeChecker is a tool for performing static code analysis and generating reports.
We provide scripts and custom configuration for running the CodeChecker.

We regularly run CodeChecker to check for any potential bugs or security issues. It is also
recommended to run the CodeChecker yourself on any code you plan to contribute, as it can discover
issues that other tools (compiler, ASAN, ...) can miss.

Note that the CodeChecker is supported on Linux and macOS only.

## Generating Reports
You need to install the following dependencies:

- CodeChecker
    - Check out the [official repository](https://github.com/Ericsson/codechecker) for guidance.
- Checkers for CodeChecker:
    - clang-tidy
    - clangsa
- jq (used by our script for merging JSON reports)

We generate reports from multiple projects (examples and tests) to cover as much CALs and HALs
as possible. The reports are then merged and exported to HTML.

To generate HTML report, you can use our convenience script. Reports will be
generated to `.codechecker/reports_html` in the Libtropic repository.

!!! example "Generating HTML report"
    === ":fontawesome-brands-linux: Linux"
        ```bash { .copy }
        # Run from root directory of the Libtropic repository.
        scripts/codechecker/run_checks.sh
        ```

    === ":fontawesome-brands-apple: macOS"
        TBA

??? note "Note: Running from a different directory"
    The script also supports running from a different directory, but you have to pass
    a path to the Libtropic repository as a first argument:

    !!! example "Generating HTML report from any directory"
        === ":fontawesome-brands-linux: Linux"
            ```bash { .copy }
            scripts/codechecker/run_checks.sh <path_to_repo>
            ```

        === ":fontawesome-brands-apple: macOS"
            TBA

If the script executes without any errors, exports will be ready and you can open
`.codechecker/reports_html/index.html` in your favourite web browser.

## Remarks
The current CodeChecker configuration is in YAML format, as it is more human-readable than JSON and also supports comments.  

The configuration file enables some strict checkers, which may produce a lot of warnings. It is recommended to run the analysis using the full configuration at least once. After that, you can manually disable any checkers you find unnecessary.