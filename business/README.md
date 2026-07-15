# Cloud Disk Business Server

The Go business server is the control plane for the cloud disk project.

It owns:

- user/session/auth logic
- file and directory metadata
- access policy
- transfer prepare decisions
- transfer result recording

It does not stream large file bytes. File transfer bytes stay in the C++ network server.

## Layout

```text
cmd/server       process entry
configs          environment config files
internal/api     browser-facing HTTP API
internal/ipc     C++ network server control-plane IPC
internal/service business use cases
internal/dal     MongoDB access
internal/model   document models
```

## Run

```bash
go mod tidy
go run ./cmd/server -config configs/config.develop.json
```

`GET /health` should return service status.

