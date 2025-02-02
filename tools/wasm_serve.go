package main

import (
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

func main() {
	port := "8080"
	srcDir := "./web/wasm-simulator/"
	destDir := "./build_wasm/source/"

	fmt.Println("Copying files...")
	err := copyDir(srcDir, destDir)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error copying files: %v\n", err)
		os.Exit(1)
	}
	fmt.Println("Files copied successfully.")

	fs := http.FileServer(http.Dir(destDir))

	// these headers seem to be required for the pthreads to work
	// see: https://emscripten.org/docs/porting/pthreads.html
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Cross-Origin-Opener-Policy", "same-origin")
		w.Header().Set("Cross-Origin-Embedder-Policy", "require-corp")
		fs.ServeHTTP(w, r)
	})

	fmt.Printf("Serving on http://localhost:%s\n", port)
	err = http.ListenAndServe("0.0.0.0:"+port, nil)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error starting server: %v\n", err)
		os.Exit(1)
	}
}
