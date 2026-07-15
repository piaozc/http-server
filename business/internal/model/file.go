package model

import "time"

type FileEntry struct {
	ID           string    `bson:"_id,omitempty" json:"id"`
	OwnerUserID  string    `bson:"owner_user_id" json:"owner_user_id"`
	ParentPath   string    `bson:"parent_path" json:"parent_path"`
	Name         string    `bson:"name" json:"name"`
	RelativePath string    `bson:"relative_path" json:"relative_path"`
	StoragePath  string    `bson:"storage_path" json:"storage_path"`
	Size         int64     `bson:"size" json:"size"`
	ContentType  string    `bson:"content_type" json:"content_type"`
	SHA256       string    `bson:"sha256" json:"sha256"`
	IsDir        bool      `bson:"is_dir" json:"is_dir"`
	CreatedAt    time.Time `bson:"created_at" json:"created_at"`
	UpdatedAt    time.Time `bson:"updated_at" json:"updated_at"`
}
