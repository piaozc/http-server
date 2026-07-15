package dal

import (
	"context"
	"fmt"
	"time"

	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

type MongoStore struct {
	Client *mongo.Client
	DB     *mongo.Database
}

func NewMongoStore(ctx context.Context, uri string, database string) (*MongoStore, error) {
	client, err := mongo.Connect(ctx, options.Client().ApplyURI(uri))
	if err != nil {
		return nil, fmt.Errorf("connect mongo: %w", err)
	}

	pingCtx, cancel := context.WithTimeout(ctx, 3*time.Second)
	defer cancel()
	if err := client.Ping(pingCtx, nil); err != nil {
		_ = client.Disconnect(ctx)
		return nil, fmt.Errorf("ping mongo: %w", err)
	}

	return &MongoStore{
		Client: client,
		DB:     client.Database(database),
	}, nil
}

func (s *MongoStore) Close(ctx context.Context) error {
	if s == nil || s.Client == nil {
		return nil
	}
	return s.Client.Disconnect(ctx)
}
