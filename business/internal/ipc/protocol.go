package ipc

const (
	MessageUploadPrepare   = "upload_prepare"
	MessageDownloadPrepare = "download_prepare"
	MessageTransferReport  = "transfer_report"
)

type TransferRequest struct {
	RequestID  string `json:"request_id"`
	Action     string `json:"action"`
	Token      string `json:"token"`
	User       string `json:"user"`
	TargetUser string `json:"target_user"`
	Path       string `json:"path"`
	Filename   string `json:"filename"`
	Size       int64  `json:"size"`
}

type TransferDecision struct {
	Allow       bool   `json:"allow"`
	Reason      string `json:"reason"`
	StoragePath string `json:"storage_path"`
	Filename    string `json:"filename"`
	ContentType string `json:"content_type"`
	Size        int64  `json:"size"`
}

type TransferResult struct {
	RequestID string `json:"request_id"`
	Action    string `json:"action"`
	Success   bool   `json:"success"`
	Bytes     int64  `json:"bytes"`
	Message   string `json:"message"`
}
