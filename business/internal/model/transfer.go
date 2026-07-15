package model

import "time"

type Transfer struct {
	ID           string    `bson:"_id,omitempty" json:"id"`
	RequestID    string    `bson:"request_id" json:"request_id"`
	UserID       string    `bson:"user_id" json:"user_id"`
	TargetUserID string    `bson:"target_user_id" json:"target_user_id"`
	Action       string    `bson:"action" json:"action"`
	RelativePath string    `bson:"relative_path" json:"relative_path"`
	StoragePath  string    `bson:"storage_path" json:"storage_path"`
	Status       string    `bson:"status" json:"status"`
	Bytes        int64     `bson:"bytes" json:"bytes"`
	Message      string    `bson:"message" json:"message"`
	CreatedAt    time.Time `bson:"created_at" json:"created_at"`
	FinishedAt   time.Time `bson:"finished_at" json:"finished_at"`
}
