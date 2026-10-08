# C4D_PieMenu - Radial Marking Menu & Profile Editor for Cinema 4D

**C4D_PieMenu** — расширяемый плагин радиального меню (Pie Menu / Marking Menu) для Maxon Cinema 4D (поддерживаются 2026 и 2025). Плагин обеспечивает быстрый вызов любых инструментов и команд прямо под курсором мыши жестом **Press & Flick**, а также включает полноценный графический редактор профилей с поддержкой **Drag & Drop** из палитры «Customize Commands».

---

## 🌟 Основные возможности

### 1. Глобальный Viewport HUD (Pie Menu)
- **Мгновенный вызов**: зажмите `Ctrl + Shift` (или настроенную клавишу) в любом месте вьюпорта — меню раскроется прямо под курсором мыши.
- **Жест Press & Flick**: проведите мышь в направлении нужного сектора и отпустите клавиши — команда выполняется моментально, без лишних кликов.
- **Клик-подтверждение**: меню также поддерживает выбор левым кликом мыши (LMB) или отмену правым кликом (RMB) / клавишей `Esc`.
- **Премиальный визуальный стиль**: темные полупрозрачные карточки, цветовые акценты слотов, динамическая стрелка-указатель, направляющие спицы и информационный бейдж текущего режима.
- **Центральный слот (Hub)**: центральная зона мертвой зоны (Deadzone) также может использоваться как отдельный слот (например, Undo или центрирование).

### 2. Контекстная адаптация (Режимы моделирования)
Плагин автоматически переключает активный набор команд в зависимости от текущего режима документа:
- **Polygons Mode**: Extrude, Inner Extrude, Bevel, Bridge, Loop Cut, Subdivide, Delete, Close Poly Hole.
- **Edges Mode**: Bevel, Slide, Edge Cut, Dissolve, Extrude, Spin Edge, Delete, Bridge.
- **Points Mode**: Optimize, Weld, Stitch & Sew, Slide, Set Point Value, Subdivide, Delete.
- **Objects Mode**: Group Objects, Connect & Delete, Current State to Object, Reset PSR, Make Editable, Axis Center, Delete.
- **Global**: Move Tool, Scale Tool, Rotate Tool, Select Tool, Center = Undo.

### 3. Графический конфигуратор (GeDialog + GeUserArea)
- **Команда вызова**: меню *Extensions* -> *Pie Menu Configuration*.
- **Интерактивный радиальный виджет**: наглядное отображение 4 или 8 слотов с их иконками, цветами и названиями.
- **Drag & Drop**: перетаскивайте любые команды напрямую из окна *Customize Commands* (Настроить команды) Cinema 4D на слоты виджета!
- **Инспектор слота**:
  - Назначение любого Command ID вручную или перетаскиванием.
  - Кастомное название (Custom Label).
  - Настройка индивидуального цвета подсветки (Accent Color).
  - Включение/отключение слота.
  - Очистка слота кнопкой *Clear Slot*.
- **Глобальные параметры**:
  - Радиус раскрытия меню (Menu Radius).
  - Радиус мертвой зоны (Deadzone).
  - Переключатель триггера `Ctrl + Shift`.
- **Сохранение и сброс**: настройки сохраняются в файл `piemenu_profiles.dat` в директории пользовательских настроек Cinema 4D. Кнопка *Reset to Defaults* сбрасывает профили на заводские.

---

## ⌨️ Управление и горячие клавиши

| Жест / Клавиша | Действие |
|---|---|
| **Удержание `Ctrl + Shift`** | Открыть радиальное меню под курсором мыши |
| **Движение мыши** | Наведение на сектор меню (с подсветкой активного слота) |
| **Отпускание `Ctrl + Shift`** | Выполнить выбранную команду (Press & Flick) |
| **LMB (клик)** | Выполнить выбранную команду |
| **RMB или `Esc`** | Отменить и закрыть меню без выполнения |

---

## 🛠️ Сборка и установка

### Требования
- Visual Studio 2022 (MSVC v143)
- CMake 3.20+
- Cinema 4D C++ SDK (2025 или 2026)

### Сборка
- Для C4D 2026: запустите `build_2026.bat` или выполните `.\build_2026.ps1`
- Для C4D 2025: запустите `build_2025.bat` или выполните `.\build_2025.ps1`

### Деплой
- Для C4D 2026: запустите `deploy_2026.bat` (копирует плагин в сетевую папку `\\vmware-host\Shared Folders\plugins\C4D_PieMenu`)
- Для C4D 2025: запустите `deploy_2025.bat`

---

## 📁 Структура проекта

```text
C4D_PieMenu/
├── project/
│   └── projectdefinition.txt        # Модуль Cinema 4D (net.maxonexample.c4dpiemenu)
├── source/
│   ├── piemenu_ids.h                # Plugin IDs (SceneHook=1067833, Dialog=1067834)
│   ├── piemenu_data.h / .cpp        # Модели PieSlot, PieProfile, PieProfileManager
│   ├── piemenu_scenehook.h / .cpp   # SceneHookData: перехват ввода и 2D HUD вьюпорта
│   ├── piemenu_editor_area.h / .cpp # GeUserArea: радиальный виджет с Drag & Drop
│   ├── piemenu_editor_dialog.h / .cpp # GeDialog: окно настроек профилей
│   └── main.cpp                     # Регистрация плагинов
├── build_2026.bat / .ps1
├── build_2025.bat / .ps1
├── deploy_2026.bat / .ps1
├── deploy_2025.bat / .ps1
└── README.md
```
