package main

import (
	"fmt"
	"os"
	"time"
)

// createLog creates logs/<date>_<time>.csv, or path if given.
// O_EXCL makes it fail instead of overwriting an existing recording.
func createLog(path string) (*os.File, error) {
	if path == "" {
		if err := os.MkdirAll("logs", 0o755); err != nil {
			return nil, err
		}
		path = "logs/" + time.Now().Format("2006-01-02_150405") + ".csv"
	}
	f, err := os.OpenFile(path, os.O_WRONLY|os.O_CREATE|os.O_EXCL, 0o644)
	if err != nil {
		return nil, fmt.Errorf("create log: %w", err)
	}
	return f, nil
}
