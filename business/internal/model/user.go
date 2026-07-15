package model

import "time"

type User struct {
	ID           string    `bson:"_id,omitempty" json:"id"`
	Username     string    `bson:"username" json:"username"`
	PasswordHash string    `bson:"password_hash" json:"-"`
	DisplayName  string    `bson:"display_name" json:"display_name"`
	RootName     string    `bson:"root_name" json:"root_name"`
	CreatedAt    time.Time `bson:"created_at" json:"created_at"`
	UpdatedAt    time.Time `bson:"updated_at" json:"updated_at"`
}
