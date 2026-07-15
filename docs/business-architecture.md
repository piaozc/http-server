# Business Side Architecture

The Go service is the business control plane. The C++ service remains the network data plane.

## Process Boundary

```text
Browser
  |
  | HTTP pages and metadata APIs
  v
Go business server

Browser
  |
  | HTTP upload/download bytes
  v
C++ network server
  |
  | prepare/report IPC
  v
Go business server
```

Large file bytes do not flow through Go. Go only makes decisions and records metadata.

## MongoDB

MongoDB is the metadata store.

Initial document groups:

- `users`
- `files`
- `permissions`
- `transfers`

These are intentionally early models. The exact fields can change after the API design document is finalized.

## Go Module Layout

```text
business/
  cmd/server              process entry
  configs                 environment configs
  internal/api            browser-facing HTTP API
  internal/ipc            C++ control-plane IPC server
  internal/service        business use cases
  internal/dal            MongoDB access
  internal/model          MongoDB document models
  pkg/response            shared API response helpers
```

## Current Entrypoints

HTTP:

- `GET /health`
- `GET /api/files` placeholder
- `POST /api/login` placeholder

IPC:

- TCP listener placeholder, default `127.0.0.1:19090` in develop.
- Frame protocol will be implemented after the C++/Go contract is finalized.

## Interfaces To Decide Together

Browser-facing APIs:

- login/logout/session
- current user profile
- file list
- directory create
- upload URL/parameters
- download URL/parameters
- delete
- rename/move
- user access policy

C++ IPC APIs:

- upload prepare
- download prepare
- transfer report
- optional transfer heartbeat/progress

## Environment Split

Develop config:

```text
HTTP: :18080
IPC:  127.0.0.1:19090
DB:   cloud_disk_dev
```

Production config:

```text
HTTP: :18088
IPC:  127.0.0.1:19091
DB:   cloud_disk_prod
```

