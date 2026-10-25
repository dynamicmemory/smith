# Smith — Project Vision

## Purpose

Smith is a self-extensible, agentic development environment written in modern C++. 
It enables a human to work alongside AI agents that can autonomously develop 
software, delegate tasks, and improve the system that runs them.

## Core Requirements

1. Interface: A terminal-native environment, designed so a GUI or other clients 
can be added later without rewriting the core runtime.
2. Model Integration: Support locally hosted models and remote model providers 
through a replaceable, unified interface.
3. Agent Execution: Agents can receive objectives, inspect project files, write 
and modify code, execute commands, build software, run tests, debug failures, and iterate.
4. Multi-Agent Orchestration: Agents can create tasks, delegate work to other 
agents, and coordinate results under configurable permissions and resource limits.
5. Development Lifecycle: Agents can use Git to branch, commit, review, and 
integrate changes, with optional GitHub integration.
6. Self-Modification: Agents can modify Smith's own source code, build a new 
executable, test it, and prepare it for replacement.
7. Safe Handover: A separate supervisor manages executable versions, starts 
updated instances, verifies successful startup, transfers task state, and supports rollback.
8. Persistence: Projects, tasks, agent state, execution history, and results survive restarts.
9. Extensibility: Model providers, tools, interfaces, and execution backends 
can be replaced or extended through explicit interfaces.

## Development Principles

- Build the smallest useful system first; expand it through practical use.
- Own the core agent runtime and orchestration logic rather than adopting an 
existing agent framework.
- Use established infrastructure where reinventing it offers little value.
- Keep execution observable, testable, and permission-controlled.
- Design for concurrency and future distributed execution without prematurely 
implementing a distributed system.

## Initial Milestone

A single agent can connect to a model, accept a development task, inspect and modify 
a repository, execute build and test commands, diagnose failures, and present 
its changes for review.

Smith will use this capability to develop subsequent features, eventually including 
multi-agent orchestration and safe self-updating.
