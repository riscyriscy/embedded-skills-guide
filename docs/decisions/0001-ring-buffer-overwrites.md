# 0001 The ring buffer overwrites the oldest sample

Status: accepted.

## Context

The sensor delivers samples faster than the application reads them when the application is busy. The buffer
has a fixed capacity because the target has 64 KiB of RAM and no allocator.

## Decision

A push into a full buffer replaces the oldest sample and returns true. The caller learns that data was lost
and the newest data is always present.

## Consequences

The mean covers the most recent samples only. A caller that needs every sample must read faster or use a
larger `RINGBUF_CAPACITY`. Refusing the new sample was rejected, because stale readings are worse than
missing ones for a control loop.
