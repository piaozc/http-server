package main

import (
	"context"
	"flag"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/piaozc/http-server/business/internal/api"
	"github.com/piaozc/http-server/business/internal/config"
	"github.com/piaozc/http-server/business/internal/dal"
	"github.com/piaozc/http-server/business/internal/ipc"
	"github.com/piaozc/http-server/business/internal/service"
)

func main() {
	configPath := flag.String("config", "configs/config.develop.json", "path to config file")
	flag.Parse()

	cfg, err := config.Load(*configPath)
	if err != nil {
		log.Fatalf("load config: %v", err)
	}

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()

	store, err := dal.NewMongoStore(ctx, cfg.MongoURI, cfg.MongoDatabase)
	if err != nil {
		log.Fatalf("init mongo: %v", err)
	}
	defer func() {
		shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		_ = store.Close(shutdownCtx)
	}()

	services := service.NewServices(cfg, store)

	ipcServer := ipc.NewServer(cfg.IPCAddr, services)
	go func() {
		if err := ipcServer.ListenAndServe(ctx); err != nil {
			log.Printf("ipc server stopped: %v", err)
			stop()
		}
	}()

	httpServer := &http.Server{
		Addr:              cfg.HTTPAddr,
		Handler:           api.NewRouter(services),
		ReadHeaderTimeout: 5 * time.Second,
	}

	go func() {
		<-ctx.Done()
		shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		_ = httpServer.Shutdown(shutdownCtx)
	}()

	log.Printf("business server environment=%s http=%s ipc=%s mongo_db=%s", cfg.Environment, cfg.HTTPAddr, cfg.IPCAddr, cfg.MongoDatabase)
	if err := httpServer.ListenAndServe(); err != nil && err != http.ErrServerClosed {
		log.Fatalf("http server: %v", err)
	}
}
