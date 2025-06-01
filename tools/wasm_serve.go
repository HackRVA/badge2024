package main

import (
	"flag"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
)

func copyDir(src, dst string) error {
	return filepath.Walk(src, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		relPath, err := filepath.Rel(src, path)
		if err != nil {
			return err
		}

		destPath := filepath.Join(dst, relPath)

		if info.IsDir() {
			return os.MkdirAll(destPath, os.ModePerm)
		}

		srcFile, err := os.Open(path)
		if err != nil {
			return err
		}
		defer srcFile.Close()

		destFile, err := os.Create(destPath)
		if err != nil {
			return err
		}
		defer destFile.Close()

		_, err = io.Copy(destFile, srcFile)
		return err
	})
}

var (
	port    = flag.String("port", "8080", "Port to serve on")
	srcDir  = flag.String("src", "", "Source directory to copy from (optional)")
	destDir = flag.String("dir", "./build_wasm/source/", "Destination directory to copy to")

	password string
)

func main() {
	flag.Parse()

	if *srcDir != "" {
		fmt.Println("Copying files...")
		err := copyDir(*srcDir, *destDir)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error copying files: %v\n", err)
			os.Exit(1)
		}
		fmt.Println("Files copied successfully.")
	} else {
		fmt.Println("No source directory specified, skipping copy.")
	}

	password = os.Getenv("BADGESIM_PASSWORD")

	http.Handle("/", http.HandlerFunc(fileHandler))

	fmt.Printf("Serving on :%s\n", *port)
	err := http.ListenAndServe("0.0.0.0:"+*port, nil)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error starting server: %v\n", err)
		os.Exit(1)
	}
}

func fileHandler(w http.ResponseWriter, r *http.Request) {
	fs := http.FileServer(http.Dir(*destDir))
	w.Header().Set("Cross-Origin-Opener-Policy", "same-origin")
	w.Header().Set("Cross-Origin-Embedder-Policy", "require-corp")

	fs.ServeHTTP(w, r)
}
