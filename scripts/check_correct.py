#!/usr/bin/env python3
"""CORRECT guards: fail on the two repeat-mistake shapes in CORRECT.md."""
import pathlib, re, sys
root = pathlib.Path(sys.argv[1] if len(sys.argv)>1 else "src")
files = list(root.rglob("*.h")) + list(root.rglob("*.cpp"))
bad=[]
for f in files:
    t=f.read_text(errors="ignore")
    # T1: raw component pointers collected in for_each_with must not be rendered from once()
    if "shader_batches" in t and re.search(r"void\s+once\s*\(|virtual void once\s*\(", t):
        bad.append(f"CORRECT-T1 {f}: batch of raw component pointers rendered from once() (once runs BEFORE the entity loop; use after())")
    # T2: PausableSystem must not stub should_run to true
    if f.name=="pausable_system.h" and re.search(r"should_run.*?return true\s*;", t, re.S):
        bad.append(f"CORRECT-T2 {f}: PausableSystem::should_run returns true unconditionally (pause is dead); return !GameStateManager::get().is_paused()")
print("\n".join(bad) if bad else "check_correct: OK")
sys.exit(1 if bad else 0)
