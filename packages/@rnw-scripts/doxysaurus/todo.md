TODO list for Doxysaurus
[ ] Create index template and generate index file from it
[ ] Add support for enums
[ ] Add support for templates and specializations
[ ] Research public-type, protected-type, and friend sections
[ ] Fix link substitution algorithm to avoid nested substitutions
[ ] Config related changes
    [ ] Map std:: operators
    [ ] Map member overrides
    [ ] Remove std:: class mapping from the config
[ ] Refactoring
    [ ] Simplify doxygen-model
    [ ] Add support for log file (no color, add time stamp)
    [ ] Split up the transform function into multiple small function
    [ ] Move MarkdownTransform to Transform
    [ ] Extract type declaration creation in a separate function
    [ ] Extract member declaration creation into a separate function
[ ] Add app Promise rejection handler.
[ ] Make sure that we handle only .h files.
[ ] Make output folder be project specific
[ ] Better algorithm for std:: operator mapping
[ ] Implement a map for member overloads