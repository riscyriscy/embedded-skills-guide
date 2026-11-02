# Building

Install the tools listed by `tools/check_env.sh`, then run the whole pipeline on your machine:

```
tools/ci.sh all
```

The same script runs in GitHub Actions, GitLab CI and Jenkins. A single stage can be run alone, for
example `tools/ci.sh m4` for the target build and its tests under QEMU.
