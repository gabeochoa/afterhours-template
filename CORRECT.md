# CORRECT — repeat-mistake classes (template: every downstream game copies this)

| Rule | Banned shape | Fix level | Evidence (x2+) |
|---|---|---|---|
| CORRECT-T1 | Raw component pointers collected in `for_each_with`, rendered from `once()` | Architecture (hook) | `src/systems/render_sprites_with_shaders.h` at HEAD; same file downstream in kart: 53fa0e9 UAF/stale-frame, `tests/e2e/14` segfaults without `after()` |
| CORRECT-T2 | `PausableSystem::should_run` stub `return true; //TODO missing` | Types (wire real singleton) | File at HEAD claimed `GameStateManager` missing while `src/game_state_manager.h` exists; c49f705 escape/pause relies on `is_paused()`/`is_menu_active()` — dead pause would be copied everywhere |

Why these levels: T1 is hook-order semantics (no test/doc in this repo can catch a frame-stale pointer batch; `after()` is the only correct hook). T2: the singleton type already existed — making `should_run` call it removes the stub instead of documenting it. Docs-only would be copied as a comment and ignored.

Commits (local, one per class): `b09e81a` T1 (`once->after` + `scripts/check_correct.py`), `ba06005` T2 (`!is_paused()` + `make check`).
Proof: `python3 scripts/check_correct.py src` passes HEAD; on pre-fix files (extract `git show c49f705:src/systems/render_sprites_with_shaders.h` / `pausable_system.h` into a temp `src/`) it exits 1 naming CORRECT-T1/T2 (verified before fixing).
Not classes (<x2 in this repo, not acted on): submodule/vendor churn (295f49d/60fa286, one migration), UI TODOs.
Rule table above is the instruction file for this repo; run `make check` before committing.
