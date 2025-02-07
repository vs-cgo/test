package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"golang.org/x/image/bmp"
	"image"
	"image/color"
	"image/draw"
	"log"
	"net/http"
	"sync"
	"time"

	"fyne.io/fyne/v2"
	"fyne.io/fyne/v2/app"
	"fyne.io/fyne/v2/canvas"
	_ "fyne.io/fyne/v2/container"
	"fyne.io/fyne/v2/widget"
)

type User struct {
	Domain  string `json:"domain"`
	Mashine string `json:"mashine"`
	IP      string `json:"ip"`
	User    string `json:"user"`
	Time    string `json:"time"`
}

type Data struct {
	Info    [][]string
	Address string
	Mutex   sync.Mutex
}

var data Data

func init() {
	flag.StringVar(&data.Address, "U", "", "host:port [127.0.0.1:8888]")
	flag.Parse()
}
func main() {
	if data.Address == "" {
		flag.Usage()
		return
	}

	a := app.New()
	w := a.NewWindow("Server")
	w.SetOnClosed(func() {
		a.Quit()
	})
	// need host:port to start server
	server := http.Server{Addr: data.Address, Handler: nil}
	go startServer(&server)
	settingWindow(&w, a)

	w.ShowAndRun()
	server.Close()
}

func settingWindow(w *fyne.Window, a fyne.App) {
	table := widget.NewTableWithHeaders(
		func() (int, int) {
			return len(data.Info), 5
		},
		func() fyne.CanvasObject {
			return widget.NewLabel("test")
		},
		func(i widget.TableCellID, o fyne.CanvasObject) {
			o.(*widget.Label).SetText(data.Info[i.Row][i.Col])
		},
	)

	table.OnSelected = func(id widget.TableCellID) {
		if id.Col == 4 {
			data.Mutex.Lock()
			screenSave(a, data.Info[id.Row][2])
			data.Mutex.Unlock()
		}
		table.Unselect(id)
	}

	table.ShowHeaderColumn = false
	table.CreateHeader = func() fyne.CanvasObject {
		return widget.NewLabel("Header")
	}

	table.UpdateHeader = func(id widget.TableCellID, template fyne.CanvasObject) {
		switch id.Col {
		case 0:
			template.(*widget.Label).SetText("Domain")
		case 1:
			template.(*widget.Label).SetText("Mashine")
		case 2:
			template.(*widget.Label).SetText("Ip")
		case 3:
			template.(*widget.Label).SetText("User")
		case 4:
			template.(*widget.Label).SetText("Last")
		}
	}

	table.SetColumnWidth(0, 100)
	table.SetColumnWidth(1, 150)
	table.SetColumnWidth(2, 170)
	table.SetColumnWidth(3, 130)
	table.SetColumnWidth(4, 150)
	go update(table)
	(*w).SetContent(table)
	(*w).Resize(fyne.Size{780, 400})
}

func screenSave(a fyne.App, url string) {
	nw := a.NewWindow("Image")
	img := getImage(url)
	nw.SetContent(img)
	nw.Show()
}

func startServer(s *http.Server) {
	http.HandleFunc("/adduser", addUserHandler)
	fmt.Printf("Server listen at %s\n", s.Addr)
	log.Print(s.ListenAndServe())
}

func addUserHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != "POST" {
		http.Error(w, "wrong method", http.StatusMethodNotAllowed)
		return
	}
	var u User
	err := json.NewDecoder(r.Body).Decode(&u)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	data.Mutex.Lock()
	upd := true
	for i := 0; i < len(data.Info); i++ {
		if data.Info[i][2] == u.IP {
			data.Info[i][0] = u.Domain
			data.Info[i][1] = u.Mashine
			data.Info[i][3] = u.User
			data.Info[i][4] = time.Now().Format("02.01.2006 15:04:05")
			upd = false
			break
		}
	}
	if upd {
		data.Info = append(data.Info, []string{u.Domain, u.Mashine, u.IP,
			u.User, time.Now().Format("02.01.2006 15:04:05")})
	}
	data.Mutex.Unlock()

	w.WriteHeader(http.StatusOK)
	w.Write([]byte("200 OK ghbdrnt"))
}

func update(t *widget.Table) {
	for {
		for i := 0; i < len(data.Info); i++ {
			if ping(data.Info[i][2]) {
				data.Info[i][4] = time.Now().Format("02.01.2006 15:04:05")
			}
		}
		t.Refresh()
		time.Sleep(10 * time.Second)
	}
}

func ping(url string) bool {
	str := fmt.Sprintf("http://%s:7777/ping", url)
	res, err := http.Get(str)
	if err != nil || res.StatusCode != http.StatusOK {
		return false
	}
	return true
}

func getImage(url string) *canvas.Image {
	str := fmt.Sprintf("http://%s:7777/getimage", url)
	res, err := (&(http.Client{Timeout: 1 * time.Second})).Get(str)
	img := image.NewRGBA(image.Rect(0, 0, 300, 300))

	if err != nil || res.StatusCode != http.StatusOK {
		draw.Draw(img, img.Bounds(),
			&image.Uniform{color.RGBA{R: 250, G: 100, B: 100, A: 255}},
			image.ZP, draw.Src)
		fimg := canvas.NewImageFromImage(img)
		fimg.SetMinSize(fyne.NewSize(300, 300))
		return fimg
	}
	ret, err := bmp.Decode(res.Body)
	if err != nil {
		draw.Draw(img, img.Bounds(),
			&image.Uniform{color.RGBA{R: 100, G: 250, B: 100, A: 255}},
			image.ZP, draw.Src)
		fimg := canvas.NewImageFromImage(img)
		fimg.SetMinSize(fyne.NewSize(300, 300))
		return fimg
	}
	defer res.Body.Close()

	fimg := canvas.NewImageFromImage(ret)
	//fimg.SetMinSize(fyne.NewSize(600, 600))
	fimg.FillMode = canvas.ImageFillOriginal
	return fimg
}
