# Era snapshots

Created to keep pre-hackathon, post-hackathon, and current trees side by side without deleting history.

| Folder | Git tag | Commit | Date (author) | Tip message |
|--------|---------|--------|---------------|-------------|
| `before-hackathon/` | `era/before-hackathon` | `71fe23f291d3c3225deb1bc887211012d8d0e0d7` | 2026-06-12 | refactor code to stop prioritizing precision for speed |
| `after-hackathon/` | `era/after-hackathon` | `d614b0503ceb77189da147796823cfef9f01aa91` | 2026-06-28 | Merge pull request #3 from NanoOpusGoonClawX/main |
| `current/` (live) | `era/current-at-restructure` at restructure time | `e31ba589888c0cb08d2d493bde8036b89851a6cc` | 2026-08-02 | refactor(fw): modularize firmware into six yoinkable ESP-IDF components |

## Notes

- `before-hackathon` is the parent side of merge PR #3 ("update codebase with hackathon codebase") — last `main` state before that dump.
- `after-hackathon` is merge commit `d614b050` (PR #3 into main).
- `current/` keeps evolving; the tag `era/current-at-restructure` pins what "now" meant when the folders were created.
- Snapshot folders are full trees from `git archive`; they do not replace `git log` / tags for true history.
