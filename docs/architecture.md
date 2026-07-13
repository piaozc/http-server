# Cloud Disk Network Layer Architecture

This project uses the C++ server as the public HTTP data plane. The Go service is the business control plane.

## Runtime Boundary

```text
Browser
  |
  | HTTP upload/download
  v
C++ network server
  |
  | custom IPC command/event protocol
  v
Go business service
```

The browser connection is owned by the C++ process, so the C++ process writes the final HTTP response bytes. The Go process does not stream file bytes. It only returns transfer decisions and receives transfer results.

## C++ Responsibilities

- Accept TCP connections.
- Run epoll reactors.
- Parse enough HTTP to identify transfer commands.
- Stream upload request bodies to files.
- Stream files to download responses.
- Maintain connection state, read buffers, write buffers, and file descriptors.
- Notify the business control plane before and after transfers.
- Expose load and flow-control extension points.

## Go Responsibilities

- Authentication and session/token validation.
- User home directory and cross-user access policy.
- File metadata.
- Transfer decision generation.
- Transfer result persistence.

## Current Module Map

- `config/server.conf`
  - `active_environment` selects the runtime block.
  - `[production]` keeps the online port and storage root.
  - `[develop]` keeps the isolated test port and storage root.
- `src/reactor`
  - `MainReactor` owns the listening socket.
  - `SubReactor` owns client epoll instances and `Connection` objects.
  - `SubReactor` also owns an `eventfd` wakeup queue so worker threads can safely return results to the reactor thread.
  - `MainReactor::chooseSubReactor()` currently uses lowest connection count, and can later include queue depth, bandwidth, or latency.
- `src/connection`
  - `Connection` owns one client fd, HTTP parse state, transfer state, read buffer, write buffer, upload fd, and download fd.
  - Socket readiness and connection state changes stay on the reactor thread.
  - File open/read/write tasks run in the thread pool and return completions through `SubReactor::postToLoop()`.
- `src/http`
  - Minimal HTTP request parser and response builders.
- `src/business`
  - `BusinessClient` is the control-plane interface.
  - `MockBusinessClient` is a local stand-in until the Go IPC protocol is implemented.
- `src/transfer`
  - Shared transfer request, decision, and result types.
- `src/threadpool`
  - Generic task execution pool for blocking work.
  - Current file upload writes use `pwrite()` tasks.
  - Current file download reads use `pread()` tasks.
  - Exposes queue depth for future load balancing.

## Planned IPC Protocol

The C++ server should talk to Go through a small custom frame protocol over Unix Domain Socket on Linux. During Windows-side Go development, the same frame protocol can run over `127.0.0.1`.

Frame:

```text
| magic 4B | version 1B | type 1B | request_id 8B | body_len 4B | body |
```

Body can start as JSON for easier debugging:

```json
{
  "action": "download_prepare",
  "request_id": "123",
  "user": "alice",
  "target_user": "bob",
  "path": "/movie.mkv"
}
```

Go response:

```json
{
  "request_id": "123",
  "allow": true,
  "storage_path": "/data/bob/movie.mkv",
  "filename": "movie.mkv",
  "content_type": "application/octet-stream"
}
```

Result event:

```json
{
  "event": "download_done",
  "request_id": "123",
  "success": true,
  "bytes": 21474836480
}
```

## Flow Control Extension Points

Keep these concerns out of business logic:

- Per-connection read enable/disable in `SubReactor::updateClientEvents()`.
- Per-connection write queue size in `Connection`.
- Global reactor load through `SubReactor::load()`.
- Worker queue pressure through `ThreadPool::queueSize()`.
- Future token-bucket limiters can live in `src/transfer` and be called before reading upload bytes or streaming download chunks.
