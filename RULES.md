# Smith — Development Rules

## Architecture

1. **Keep `main.cpp` minimal.** It handles application startup, top-level coordination 
and shutdown. Agent logic belongs in separate components.
2. **Separate responsibilities.** Model communication, tool execution, agent 
reasoning and runtime management must remain independently testable.
3. **Keep the core independent of the interface.** The TUI must be replaceable 
without rewriting the agent runtime.
4. **Prefer explicit interfaces.** Components should communicate through defined 
contracts rather than hidden shared state.
5. **Avoid premature abstraction.** Introduce interfaces and complexity when 
justified by actual requirements, not hypothetical future needs.

## Engineering

6. **Use modern C++20** and standard library facilities where appropriate.
7. **Enable compiler warnings.** New code must compile cleanly under the project's configured warnings.
8. **Test behaviour, not implementation details.** Components should be testable independently wherever practical.
9. **Handle errors explicitly.** Failures must be reported and propagated appropriately; do not silently ignore them.
10. **Keep dependencies deliberate.** Every external dependency must have a clear justification.

## Agent Safety

11. **Treat model output as untrusted input.** Never assume generated commands, paths or arguments are safe.
12. **Keep execution boundaries explicit.** File access and command execution must eventually be governed by configurable permissions.
13. **Protect the running executable.** Self-updates must build and validate a separate executable before any handover.
