// ============================================================================
//  vcl_win.cpp — минимальный VCL-подобный GUI-фреймворк + Windows-драйвер
//  C++17. Контролы — нативные HWND, но за абстракцией IOSDriver / TControl.
//
//  ИЗМЕНЕНИЯ:
//   * TOSControl владеет окном через unique_ptr<IOSHandle>.
//   * ~TOSControl делает FHandle.reset() в теле деструктора — пока this валиден,
//     чтобы ~Win (и DestroyWindow внутри) отработал до разрушения подобъектов.
//   * Win — владелец sink и hwnd. ~Win сам обнуляет sink (владелец поля —
//     владелец и обнуляет) и через unique_hwnd вызывает DestroyWindow.
//   * DestroyHandle / DestroyControl / DestroyWin — удалены.
//   * WM_NCDESTROY чистит GWLP_USERDATA и FByHwnd, но Win НЕ удаляет.
// ============================================================================
#pragma once

#include "include/wil/resource.h"

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#undef min
#undef max

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <sstream>

// ============================================================================
//  vcl namespace
// ============================================================================
namespace vcl {

    // ---------------------------------------------------------------------------
    //  UTF-8 <-> UTF-16 helpers
    // ---------------------------------------------------------------------------
    inline std::wstring Utf8ToW(const std::string& s) {
        if (s.empty()) return {};
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
        std::wstring w(n, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
        return w;
    }
    inline std::string WToUtf8(const std::wstring& w) {
        if (w.empty()) return {};
        int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
            nullptr, 0, nullptr, nullptr);
        std::string s(n, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
            &s[0], n, nullptr, nullptr);
        return s;
    }

    // ============================================================================
    //  TObject
    // ============================================================================
    class TObject {
    public:
        TObject() = default;
        virtual ~TObject() = default;

        virtual const char* ClassName() const { return "TObject"; }

        virtual bool InheritsFrom(const char* cls) const {
            return std::string(cls) == ClassName();
        }

        template<typename T>
        bool is() const { return dynamic_cast<const T*>(this) != nullptr; }

        virtual std::string ToString() const { return ClassName(); }
    };

    // ============================================================================
    //  TComponent — источник истины по ВЛАДЕНИЮ.
    // ============================================================================
    class TComponent : public TObject {
    public:
        using TComponentList = std::vector<std::unique_ptr<TComponent>>;

    protected:
        TComponent* FOwner = nullptr;
        TComponentList  FOwnedComponents;
        std::string     FName;

    public:
        TComponent() = default;

        // ВНИМАНИЕ: Если передан owner, объект СРАЗУ переходит во владение вектору FOwnedComponents.
        // Вызывать delete для такого объекта вручную ЗАПРЕЩЕНО. Его удалит owner.
        explicit TComponent(TComponent* owner) : FOwner(owner) {
            if (owner) {
                owner->InsertComponent(this);
            }
        }

        ~TComponent() override {
            // Защита от рекурсии: сначала зануляем owner, чтобы дочерние элементы
            // при своем уничтожении не пытались вызвать RemoveComponent у умирающего владельца.
            FOwner = nullptr;

            // Очистка дочерних компонентов произойдет автоматически при уничтожении вектора FOwnedComponents
        }

        void InsertComponent(TComponent* c) {
            if (!c) return;

            // Корректный поиск сырого указателя внутри unique_ptr
            auto it = std::find_if(FOwnedComponents.begin(), FOwnedComponents.end(),
                [c](const std::unique_ptr<TComponent>& ptr) { return ptr.get() == c; });

            if (it == FOwnedComponents.end()) {
                // Вектор захватывает владение сырым указателем
                FOwnedComponents.push_back(std::unique_ptr<TComponent>(c));
            }
        }

        void RemoveComponent(TComponent* c) {
            if (!c) return;

            // Исправленный поиск и удаление unique_ptr по сырому указателю
            auto it = std::find_if(FOwnedComponents.begin(), FOwnedComponents.end(),
                [c](const std::unique_ptr<TComponent>& ptr) { return ptr.get() == c; });
            if (it != FOwnedComponents.end()) { FOwnedComponents.erase(it); }
        }

        const std::string& Name() const { return FName; }
        void SetName(const std::string& n) { FName = n; }

        TComponent* FindComponent(const std::string& name) {
            for (auto&& c : FOwnedComponents) {
                if (c->FName == name) return c.get();
                if (auto* r = c->FindComponent(name)) return r;
            }
            return nullptr;
        }

        const char* ClassName() const override { return "TComponent"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TComponent" || TObject::InheritsFrom(cls);
        }
    };

    // ============================================================================
    //  События
    // ============================================================================
    using TNotifyEvent = std::function<void(TObject* Sender)>;

    enum TMouseButton { mbLeft, mbRight, mbMiddle };

    using TMouseEvent = std::function<void(TObject*, TMouseButton, int Shift, int X, int Y)>;
    using TKeyEvent = std::function<void(TObject*, int& Key, int Shift)>;

    // ============================================================================
    //  TColor / TCanvas
    // ============================================================================
    struct TColor {
        uint8_t r = 0, g = 0, b = 0;
        static TColor FromRGB(uint8_t R, uint8_t G, uint8_t B) { return { R, G, B }; }
    };

    class TCanvas {
    public:
        virtual ~TCanvas() = default;

        virtual void SetColor(TColor) = 0;
        virtual void FillRect(int l, int t, int w, int h) = 0;
        virtual void DrawRect(int l, int t, int w, int h) = 0;
        virtual void DrawTextOut(int x, int y, const std::string& text) = 0;
        virtual void Line(int x1, int y1, int x2, int y2) = 0;
        virtual void Clear(TColor) = 0;
    };

    // ============================================================================
    //  IOSHandle / OSEvent / IEventSink
    // ============================================================================
    struct IOSHandle {
        virtual ~IOSHandle() = default;
    };

    struct OSEvent {
        enum Type {
            MouseDown, MouseUp, MouseMove,
            KeyDown, KeyUp,
            Resize, Move, Close, Paint,
            Show, Hide,
            Command,
            Change
        };
        Type     type = Paint;
        int      x = 0, y = 0;
        int      button = 0;
        int      key = 0;
        int      width = 0, height = 0;
        uint32_t timestamp = 0;

        // Sink может выставить true, чтобы отменить операцию (сейчас — Close).
        bool     cancel = false;
    };

    class IEventSink {
    public:
        virtual ~IEventSink() = default;
        virtual void OnOSEvent(OSEvent& e) = 0;
    };

    // ============================================================================
    //  ControlKind / ControlDesc
    // ============================================================================
    enum class ControlKind {
        Form,
        Panel,
        Label,
        Button,
        CheckBox,
        Edit,
        ComboBox,
        ListBox,
    };

    struct ControlDesc {
        ControlKind kind = ControlKind::Panel;
        std::string caption;
        int  x = 0, y = 0, w = 100, h = 25;
        bool visible = true;
        bool enabled = true;
        int  id = 0;
        IOSHandle* parent = nullptr;
    };

    // ============================================================================
    //  IOSDriver
    //
    //  DestroyControl больше нет: Win умирает через unique_ptr<IOSHandle>
    //  у владельца (TOSControl). Драйвер не управляет временем жизни Win.
    // ============================================================================
    class IOSDriver {
    public:
        virtual ~IOSDriver() = default;

        virtual const char* Name() const = 0;

        virtual bool Init() = 0;
        virtual void Shutdown() = 0;
        virtual int  RunMessageLoop() = 0;
        virtual void Quit() = 0;

        virtual IOSHandle* CreateControl(const ControlDesc& d) = 0;

        virtual void SetBounds(IOSHandle* h, int l, int t, int w, int ht) = 0;
        virtual void SetVisible(IOSHandle* h, bool v) = 0;
        virtual void SetText(IOSHandle* h, const std::string& text) = 0;
        virtual std::string GetText(IOSHandle* h) const = 0;
        virtual void SetEnabled(IOSHandle* h, bool e) = 0;
        virtual void Invalidate(IOSHandle* h) = 0;

        virtual void SetCheck(IOSHandle* h, bool c) = 0;
        virtual bool GetCheck(IOSHandle* h) const = 0;
        virtual void AddString(IOSHandle* h, const std::string& s) = 0;
        virtual void SetSel(IOSHandle* h, int idx) = 0;
        virtual int  GetSel(IOSHandle* h) const = 0;

        virtual void SetEventSink(IOSHandle* h, IEventSink* sink) = 0;
        virtual IEventSink* SinkFor(IOSHandle* h) = 0;

        virtual std::unique_ptr<TCanvas> CreateCanvas(IOSHandle* h,
            HDC dc = nullptr,
            bool ownsDC = false) = 0;

        virtual IEventSink* SinkForHwnd(HWND h) = 0;
    };

    // ============================================================================
    //  OSDriverRegistry
    // ============================================================================
    class OSDriverRegistry {
    public:
        using Factory = IOSDriver * (*)();

        static std::vector<std::pair<std::string, Factory>>& Table() {
            static std::vector<std::pair<std::string, Factory>> t;
            return t;
        }

        static void Register(const std::string& name, Factory f) {
            auto& t = Table();
            auto it = std::find_if(t.begin(), t.end(),
                [&](auto& p) { return p.first == name; });
            if (it != t.end()) it->second = f;
            else t.emplace_back(name, f);
        }

        static IOSDriver* Create(const std::string& name) {
            for (auto& p : Table())
                if (p.first == name) return p.second();
            return nullptr;
        }

        static IOSDriver* AutoDetect() {
#if defined(_WIN32)
            if (auto* d = Create("Windows")) return d;
#endif
            return nullptr;
        }

        static std::vector<std::string> Names() {
            std::vector<std::string> r;
            for (auto& p : Table()) r.push_back(p.first);
            return r;
        }
    };

#define REGISTER_OS_DRIVER(NAME, CLASS)                                       \
    namespace {                                                               \
        struct CLASS##Registrar {                                             \
            CLASS##Registrar() {                                              \
                ::vcl::OSDriverRegistry::Register(NAME,                       \
                    []() -> ::vcl::IOSDriver* { return new CLASS(); });       \
            }                                                                 \
        } CLASS##RegistrarInstance;                                           \
    }

    // ============================================================================
    //  TControl — ВИЗУАЛЬНАЯ иерархия (parent/children).
    // ============================================================================
    class TControl : public TComponent {
    protected:
        int  FLeft = 0, FTop = 0;
        int  FWidth = 0, FHeight = 0;
        bool FVisible = true;
        bool FEnabled = true;
        std::string FCaption;

        TControl* FParent = nullptr;
        std::vector<TControl*> FChildControls;

        TNotifyEvent FOnClick;
        TNotifyEvent FOnChange;
        TMouseEvent  FOnMouseDown;
        TMouseEvent  FOnMouseUp;
        TMouseEvent  FOnMouseMove;
        TKeyEvent    FOnKeyDown;

        // Query-хук закрытия. Возврат false → отмена (e.cancel = true).
        // Отдельный от FOnClick/FOnChange, потому что имеет возврат.
        using TCloseQueryEvent = std::function<bool(TObject*, OSEvent&)>;
        TCloseQueryEvent FOnCloseQuery;

        bool FUpdating = false;

    public:
        TControl() = default;
        explicit TControl(TControl* parent) { SetParent(parent); }

        ~TControl() override {
            if (FParent) FParent->RemoveChildControl(this);
        }

        int Left()   const { return FLeft; }
        int Top()    const { return FTop; }
        int Width()  const { return FWidth; }
        int Height() const { return FHeight; }

        virtual void SetBounds(int l, int t, int w, int h) {
            FLeft = l; FTop = t;
            if (w != FWidth || h != FHeight) {
                FWidth = w; FHeight = h;
                OnResize();
            }
            OnMove();
        }
        void SetPosition(int l, int t) { SetBounds(l, t, FWidth, FHeight); }
        void SetSize(int w, int h) { SetBounds(FLeft, FTop, w, h); }

        bool Visible() const { return FVisible; }

        virtual void SetVisible(bool v) {
            if (FVisible == v) return;
            FVisible = v;
            OnVisibleChanged();
        }

        bool Enabled() const { return FEnabled; }
        virtual void SetEnabled(bool e) { FEnabled = e; }

        const std::string& Caption() const { return FCaption; }
        virtual void SetCaption(const std::string& c) { FCaption = c; }

        // Меняет ТОЛЬКО визуальную иерархию. Владение — в TComponent.
        void SetParent(TControl* p) {
            if (FParent == p) return;
            if (FParent) FParent->RemoveChildControl(this);
            FParent = p;
            if (p) p->AddChildControl(this);
        }

        const std::vector<TControl*>& Children() const { return FChildControls; }

        void AddChildControl(TControl* c) {
            if (c && std::find(FChildControls.begin(), FChildControls.end(), c) == FChildControls.end())
                FChildControls.push_back(c);
        }
        void RemoveChildControl(TControl* c) {
            FChildControls.erase(std::remove(FChildControls.begin(), FChildControls.end(), c),
                FChildControls.end());
        }

        TNotifyEvent& OnClick() { return FOnClick; }
        TNotifyEvent& OnChange() { return FOnChange; }
        TMouseEvent& OnMouseDown() { return FOnMouseDown; }
        TMouseEvent& OnMouseUp() { return FOnMouseUp; }
        TMouseEvent& OnMouseMove() { return FOnMouseMove; }
        TKeyEvent& OnKeyDown() { return FOnKeyDown; }

        // Возврат false → отмена закрытия (e.cancel = true).
        TCloseQueryEvent& OnCloseQuery() { return FOnCloseQuery; }

        virtual void OnMove() {}
        virtual void OnResize() {}
        virtual void OnVisibleChanged() {}
        virtual void OnPaint(TCanvas& /*Canvas*/) {}
        virtual void Invalidate() {}

        virtual void PaintTree(TCanvas& c) {
            if (!FVisible) return;
            OnPaint(c);
            for (auto* child : FChildControls) child->PaintTree(c);
        }

        const char* ClassName() const override { return "TControl"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TControl" || TComponent::InheritsFrom(cls);
        }
    };

    // ============================================================================
    //  TOSControl — TControl с HWND.
    //
    //  FHandle — unique_ptr<IOSHandle>. Владение окном — здесь.
    //  ~TOSControl: FHandle.reset() в ТЕЛЕ деструктора, пока this ещё валиден.
    //  Это гарантирует, что ~Win отработает до разрушения подобъектов TControl,
    //  и DestroyWindow внутри ~Win не «выстрелит» в полуразрушенный объект.
    // ============================================================================
    class TOSControl : public TControl, public IEventSink {
    protected:
        std::unique_ptr<IOSHandle> FHandle;
        IOSDriver* FDriver = nullptr;
        int        FId = 0;

    public:
        TOSControl() = default;

        ~TOSControl() override {
            // Тело деструктора: this ещё валиден, подобъекты целы.
            // reset() уничтожит Win ЗДЕСЬ — ~Win сделает sink=nullptr и DestroyWindow.
        }

        bool CreateHandlesRecursive(IOSHandle* parentHandle) {
            if (!CreateHandle(parentHandle)) return false;
            for (auto* c : FChildControls) {
                if (auto* os = dynamic_cast<TOSControl*>(c)) {
                    if (!os->CreateHandlesRecursive(FHandle.get())) return false;
                }
            }
            return true;
        }

        virtual ControlKind Kind() const { return ControlKind::Panel; }

        void SetDriver(IOSDriver* d) { FDriver = d; }
        IOSDriver* Driver() const { return FDriver; }

        // Пока не тащим если не требуется
        // IOSHandle* Handle() const { return FHandle.get(); }

        // Создать HWND себя. parentHandle == nullptr — top-level,
        // иначе — дочернее окно относительно указанного родителя.
        virtual bool CreateHandle(IOSHandle* parentHandle) {
            if (FHandle) return true;
            if (!FDriver) return false;

            ControlDesc d;
            d.kind = Kind();
            d.caption = FCaption;
            d.x = FLeft; d.y = FTop; d.w = FWidth; d.h = FHeight;
            d.visible = FVisible;
            d.enabled = FEnabled;
            d.id = FId;
            d.parent = parentHandle;

            FHandle.reset(FDriver->CreateControl(d));
            if (!FHandle) return false;

            FDriver->SetEventSink(FHandle.get(), this);
            FDriver->SetText(FHandle.get(), FCaption);
            FDriver->SetVisible(FHandle.get(), FVisible);
            FDriver->SetEnabled(FHandle.get(), FEnabled);
            return true;
        }

        // Раздать драйвер всему поддереву.
        void DistributeDriverRecursive() {
            if (!FDriver) return;
            for (auto* c : FChildControls) {
                if (auto* os = dynamic_cast<TOSControl*>(c)) {
                    os->SetDriver(FDriver);
                    os->DistributeDriverRecursive();
                }
            }
        }

        // FHandle != nullptr ⇒ FDriver != nullptr.
        void SetBounds(int l, int t, int w, int h) override {
            TControl::SetBounds(l, t, w, h);
            if (FHandle) FDriver->SetBounds(FHandle.get(), l, t, w, h);
        }
        void SetVisible(bool v) override {
            TControl::SetVisible(v);
            if (FHandle) FDriver->SetVisible(FHandle.get(), v);
        }
        void SetCaption(const std::string& c) override {
            TControl::SetCaption(c);
            if (FHandle) FDriver->SetText(FHandle.get(), c);
        }
        void SetEnabled(bool e) override {
            TControl::SetEnabled(e);
            if (FHandle) FDriver->SetEnabled(FHandle.get(), e);
        }
        void Invalidate() override {
            if (FHandle) FDriver->Invalidate(FHandle.get());
            TControl::Invalidate();
        }

        void OnOSEvent(OSEvent& e) override {
            switch (e.type) {
            case OSEvent::MouseDown:
                if (FOnMouseDown) FOnMouseDown(this, ToButton(e.button), 0, e.x, e.y);
                break;
            case OSEvent::MouseUp:
                if (FOnMouseUp) FOnMouseUp(this, ToButton(e.button), 0, e.x, e.y);
                break;
            case OSEvent::MouseMove:
                if (FOnMouseMove) FOnMouseMove(this, mbLeft, 0, e.x, e.y);
                break;
            case OSEvent::KeyDown: {
                int key = e.key;
                if (FOnKeyDown) FOnKeyDown(this, key, 0);
                break;
            }
            case OSEvent::Resize:
                FWidth = e.width;
                FHeight = e.height;
                OnResize();
                break;
            case OSEvent::Move:
                FLeft = e.x;
                FTop = e.y;
                OnMove();
                break;
            case OSEvent::Change:
                if (!FUpdating && FOnChange) FOnChange(this);
                break;
            case OSEvent::Command:
                if (FOnClick) FOnClick(this);
                break;
            case OSEvent::Paint:
                DoPaint();
                break;
            case OSEvent::Close:
                // Query-хук: логика решает, можно ли закрываться.
                // Возврат false → отмена (e.cancel = true).
                if (FOnCloseQuery && !FOnCloseQuery(this, e)) {
                    e.cancel = true;
                }
                break;
            case OSEvent::Show:      FVisible = true;  break;
            case OSEvent::Hide:      FVisible = false; break;
            }
        }

        void DoPaint(HDC dcFromPaint = nullptr) {
            if (!FDriver || !FHandle) return;
            auto canvas = FDriver->CreateCanvas(FHandle.get(), dcFromPaint);
            if (canvas) PaintTree(*canvas);
        }

        const char* ClassName() const override { return "TOSControl"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TOSControl" || TControl::InheritsFrom(cls);
        }
    private:
        static TMouseButton ToButton(int b) {
            switch (b) {
            case 1: return mbLeft;
            case 2: return mbRight;
            case 3: return mbMiddle;
            default: return mbLeft;
            }
        }
    };

    // ============================================================================
    //  TForm — обычный TOSControl. Отличается поведением и двумя публичными
    //  операциями: CreateHandle (свой HWND) и CreateFormHandlesRecursive
    //  (построить поддерево относительно своего HWND).
    // ============================================================================
    class TForm : public TOSControl {
        bool FClosed = false;
        TNotifyEvent FOnHide;
        TNotifyEvent FOnShow;
        TNotifyEvent FOnClose;
    public:
        TForm() = default;
        explicit TForm(TComponent* owner) : TOSControl() { (void)owner; }

        ControlKind Kind() const override { return ControlKind::Form; }

        TNotifyEvent& OnShow() { return FOnShow; }
        TNotifyEvent& OnHide() { return FOnHide; }
        TNotifyEvent& OnClose() { return FOnClose; }


        bool CreateFormHandlesRecursive() {
            return CreateHandlesRecursive(FHandle.get());
        }

        void Show() { if (!FClosed) SetVisible(true); }
        void Hide() { SetVisible(false); }

        void Close() {
            if (FClosed) return;
            FClosed = true;
            if (FOnClose) FOnClose(this);
            SetVisible(false);
        }

        void OnOSEvent(OSEvent& e) override {
            switch (e.type) {
            case OSEvent::Close:
                // Сначала даём логике решить (query-хук). Если она отменяет —
                // e.cancel = true, и драйвер заглушит WM_CLOSE.
                TOSControl::OnOSEvent(e);
                if (e.cancel) return;
                // Логика разрешила — уведомляем post-factum и прячем.
                Close();
                return;
            case OSEvent::Show:
                TOSControl::OnOSEvent(e);   // FVisible = true
                if (FOnShow) FOnShow(this); // логический слой
                return;
            case OSEvent::Hide:
                TOSControl::OnOSEvent(e);   // FVisible = false
                if (FOnHide) FOnHide(this); // логический слой
                return;
            default:
                TOSControl::OnOSEvent(e);
                return;
            }
        }

        const char* ClassName() const override { return "TForm"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TForm" || TOSControl::InheritsFrom(cls);
        }
    };

    // ============================================================================
    //  TApplication — одна MainForm. Всё.
    // ============================================================================
    class TApplication {
        std::unique_ptr<IOSDriver> FDriver;
        TForm* FMainForm = nullptr;
        std::string FTitle;
    public:
        TApplication() = default;
        ~TApplication() {
            delete FMainForm;
            FMainForm = nullptr;
        }

        void SetDriver(std::unique_ptr<IOSDriver> d) { FDriver = std::move(d); }

        const std::string& Title() const { return FTitle; }
        void SetTitle(const std::string& t) { FTitle = t; }

        void SetMainForm(TForm* f) { FMainForm = f; }
        TForm* MainForm() const { return FMainForm; }

        int Run() {
            if (!FDriver) {
                std::cerr << "[TApplication] No driver set!\n";
                return 1;
            }
            if (!FMainForm) {
                std::cerr << "[TApplication] No main form!\n";
                return 1;
            }
            if (!FDriver->Init()) {
                std::cerr << "[TApplication] Driver Init() failed!\n";
                return 2;
            }

            // 1. Раздать драйвер дереву.
            FMainForm->SetDriver(FDriver.get());
            FMainForm->DistributeDriverRecursive();
            FMainForm->SetVisible(false);

            // 2. Создать HWND главной формы (top-level). Единственный nullptr
            // в потоке создания HWND — здесь.
            if (!FMainForm->CreateHandle(nullptr))
                return 3;

            // 3. Построить HWND всего поддерева относительно HWND формы.
            if (!FMainForm->CreateFormHandlesRecursive())
                return 4;

            // 4. Подписка на закрытие главной формы = выход из приложения.
            FMainForm->OnClose() = [this](TObject*) { Terminate(); };

            // 5. Показать.
            FMainForm->Show();

            // 6. Цикл сообщений.
            return FDriver->RunMessageLoop();
        }

        void Terminate() {

            if (FDriver) FDriver->Quit();
        }
    };

    // ============================================================================
    //  Простые контролы
    // ============================================================================
    class TLabel : public TOSControl {
    public:
        TLabel() = default;
        explicit TLabel(TControl* parent) : TOSControl() { SetParent(parent); }

        ControlKind Kind() const override { return ControlKind::Label; }
        const char* ClassName() const override { return "TLabel"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TLabel" || TOSControl::InheritsFrom(cls);
        }
    };

    class TButton : public TOSControl {
    public:
        TButton() = default;
        explicit TButton(TControl* parent) : TOSControl() { SetParent(parent); }

        ControlKind Kind() const override { return ControlKind::Button; }
        const char* ClassName() const override { return "TButton"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TButton" || TOSControl::InheritsFrom(cls);
        }
    };

    class TCheckBox : public TOSControl {
    public:
        TCheckBox() = default;
        explicit TCheckBox(TControl* parent) : TOSControl() { SetParent(parent); }

        ControlKind Kind() const override { return ControlKind::CheckBox; }

        void SetChecked(bool c) {
            FUpdating = true;
            if (FHandle) FDriver->SetCheck(FHandle.get(), c);
            FUpdating = false;
        }
        bool Checked() const {
            return FHandle ? FDriver->GetCheck(FHandle.get()) : false;
        }

        const char* ClassName() const override { return "TCheckBox"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TCheckBox" || TOSControl::InheritsFrom(cls);
        }
    };

    class TEdit : public TOSControl {
    public:
        TEdit() = default;
        explicit TEdit(TControl* parent) : TOSControl() { SetParent(parent); }

        ControlKind Kind() const override { return ControlKind::Edit; }

        std::string Text() const {
            return FHandle ? FDriver->GetText(FHandle.get()) : std::string{};
        }
        void SetText(const std::string& s) {
            FUpdating = true;
            if (FHandle) FDriver->SetText(FHandle.get(), s);
            FUpdating = false;
        }

        const char* ClassName() const override { return "TEdit"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TEdit" || TOSControl::InheritsFrom(cls);
        }
    };

    class TComboBox : public TOSControl {
        std::vector<std::string> FPending;
    public:
        TComboBox() = default;
        explicit TComboBox(TControl* parent) : TOSControl() { SetParent(parent); }

        ControlKind Kind() const override { return ControlKind::ComboBox; }

        void AddItem(const std::string& s) {
            if (FHandle) FDriver->AddString(FHandle.get(), s);
            else FPending.push_back(s);
        }
        int  SelectedIndex() const {
            return FHandle ? FDriver->GetSel(FHandle.get()) : -1;
        }
        void SetSelectedIndex(int i) {
            FUpdating = true;
            if (FHandle) FDriver->SetSel(FHandle.get(), i);
            FUpdating = false;
        }

        bool CreateHandle(IOSHandle* parentHandle) override {
            if (!TOSControl::CreateHandle(parentHandle)) return false;
            FUpdating = true;
            for (auto& s : FPending) FDriver->AddString(FHandle.get(), s);
            if (!FPending.empty()) FDriver->SetSel(FHandle.get(), 0);
            FUpdating = false;
            FPending.clear();
            return true;
        }

        const char* ClassName() const override { return "TComboBox"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TComboBox" || TOSControl::InheritsFrom(cls);
        }
    };

    class TPanel : public TOSControl {
    public:
        TPanel() = default;
        explicit TPanel(TControl* parent) : TOSControl() { SetParent(parent); }

        ControlKind Kind() const override { return ControlKind::Panel; }

        const char* ClassName() const override { return "TPanel"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TPanel" || TOSControl::InheritsFrom(cls);
        }
    };

    // ============================================================================
    //  Windows-драйвер
    // ============================================================================
#if defined(_WIN32)

    class TWindowsCanvas : public TCanvas {
        HDC      FDC = nullptr;
        HWND     FHwnd = nullptr;
        wil::unique_hbrush FBrush;
        wil::unique_hpen   FPen;
        COLORREF FColor = RGB(0, 0, 0);
        bool     FOwnsDC = false;

    public:
        TWindowsCanvas(HDC dc, HWND hwnd, bool ownsDC)
            : FDC(dc), FHwnd(hwnd), FOwnsDC(ownsDC) {
        }

        ~TWindowsCanvas() override {
            if (FOwnsDC && FDC && FHwnd) {
                ::ReleaseDC(FHwnd, FDC);
            }
        }

        void SetColor(TColor c) override {
            FColor = RGB(c.r, c.g, c.b);
            FBrush.reset(CreateSolidBrush(FColor));
            FPen.reset(CreatePen(PS_SOLID, 1, FColor));
        }

        void FillRect(int l, int t, int w, int h) override {
            RECT r{ l, t, l + w, t + h };
            HBRUSH b = FBrush ? FBrush.get() : (HBRUSH)GetStockObject(BLACK_BRUSH);
            ::FillRect(FDC, &r, b);
        }

        void DrawRect(int l, int t, int w, int h) override {
            HBRUSH oldB = (HBRUSH)SelectObject(FDC, GetStockObject(NULL_BRUSH));
            HPEN   oldP = (HPEN)SelectObject(FDC, FPen ? FPen.get()
                : GetStockObject(BLACK_PEN));
            ::Rectangle(FDC, l, t, l + w, t + h);
            SelectObject(FDC, oldB);
            SelectObject(FDC, oldP);
        }

        void DrawTextOut(int x, int y, const std::string& text) override {
            SetTextColor(FDC, FColor);
            SetBkMode(FDC, TRANSPARENT);
            std::wstring w = Utf8ToW(text);
            ::TextOutW(FDC, x, y, w.c_str(), (int)w.size());
        }

        void Line(int x1, int y1, int x2, int y2) override {
            HPEN oldP = (HPEN)SelectObject(FDC, FPen ? FPen.get()
                : GetStockObject(BLACK_PEN));
            MoveToEx(FDC, x1, y1, nullptr);
            ::LineTo(FDC, x2, y2);
            SelectObject(FDC, oldP);
        }

        void Clear(TColor c) override {
            RECT r;
            GetClientRect(FHwnd, &r);
            wil::unique_hbrush b(CreateSolidBrush(RGB(c.r, c.g, c.b)));
            ::FillRect(FDC, &r, b.get());
        }
    };

    class ITWindowsDriver {
    public:
        // ---- Служебная структура окна ----------------------------------------
        //
        //  Win — владелец sink и hwnd.
        //  ~Win — единственная точка, где Win рвёт свои связи:
        //    * sink = nullptr  (владелец поля обнуляет поле)
        //    * hwnd уникальный  (unique_hwnd сам вызовет DestroyWindow)
        //  Порядок: sink = nullptr ДО разрушения hwnd → WM_* от DestroyWindow
        //  прилетят в WndProc, увидят sink == nullptr и ничего не сделают.
        // ------------------------------------------------------------------------
        struct Win : IOSHandle {
            wil::unique_hwnd hwnd;
            IEventSink* sink = nullptr;
            ITWindowsDriver* owner = nullptr;
            ControlKind      kind = ControlKind::Panel;
            int              id = 0;
            bool             isForm = false;

            ~Win() override {
                // Владелец поля обнуляет поле.
                // WM_* от DestroyWindow (ниже, в ~unique_hwnd) увидят sink == nullptr.
                sink = nullptr;
                // hwnd разрушится автоматически: unique_hwnd вызовет DestroyWindow.
                // WM_NCDESTROY почистит GWLP_USERDATA и FByHwnd.
            }
        };

        // ---- Хранилище окон ---------------------------------------------------
        //  Владеет Win-обёртками НЕ драйвер — их владелец TOSControl
        //  (unique_ptr<IOSHandle>). Здесь карта для маршрутизации WndProc.
        std::map<HWND, Win*> FByHwnd;
        HINSTANCE            FInst = nullptr;
        HINSTANCE            FHInst = nullptr;
        int                  FNextId = 1000;

        virtual ~ITWindowsDriver() = default;

        // ---- Настройка инстанса ----------------------------------------------
        void SetHInstance(HINSTANCE h) { FHInst = h; }
        HINSTANCE GetHInstance() const { return FInst; }

        // ---- Регистрация оконных классов -------------------------------------
        bool InitWindowClasses() {
            FInst = FHInst ? FHInst : GetModuleHandleW(nullptr);

            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
            wc.lpfnWndProc = &ITWindowsDriver::WndProc;
            wc.hInstance = FInst;
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = nullptr;
            wc.lpszClassName = L"VCLFormClass";
            if (!RegisterClassExW(&wc) &&
                GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                LOG_LAST_ERROR();
                return false;
            }

            WNDCLASSEXW wc2 = wc;
            wc2.lpszClassName = L"VCLPanelClass";
            wc2.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            if (!RegisterClassExW(&wc2) &&
                GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                LOG_LAST_ERROR();
                return false;
            }
            return true;
        }

        void UnregisterWindowClasses() {
            if (FInst) {
                UnregisterClassW(L"VCLFormClass", FInst);
                UnregisterClassW(L"VCLPanelClass", FInst);
            }
        }

        // ---- Помощник создания окна ------------------------------------------
        Win* CreateWin(const ControlDesc& d,
            const wchar_t* cls,
            DWORD style, DWORD exStyle,
            HWND parent,
            int x, int y, int w, int h,
            int ctrlId)
        {
            auto* win = new Win();
            win->kind = d.kind;
            win->id = d.id;
            win->isForm = (d.kind == ControlKind::Form);
            win->owner = this;

            std::wstring wcap = Utf8ToW(d.caption);

            win->hwnd.reset(::CreateWindowExW(
                exStyle, cls, wcap.c_str(), style,
                x, y, w, h, parent, (HMENU)(INT_PTR)ctrlId,
                FInst, win));

            if (!win->hwnd) {
                LOG_LAST_ERROR();
                delete win;
                return nullptr;
            }

            SetWindowLongPtrW(win->hwnd.get(), GWLP_USERDATA, (LONG_PTR)win);
            FByHwnd[win->hwnd.get()] = win;
            return win;
        }

        // =======================================================================
        //  Виртуальные хуки обработки сообщений.
        //  Наследник переопределяет только нужные ему.
        //  По умолчанию — пустые.
        // =======================================================================

        virtual void OnCommand(HWND /*hwnd*/, Win* /*w*/, WORD /*code*/,
            HWND /*child*/, WORD /*id*/) {
        }

        virtual void OnPaint(HWND /*hwnd*/, Win* /*w*/, HDC /*dc*/) {}

        virtual bool OnEraseBackground(HWND /*hwnd*/, Win* /*w*/, HDC /*dc*/) {
            return false;
        }

        virtual void OnShowWindow(HWND /*hwnd*/, Win* /*w*/, BOOL /*shown*/) {}

        // true  → "отменяю закрытие", WM_CLOSE глушится (окно живо).
        // false → "пусть закрывается", DefWindowProc сделает DestroyWindow.
        virtual bool OnClose(HWND /*hwnd*/, Win* /*w*/) { return false; }

        virtual void OnSize(HWND /*hwnd*/, Win* /*w*/,
            int /*width*/, int /*height*/) {
        }

        virtual void OnMove(HWND /*hwnd*/, Win* /*w*/, int /*x*/, int /*y*/) {}

        virtual void OnMouseDown(HWND /*hwnd*/, Win* /*w*/,
            int /*x*/, int /*y*/, int /*button*/) {
        }

        virtual void OnMouseUp(HWND /*hwnd*/, Win* /*w*/,
            int /*x*/, int /*y*/, int /*button*/) {
        }

        virtual void OnMouseMove(HWND /*hwnd*/, Win* /*w*/,
            int /*x*/, int /*y*/) {
        }

        virtual void OnKeyDown(HWND /*hwnd*/, Win* /*w*/, int /*vk*/) {}

        virtual void OnKeyUp(HWND /*hwnd*/, Win* /*w*/, int /*vk*/) {}

        // ---- Универсальный WndProc -------------------------------------------
        // Только маршрутизация. Никакой бизнес-логики.
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
            Win* w = nullptr;

            if (msg == WM_NCCREATE) {
                auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
                w = static_cast<Win*>(cs->lpCreateParams);
                if (w) SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)w);
            }
            else {
                w = reinterpret_cast<Win*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            }

            ITWindowsDriver* drv = w ? w->owner : nullptr;
            if (!drv) {
                return DefWindowProcW(hwnd, msg, wp, lp);
            }

            switch (msg) {
            case WM_COMMAND:
                drv->OnCommand(hwnd, w, HIWORD(wp), (HWND)lp, LOWORD(wp));
                return 0;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                wil::unique_hdc_paint hdc = wil::BeginPaint(hwnd, &ps);
                drv->OnPaint(hwnd, w, hdc.get());
                return 0;
            }

            case WM_ERASEBKGND:
                if (drv->OnEraseBackground(hwnd, w, (HDC)wp)) return 1;
                break;

            case WM_SHOWWINDOW:
                drv->OnShowWindow(hwnd, w, (BOOL)wp);
                return 0;

            case WM_CLOSE:
                // true → отмена (окно живо). false → DefWindowProc сделает DestroyWindow.
                if (drv->OnClose(hwnd, w)) return 0;
                break;

            case WM_DESTROY:
                // Top-level окно ушло в DESTROY → цикл сообщений должен завершиться.
                // Child-окна (GetParent != nullptr) этого не требуют.
                if (!GetParent(hwnd)) {
                    PostQuitMessage(0);
                }
                return 0;

            case WM_NCDESTROY:
                // Последнее сообщение окна. Чистим карты, но Win НЕ удаляем —
                // им владеет TOSControl через unique_ptr<IOSHandle>.
                if (w) {
                    SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                    drv->FByHwnd.erase(hwnd);
                    w->hwnd.release();   // HWND уже мёртв
                }
                return DefWindowProcW(hwnd, msg, wp, lp);

            case WM_SIZE:
                drv->OnSize(hwnd, w, LOWORD(lp), HIWORD(lp));
                return 0;

            case WM_MOVE:
                drv->OnMove(hwnd, w, (short)LOWORD(lp), (short)HIWORD(lp));
                return 0;

            case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
                drv->OnMouseDown(hwnd, w, GET_X_LPARAM(lp), GET_Y_LPARAM(lp),
                    msg == WM_LBUTTONDOWN ? 1 :
                    msg == WM_RBUTTONDOWN ? 2 : 3);
                return 0;

            case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
                drv->OnMouseUp(hwnd, w, GET_X_LPARAM(lp), GET_Y_LPARAM(lp),
                    msg == WM_LBUTTONUP ? 1 :
                    msg == WM_RBUTTONUP ? 2 : 3);
                return 0;

            case WM_MOUSEMOVE:
                drv->OnMouseMove(hwnd, w, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                return 0;

            case WM_KEYDOWN:
                drv->OnKeyDown(hwnd, w, (int)wp);
                return 0;

            case WM_KEYUP:
                drv->OnKeyUp(hwnd, w, (int)wp);
                return 0;
            }

            return DefWindowProcW(hwnd, msg, wp, lp);
        }
    };

    class TWindowsDriver : public IOSDriver, public ITWindowsDriver {
    public:
        inline static TWindowsDriver* singleton_windows_driver = nullptr;

        ~TWindowsDriver() override {
            Shutdown();
        }

        const char* Name() const override { return "Windows"; }

        // ---- IOSDriver: Init / Shutdown --------------------------------------
        bool Init() override {
            if (!InitWindowClasses()) return false;
            singleton_windows_driver = this;
            return true;
        }

        void Shutdown() override {
            UnregisterWindowClasses();
            singleton_windows_driver = nullptr;
        }

        // ---- IOSDriver: создание контролов -----------------------------------
        //
        //  DestroyControl убран: Win удаляется через unique_ptr<IOSHandle>
        //  у владельца TOSControl. Драйвер лишь предоставляет операции над окном.
        IOSHandle* CreateControl(const ControlDesc& d) override {
            HWND parent = d.parent
                ? static_cast<Win*>(d.parent)->hwnd.get()
                : nullptr;

            DWORD style = 0, exStyle = 0;
            const wchar_t* cls = nullptr;

            switch (d.kind) {
            case ControlKind::Form:
                cls = L"VCLFormClass";
                style = WS_OVERLAPPEDWINDOW;
                parent = nullptr;
                break;
            case ControlKind::Panel:
                cls = L"VCLPanelClass";
                style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
                break;
            case ControlKind::Label:
                cls = L"STATIC";
                style = WS_CHILD | WS_VISIBLE | SS_LEFT;
                break;
            case ControlKind::Button:
                cls = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;
                break;
            case ControlKind::CheckBox:
                cls = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX;
                break;
            case ControlKind::Edit:
                cls = L"EDIT";
                style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL;
                exStyle = WS_EX_CLIENTEDGE;
                break;
            case ControlKind::ComboBox:
                cls = L"COMBOBOX";
                style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL;
                break;
            case ControlKind::ListBox:
                cls = L"LISTBOX";
                style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY;
                exStyle = WS_EX_CLIENTEDGE;
                break;
            }

            if (!d.visible) style &= ~WS_VISIBLE;
            if (!d.enabled) style |= WS_DISABLED;

            int x = d.x, y = d.y, ww = d.w, hh = d.h;
            if (d.kind == ControlKind::Form) {
                RECT r{ 0, 0, ww, hh };
                AdjustWindowRectEx(&r, style, FALSE, exStyle);
                ww = r.right - r.left;
                hh = r.bottom - r.top;
            }

            int ctrlId = 0;
            if (d.kind != ControlKind::Form && d.kind != ControlKind::Panel)
                ctrlId = (d.id ? d.id : FNextId++);

            return CreateWin(d, cls, style, exStyle, parent, x, y, ww, hh, ctrlId);
        }

        // ---- IOSDriver: операции над окном -----------------------------------
        void SetBounds(IOSHandle* h, int l, int t, int w, int ht) override {
            auto* win = static_cast<Win*>(h);
            if (!win || !win->hwnd) return;
            if (win->kind == ControlKind::Form) {
                RECT r{ 0, 0, w, ht };
                AdjustWindowRectEx(&r, WS_OVERLAPPEDWINDOW, FALSE, 0);
                ::SetWindowPos(win->hwnd.get(), nullptr, l, t, r.right - r.left, r.bottom - r.top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
            else {
                ::SetWindowPos(win->hwnd.get(), nullptr, l, t, w, ht,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }

        void SetVisible(IOSHandle* h, bool v) override {
            auto* win = static_cast<Win*>(h);
            if (!win) return;
            if (win->hwnd) ::ShowWindow(win->hwnd.get(), v ? SW_SHOW : SW_HIDE);
        }

        void SetText(IOSHandle* h, const std::string& text) override {
            auto* win = static_cast<Win*>(h);
            if (win && win->hwnd)
                ::SetWindowTextW(win->hwnd.get(), Utf8ToW(text).c_str());
        }

        std::string GetText(IOSHandle* h) const override {
            auto* win = static_cast<Win*>(h);
            if (!win || !win->hwnd) return {};
            int len = GetWindowTextLengthW(win->hwnd.get());
            if (len <= 0) return {};
            std::wstring s(len + 1, L'\0');
            int got = GetWindowTextW(win->hwnd.get(), &s[0], len + 1);
            s.resize(got > 0 ? got : 0);
            return WToUtf8(s);
        }

        void SetEnabled(IOSHandle* h, bool e) override {
            auto* win = static_cast<Win*>(h);
            if (win && win->hwnd) ::EnableWindow(win->hwnd.get(), e ? TRUE : FALSE);
        }

        void Invalidate(IOSHandle* h) override {
            auto* w = static_cast<Win*>(h);
            if (w && w->hwnd) ::InvalidateRect(w->hwnd.get(), nullptr, TRUE);
        }

        // ---- IOSDriver: CheckBox / ComboBox / ListBox ------------------------
        void SetCheck(IOSHandle* h, bool c) override {
            auto* win = static_cast<Win*>(h);
            if (win && win->hwnd)
                SendMessageW(win->hwnd.get(), BM_SETCHECK,
                    c ? BST_CHECKED : BST_UNCHECKED, 0);
        }

        bool GetCheck(IOSHandle* h) const override {
            auto* win = static_cast<Win*>(h);
            return win && win->hwnd &&
                SendMessageW(win->hwnd.get(), BM_GETCHECK, 0, 0) == BST_CHECKED;
        }

        void AddString(IOSHandle* h, const std::string& s) override {
            auto* win = static_cast<Win*>(h);
            if (win && win->hwnd) {
                std::wstring ws = Utf8ToW(s);
                SendMessageW(win->hwnd.get(), CB_ADDSTRING, 0, (LPARAM)ws.c_str());
            }
        }

        void SetSel(IOSHandle* h, int idx) override {
            auto* win = static_cast<Win*>(h);
            if (win && win->hwnd)
                SendMessageW(win->hwnd.get(), CB_SETCURSEL, idx, 0);
        }

        int GetSel(IOSHandle* h) const override {
            auto* win = static_cast<Win*>(h);
            return (win && win->hwnd)
                ? (int)SendMessageW(win->hwnd.get(), CB_GETCURSEL, 0, 0) : -1;
        }

        // ---- IOSDriver: sink-и -----------------------------------------------
        void SetEventSink(IOSHandle* h, IEventSink* sink) override {
            if (auto* w = static_cast<Win*>(h)) w->sink = sink;
        }

        IEventSink* SinkFor(IOSHandle* h) override {
            auto* w = static_cast<Win*>(h);
            return w ? w->sink : nullptr;
        }

        IEventSink* SinkForHwnd(HWND h) override {
            auto it = FByHwnd.find(h);
            return it == FByHwnd.end() ? nullptr : it->second->sink;
        }

        // ---- IOSDriver: canvas -----------------------------------------------
        std::unique_ptr<TCanvas> CreateCanvas(IOSHandle* h,
            HDC dc = nullptr,
            bool ownsDC = false) override {
            auto* w = static_cast<Win*>(h);
            if (!w || !w->hwnd) return nullptr;
            if (dc) return std::make_unique<TWindowsCanvas>(dc, w->hwnd.get(), ownsDC);
            HDC tmp = GetDC(w->hwnd.get());
            return std::make_unique<TWindowsCanvas>(tmp, w->hwnd.get(), /*ownsDC=*/true);
        }

        // ---- IOSDriver: цикл сообщений ---------------------------------------
        int RunMessageLoop() override {
            MSG msg;
            while (true) {
                BOOL r = GetMessageW(&msg, nullptr, 0, 0);
                if (r == 0) break;
                if (r < 0) break;
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            return 0;
        }

        void Quit() override { PostQuitMessage(0); }

        // =======================================================================
        //  Переопределение событий Windows.
        //  Пишем только то, что нужно.
        // =======================================================================

        void OnPaint(HWND /*hwnd*/, Win* w, HDC dc) override {
            if (w && w->sink) {
                if (auto* osc = dynamic_cast<TOSControl*>(w->sink)) {
                    osc->DoPaint(dc);
                }
                else {
                    OSEvent e; e.type = OSEvent::Paint;
                    w->sink->OnOSEvent(e);
                }
            }
        }

        void OnCommand(HWND /*hwnd*/, Win* /*w*/, WORD code,
            HWND child, WORD /*id*/) override {
            if (!child) return;
            if (auto* sink = SinkForHwnd(child)) {
                OSEvent e; e.key = (int)code;
                switch (code) {
                case BN_CLICKED:
                    e.type = OSEvent::Command;
                    sink->OnOSEvent(e);
                    break;
                case CBN_SELCHANGE:
                case CBN_EDITCHANGE:
                case EN_CHANGE:
                    e.type = OSEvent::Change;
                    sink->OnOSEvent(e);
                    break;
                default:
                    break;
                }
            }
        }

        bool OnEraseBackground(HWND hwnd, Win* w, HDC dc) override {
            if (w && (w->kind == ControlKind::Panel ||
                w->kind == ControlKind::Form)) {
                RECT rc; GetClientRect(hwnd, &rc);
                wil::unique_hbrush br(
                    CreateSolidBrush(GetSysColor(COLOR_BTNFACE)));
                ::FillRect(dc, &rc, br.get());
                return true;
            }
            return false;
        }

        void OnShowWindow(HWND /*hwnd*/, Win* w, BOOL shown) override {
            if (w && w->sink) {
                OSEvent e;
                e.type = shown ? OSEvent::Show : OSEvent::Hide;
                w->sink->OnOSEvent(e);
            }
        }

        // true  → "отменяю закрытие", WM_CLOSE глушится.
        // false → пусть DefWindowProc сделает DestroyWindow.
        //
        // Никаких кастов к конкретным типам: sink сам решает,
        // выставив e.cancel = true.
        bool OnClose(HWND /*hwnd*/, Win* w) override {
            if (!w || !w->sink) return false;

            OSEvent e; e.type = OSEvent::Close;
            e.cancel = false;
            w->sink->OnOSEvent(e);
            return e.cancel;
        }

        void OnSize(HWND /*hwnd*/, Win* w, int width, int height) override {
            if (w && w->sink) {
                OSEvent e; e.type = OSEvent::Resize;
                e.width = width; e.height = height;
                w->sink->OnOSEvent(e);
            }
        }

        void OnMove(HWND /*hwnd*/, Win* w, int x, int y) override {
            if (w && w->sink) {
                OSEvent e; e.type = OSEvent::Move;
                e.x = x; e.y = y;
                w->sink->OnOSEvent(e);
            }
        }

        void OnMouseDown(HWND /*hwnd*/, Win* w, int x, int y, int button) override {
            if (w && w->sink) {
                OSEvent e; e.type = OSEvent::MouseDown;
                e.x = x; e.y = y; e.button = button;
                w->sink->OnOSEvent(e);
            }
        }

        void OnMouseUp(HWND /*hwnd*/, Win* w, int x, int y, int button) override {
            if (w && w->sink) {
                OSEvent e; e.type = OSEvent::MouseUp;
                e.x = x; e.y = y; e.button = button;
                w->sink->OnOSEvent(e);
            }
        }

        void OnMouseMove(HWND /*hwnd*/, Win* w, int x, int y) override {
            if (w && w->sink) {
                OSEvent e; e.type = OSEvent::MouseMove;
                e.x = x; e.y = y;
                w->sink->OnOSEvent(e);
            }
        }

        void OnKeyDown(HWND /*hwnd*/, Win* w, int vk) override {
            if (w && w->sink) {
                OSEvent e; e.type = OSEvent::KeyDown; e.key = vk;
                w->sink->OnOSEvent(e);
            }
        }

        void OnKeyUp(HWND /*hwnd*/, Win* w, int vk) override {
            if (w && w->sink) {
                OSEvent e; e.type = OSEvent::KeyUp; e.key = vk;
                w->sink->OnOSEvent(e);
            }
        }
    };

    REGISTER_OS_DRIVER("Windows", TWindowsDriver);

#endif // _WIN32

} // namespace vcl

// ============================================================================
//  wWinMain — точка входа
// ============================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    using namespace vcl;

    IOSDriver* drv = OSDriverRegistry::AutoDetect();
    if (!drv) {
        MessageBoxW(nullptr, L"Не найден драйвер", L"Ошибка", MB_ICONERROR);
        return 1;
    }

    if (auto* wdrv = dynamic_cast<TWindowsDriver*>(drv)) {
        wdrv->SetHInstance(hInstance);
        TWindowsDriver::singleton_windows_driver = wdrv;
    }

    TApplication app;
    app.SetDriver(std::unique_ptr<IOSDriver>(drv));
    app.SetTitle("VCL Demo");

    // --- Главная форма ---
    auto* form = new TForm();
    form->SetCaption("Hello VCL (Win32)");
    form->SetBounds(200, 200, 480, 320);

    // Query-хук: спросить пользователя, закрывать ли приложение.
    // false → e.cancel = true → WM_CLOSE глушится, окно живо.
    form->OnCloseQuery() = [](TObject*, OSEvent&) -> bool {
        int r = MessageBoxW(nullptr,
            L"Точно закрыть приложение?",
            L"Подтверждение",
            MB_YESNO | MB_ICONQUESTION);
        return r == IDYES;
        };

    // --- Панель ---
    auto* panel = new TPanel(form);
    panel->SetBounds(10, 10, 460, 80);

    // --- Метка внутри панели ---
    auto* label = new TLabel(panel);
    label->SetBounds(20, 30, 400, 24);
    label->SetCaption("Press the button!");

    // --- Кнопка на форме ---
    auto* button = new TButton(form);
    button->SetBounds(20, 120, 160, 40);
    button->SetCaption("Click me");

    button->OnClick() = [label](TObject*) {
        label->SetCaption("Clicked at " + std::to_string(GetTickCount64()));
        };

    auto* chk = new TCheckBox(form);
    chk->SetBounds(200, 120, 200, 30);
    chk->SetCaption("Check me");
    chk->OnChange() = [chk](TObject*) {
        (void)chk->Checked();
        };

    auto* combo = new TComboBox(form);
    combo->SetBounds(20, 180, 200, 200);
    combo->AddItem("Москва");
    combo->AddItem("Петербург");
    combo->AddItem("Новосибирск");
    combo->OnChange() = [combo](TObject*) {
        int idx = combo->SelectedIndex();
        (void)idx;
        };

    auto* edit = new TEdit(form);
    edit->SetBounds(20, 230, 250, 25);
    edit->SetText("Введите текст...");
    edit->OnChange() = [edit](TObject*) {
        std::string s = edit->Text();
        (void)s;
        };

    app.SetMainForm(form);
    return app.Run();
}