# 🤖 GEMINI.md — `beepbox`

> **Component-Level Instructions for Antigravity CLI (`agy`).**  
> Location: `/Users/fredi/Workspace/Beeping/beepbox/GEMINI.md`.  
> Hierarchy: Inherits Global Methodology (`~/.gemini/config/rules/methodology.md`) and Workspace Rules (`../GEMINI.md`).

---

## 🎯 Role & Technology Stack

* **Purpose**: Cloud Mode REST API & CLI mixer
* **Tech Stack**: `C++20 · Drogon HTTP Framework · CMake 3.25+ · Docker`
* **Direct Consumers**: Cloud-mode SDKs, Web clients
* **Ecosystem Note**: Hosts /v1/encode and /v1/decode REST endpoints. Validates public key pattern: ^[0-9a-v]{5}$.

---

## 🛠️ Canonical Developer Commands

### 1. Build
```bash
cmake -S . -B build
cmake --build build
```

### 2. Run Tests
```bash
ctest --test-dir build --output-on-failure
```

### 3. Lint & Format
```bash
# Lint checks
clang-format --dry-run --Werror src/*.cpp include/*.h

# Format code
clang-format -i src/*.cpp include/*.h
```

---

## 🚦 Quality Gates & Conventions

1. **Zero Tolerance Quality Gate**:
   * **0 test failures**
   * **0 lint warnings / violations**
   * Code formatting verified before reporting any task done.
2. **Lefthook Enforced**:
   * Pre-commit checks (whitespace, json/yaml lint, gitleaks).
   * Commit-msg Conventional Commits: `feat(scope): BEE-XXXX subject`.
   * Pre-push branch protection (blocks direct push to `develop` or `main`).
3. **Branching Model**:
   * Base branch: `develop`
   * Feature branch: `feat/<task-id>-<slug>`
   * Fix branch: `fix/<task-id>-<slug>`
   * Spike / Discovery branch: `spike/<task-id>-<slug>`
4. **Ecosystem Impact**:
   * If modifying APIs consumed by downstream packages, the `## 🔗 Ecosystem Impact` table must be declared and reviewed.
