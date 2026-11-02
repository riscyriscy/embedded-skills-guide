# Releasing

1. Set `VERSION` in the top-level `CMakeLists.txt` to the new number and commit.
2. Tag that commit with `git tag vMAJOR.MINOR.PATCH`.
3. In a clone that has the tags, run `tools/release.sh make` with `SIGN_KEY` set.
4. Check the result with `tools/verify_release.sh build/release/<tag> <public key>`.

The script stops when the commit has no tag, the tree is dirty or `CMakeLists.txt` and the tag disagree.
