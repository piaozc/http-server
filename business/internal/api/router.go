package api

import (
	"encoding/json"
	"net/http"

	"github.com/piaozc/http-server/business/internal/service"
)

type Router struct {
	services *service.Services
	mux      *http.ServeMux
}

func NewRouter(services *service.Services) *Router {
	r := &Router{
		services: services,
		mux:      http.NewServeMux(),
	}
	r.routes()
	return r
}

func (r *Router) ServeHTTP(w http.ResponseWriter, req *http.Request) {
	r.mux.ServeHTTP(w, req)
}

func (r *Router) routes() {
	r.mux.HandleFunc("GET /health", r.health)
	r.mux.HandleFunc("GET /api/files", r.notImplemented("file list"))
	r.mux.HandleFunc("POST /api/login", r.notImplemented("login"))
}

func (r *Router) health(w http.ResponseWriter, req *http.Request) {
	writeJSON(w, http.StatusOK, map[string]string{
		"status":      "ok",
		"environment": r.services.Config.Environment,
	})
}

func (r *Router) notImplemented(name string) http.HandlerFunc {
	return func(w http.ResponseWriter, req *http.Request) {
		writeJSON(w, http.StatusNotImplemented, map[string]string{
			"error": name + " is not implemented yet",
		})
	}
}

func writeJSON(w http.ResponseWriter, status int, value any) {
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(value)
}
