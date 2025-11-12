<!--
Sync Impact Report
Version: 1.0.0 -> 1.0.1
Modified Principles:
- IV. Layered Observability & Safe Telemetry (clarified stdout/stderr containment)
Added Sections:
- None
Removed Sections:
- None
Templates Requiring Updates:
- ? .specify/templates/plan-template.md
- ? .specify/templates/spec-template.md
- ? .specify/templates/tasks-template.md
Follow-up TODOs:
- None (no command templates exist under .specify/templates/commands to audit)
-->

# Basic Rootkit Constitution

## Core Principles

### I. Controlled Research Deployment
All execution, debugging, and demonstration of the rootkit MUST occur inside an isolated, access-controlled lab with no direct link to production networks or personal data. Every spec MUST document the lab boundary, tooling whitelist, disposal procedure for captured traces, and the compliance reviewer who signs off before code leaves the lab. Rationale: maintaining ethical, legal, and reputational safety is non-negotiable for offensive-security research.

### II. Explicit Hook Contracts
Before coding, engineers MUST enumerate every API, structure, or callback being intercepted, describe the minimal instruction patch needed, define rollback steps, and highlight expected side effects. Hook contracts MUST live with the spec/plan artifacts and stay synchronized with the code. Rationale: precise contracts keep hooks stable across Windows releases and prevent undefined process behavior.

### III. Deterministic Build & Traceability
Build scripts, compiler flags, and dependency hashes MUST produce reproducible binaries. Each release MUST embed the git commit, semantic version, and hash of linked dependencies, and the resulting artifacts must be logged in an auditable build register. Rationale: deterministic builds allow independent verification that released DLLs match reviewed source.

### IV. Layered Observability & Safe Telemetry
Instrumentation MUST exist at each hook boundary: structured logs routed to the lab collector, optional in-memory counters for stealth checks, and a one-command kill switch to detach safely. Observability MUST never exfiltrate data outside the lab or interfere with the host process' stdout/stderr contracts. Rationale: disciplined telemetry shortens debug loops while avoiding user impact.

### V. Test-First Hardening
For every concealment feature, write contract tests (hook behaves as declared), regression tests (process/file remains hidden), and detection tests (ensure kill switch restores baseline) before implementation. CI MUST fail if any rootkit test suite is missing or flaky. Rationale: the only safe way to iterate on intrusive hooks is to treat tests as the primary safety net.

## Operational Constraints
- Supported toolchains: Visual Studio 2022 and CMake toolchains pinned to documented versions; deviations require governance approval.
- Approved dependencies: only the bundled IAT Hook library plus Windows SDK components; third-party binaries require a supply-chain review and checksum capture.
- Artifact retention: lab build outputs and telemetry may be stored for up to 30 days for analysis, after which they MUST be securely destroyed.
- Secrets handling: credentials for test VMs stay in encrypted vaults; never hard-code secrets or store them in version control.

## Development Workflow & Reviews
- Feature specs MUST include lab boundary details, hook contracts, observability plan, and deterministic build notes before `/speckit.plan` runs.
- Implementation plans MUST pass the Constitution Check gates (lab readiness, contracts, reproducible build, observability, and test-first strategy) prior to Phase 0.
- Code reviews MUST verify that tests were written and run first, telemetry stays inside the lab network, and build logs include the current semantic version + commit hash.
- Tasks documents MUST map at least one task per user story to lab setup/cleanup, hook contract implementation, observability wiring, and regression testing.

## Governance
- Authority: This constitution supersedes prior informal practices for the Basic Rootkit project; conflicts are resolved in favor of the stricter rule.
- Amendments: Proposals require a written change spec referencing impacted principles, risk assessment, and migration plan. Approval needs unanimous maintainers plus legal review when principles change.
- Versioning: Semantic versioning applies (MAJOR for principle removals/redefinitions, MINOR for new principles/sections, PATCH for clarifications). Every amendment updates the version line and Sync Impact Report.
- Compliance reviews: Before each release or quarterly (whichever is sooner), run a checklist validating lab isolation, hook contract freshness, and build reproducibility evidence. Document outcomes in `/docs/compliance.md` (create if missing).

**Version**: 1.0.1 | **Ratified**: 2025-10-11 | **Last Amended**: 2025-10-11
