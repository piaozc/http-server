package service

import (
	"github.com/piaozc/http-server/business/internal/config"
	"github.com/piaozc/http-server/business/internal/dal"
)

type Services struct {
	Config config.Config
	Store  *dal.MongoStore
}

func NewServices(cfg config.Config, store *dal.MongoStore) *Services {
	return &Services{
		Config: cfg,
		Store:  store,
	}
}
