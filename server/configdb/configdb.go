package configdb

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/bmharper/cyclops/server/dbh"
	"github.com/bmharper/cyclops/server/log"
	"gorm.io/gorm"
)

type ConfigDB struct {
	Log log.Log
	DB  *gorm.DB
}

func NewConfigDB(logger log.Log, dbFilename string) (*ConfigDB, error) {
	os.MkdirAll(filepath.Dir(dbFilename), 0777)
	configDB, err := dbh.OpenDB(logger, dbh.DriverSqlite, dbFilename, Migrations(logger), 0)
	if err != nil {
		return nil, fmt.Errorf("Failed to open database %v: %w", dbFilename, err)
	}
	return &ConfigDB{
		Log: logger,
		DB:  configDB,
	}, nil
}
