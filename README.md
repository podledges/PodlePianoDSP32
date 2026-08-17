# Podles-Piano-DSP

ESP32 digital signal processing of piezo-disc signals from a piano.

## Layout

| Path | What it is |
|------|------------|
| `before-hackathon/` | Frozen snapshot of the repo **before** the hackathon codebase landed (`era/before-hackathon`) |
| `after-hackathon/` | Frozen snapshot **right after** the hackathon merge (`era/after-hackathon`) |
| `current/` | Active / latest codebase (continues from former repo root) |
| `docs/` | Living project docs (start here for orientation) |
| `data/` | Datasets, captures, fixtures that are not source code |

The three era folders are **convenience snapshots**. Full Git history is unchanged: every old commit is still on `main` history. Prefer working in `current/`.

## Git tags (same eras, no folders needed)

```bash
git fetch --tags
git switch --detach era/before-hackathon
git switch --detach era/after-hackathon
git switch main   # back to tip (folder layout)
```

## Recover without using the snapshot folders

```bash
git log --oneline --all
git switch -c recover/my-branch <commit-sha>
```

## Commits frozen into the snapshot folders

See `docs/ERA_SNAPSHOTS.md`.
