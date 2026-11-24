# Releasing

1. Set `VERSION` in the top-level `CMakeLists.txt` to the new number and commit.
2. Tag that commit with `git tag vMAJOR.MINOR.PATCH`.
3. In a clone that has the tags, run `tools/release.sh make` with `SIGN_KEY` set.
4. Check the result with `tools/verify_release.sh build/release/<tag> <public key>`.

The script stops when the commit has no tag, the tree is dirty or `CMakeLists.txt` and the tag disagree.

## A fix for a release in the field

1. Make the fix on `main` first, with its test.
2. Make `release/MAJOR.MINOR` from the tag of that release, if the branch does not exist yet.
3. Copy the fix with `git cherry-pick -x <commit>`. If it needs an earlier commit, copy that one first.
4. Raise `VERSION` in `CMakeLists.txt` on the branch, commit, and tag the commit.
5. Check that nothing is on the branch alone: `git cherry -v main release/MAJOR.MINOR` shows a plus only for the commit that changed the version.
