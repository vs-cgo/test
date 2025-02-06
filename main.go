package main

import (
	"fmt"
	"log"
	"net/http"
  "time"  
  "sync"
  "encoding/json"

  "fyne.io/fyne/v2"
	"fyne.io/fyne/v2/app"
	_"fyne.io/fyne/v2/container"
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
	Info [][]string
  Mutex sync.Mutex
}

var data Data

func main() {
	a := app.New()
	w := a.NewWindow("Server")

	server := http.Server{Addr: ":8888", Handler: nil}
	go startServer(&server)
  settingWindow(&w)
  /*
	//
	buttonHandler := func(row int) {
		log.Printf("Нажата кнопка в строке %d\n", row+1)
	}
*/
  
/*
	table.OnSelected = func(id string) {
		rowIndex, _ := strconv.Atoi(id)
		if rowIndex >= 0 && rowIndex < len(data) {
			buttonHandler(rowIndex)
		}
	}
*/
	w.ShowAndRun()
	server.Close()
}

func settingWindow(w *fyne.Window) {  
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
  data.Info = append(data.Info, 
  []string{u.Domain, u.Mashine, u.IP, u.User, time.Now().Format("02.01.2006 15:04:05")})
  data.Mutex.Unlock()

  w.WriteHeader(http.StatusOK)
  w.Write([]byte("200 OK"))
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
