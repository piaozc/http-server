package config

import (
	"encoding/json"
	"fmt"
	"os"
)

type Config struct {
	Environment   string `json:"environment"`
	HTTPAddr      string `json:"http_addr"`
	IPCAddr       string `json:"ipc_addr"`
	MongoURI      string `json:"mongo_uri"`
	MongoDatabase string `json:"mongo_database"`
	StorageRoot   string `json:"storage_root"`
}

func Load(path string) (Config, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return Config{}, fmt.Errorf("read config: %w", err)
	}

	var cfg Config
	if err := json.Unmarshal(data, &cfg); err != nil {
		return Config{}, fmt.Errorf("parse config: %w", err)
	}

	if cfg.Environment == "" {
		cfg.Environment = "develop"
	}
	if cfg.HTTPAddr == "" {
		cfg.HTTPAddr = ":18080"
	}
	if cfg.IPCAddr == "" {
		cfg.IPCAddr = "127.0.0.1:19090"
	}
	if cfg.MongoDatabase == "" {
		cfg.MongoDatabase = "cloud_disk_dev"
	}
	if cfg.StorageRoot == "" {
		cfg.StorageRoot = "../storage-dev"
	}

	return cfg, nil
}
