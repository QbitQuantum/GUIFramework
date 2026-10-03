// ============================================================================
//  vcl_win.cpp — минимальный VCL-подобный GUI-фреймворк + Windows-драйвер
//  C++17. Контролы — нативные HWND, но за абстракцией IOSDriver / TControl.
//
//  КОНТРАКТ ВЛАДЕНИЯ (C++-объекты):
//   1. TComponent владеет детьми через FOwnedComponents (vector<unique_ptr>).
//   2. Владение передаётся В КОНСТРУКТОРЕ: TComponent(TComponent* owner).
//   3. Ребёнок удаляется ТОЛЬКО через деструктор owner-а.
//   4. Удалять ребёнка в обход owner-а (delete, .reset(), ...) ЗАПРЕЩЕНО.
//      Нарушение — UB, и это баг того, кто нарушил.
//   5. FOwner — только для чтения (Owner()). Не использовать для удаления.
//   6. SetParent(TControl*) — визуальная иерархия, отдельно от владения.
//   7. RemoveComponent нет. Отцепления в ~TComponent нет. Дети удаляются
//      автоматически при разрушении FOwnedComponents.
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
//  INHERITED(Base) — псевдоним базового класса внутри текущего.
//
//  Переключатель VCL_USE_TYPEDEF_INHERITED:
//    1 — typedef Base inherited;   (стиль C++ Builder / VCL)
//    0 — using inherited = Base;   (современный C++)
//
//  Использование:
//      class TControl : public TComponent {
//          INHERITED(TComponent);
//          ...
//      };
//
//  Тогда в методах:
//      inherited::SetBounds(...);   // = TComponent::SetBounds(...)
// ============================================================================
#define VCL_USE_TYPEDEF_INHERITED 1

#if VCL_USE_TYPEDEF_INHERITED
#  define INHERITED(Base) typedef Base inherited;
#else
#  define INHERITED(Base) using inherited = Base;
#endif

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

    void ShowMessage(const wchar_t* msg)
    {
        MessageBoxW(nullptr, msg, L"Ошибка", MB_ICONERROR);
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
        INHERITED(TObject);
    public:
        using TComponentList = std::vector<std::unique_ptr<TComponent>>;

    protected:
        TComponent* FOwner = nullptr;   // только для чтения
        TComponentList  FOwnedComponents;   // владеет детьми
        std::string     FName;

        // Принять this во владение. Вызывается ТОЛЬКО из конструктора
        // TComponent(TComponent*). Наружу не торчит.
        void AdoptThis(TComponent* c) {
            if (!c) return;
            c->FOwner = this;
            FOwnedComponents.emplace_back(c);
        }

    public:

        // Владение — здесь. Owner забирает unique_ptr(this).
        explicit TComponent(TComponent* owner) : FOwner(owner) {
            if (owner) {
                owner->AdoptThis(this);
            }
        }

        ~TComponent() override = default;

        TComponent(const TComponent&) = delete;
        TComponent& operator=(const TComponent&) = delete;

        TComponent* Owner() const { return FOwner; }

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
            return std::string(cls) == "TComponent" || inherited::InheritsFrom(cls);
        }
    };

    // ============================================================================
    //  События
    // ============================================================================
    using TNotifyEvent = std::function<void(TObject* Sender)>;
    using TCloseEvent = std::function<void(TObject*, bool&)>;

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
    // ============================================================================
    class IOSDriver {
    public:
        virtual ~IOSDriver() = default;

        virtual const char* Name() const = 0;

        virtual bool Init() = 0;
        virtual void Shutdown() = 0;
        virtual int  RunMessageLoop() = 0;
        virtual void Quit() = 0;

        virtual std::unique_ptr<IOSHandle>
            CreateControl(const ControlDesc& d) = 0;

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
        virtual std::unique_ptr<TCanvas> CreateCanvas(IOSHandle* h,
            HDC dc = nullptr,
            bool ownsDC = false) = 0;
    };

    // ============================================================================
    //  TControl — ВИЗУАЛЬНАЯ иерархия. Владение — в TComponent.
    // ============================================================================
    class TControl : public TComponent {
        INHERITED(TComponent);
    protected:
        int  FLeft = 0, FTop = 0;
        int  FWidth = 0, FHeight = 0;
        bool FVisible = true;
        bool FEnabled = true;
        std::string FCaption;

        TControl* FParent = nullptr;              // визуальный, НЕ владеет

        TNotifyEvent FOnClick;
        TNotifyEvent FOnChange;
        TNotifyEvent FOnHide;
        TNotifyEvent FOnShow;
        TCloseEvent  FOnClose;
        TMouseEvent  FOnMouseDown;
        TMouseEvent  FOnMouseUp;
        TMouseEvent  FOnMouseMove;
        TKeyEvent    FOnKeyDown;
        bool FUpdating = false;

    public:
        explicit TControl(TComponent* owner) : TComponent(owner) {}

        ~TControl() override = default;

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

        TControl* Parent() const { return FParent; }

        TNotifyEvent& OnClick() { return FOnClick; }
        TNotifyEvent& OnChange() { return FOnChange; }
        TMouseEvent& OnMouseDown() { return FOnMouseDown; }
        TMouseEvent& OnMouseUp() { return FOnMouseUp; }
        TMouseEvent& OnMouseMove() { return FOnMouseMove; }
        TKeyEvent& OnKeyDown() { return FOnKeyDown; }

        virtual void OnMove() {}
        virtual void OnResize() {}
        virtual void OnVisibleChanged() {}
        virtual void OnPaint(TCanvas& /*Canvas*/) {}
        virtual void Invalidate() {}

        const char* ClassName() const override { return "TControl"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TControl" || inherited::InheritsFrom(cls);
        }
    };

    // ============================================================================
    //  TOSControl — TControl с HWND.
    // ============================================================================
    class TOSControl : public TControl, public IEventSink {
        INHERITED(TControl);
    protected:
        std::unique_ptr<IOSHandle> FHandle;
        std::vector<TOSControl*> FChildControls;    // дети-контролы, НЕ владеет
        IOSDriver* FDriver = nullptr;
        int        FId = 0;

    public:
        explicit TOSControl(TComponent* owner) : TControl(owner) {}

        ~TOSControl() override {
            // Тело деструктора: this ещё валиден, подобъекты целы.
            // reset() уничтожит Win ЗДЕСЬ — ~Win сделает sink=nullptr и DestroyWindow.
            FHandle.reset();
        }

        // ЯВНЫЙ вызов. Меняет ТОЛЬКО визуальную иерархию.
        void SetParent(TOSControl* p) {
            if (FParent == p) return;
            if (FParent) RemoveChildControl(this);
            FParent = p;
            if (p) p->AddChildControl(this);
        }

        void AddChildControl(TOSControl* c) {
            if (c && std::find(FChildControls.begin(), FChildControls.end(), c) == FChildControls.end())
                FChildControls.push_back(c);
        }
        void RemoveChildControl(TOSControl* c) {
            FChildControls.erase(std::remove(FChildControls.begin(), FChildControls.end(), c),
                FChildControls.end());
        }

        virtual void PaintTree(TCanvas& c) {
            if (!FVisible) return;
            OnPaint(c);
            for (auto* child : FChildControls) child->PaintTree(c);
        }

        TOSControl(const TOSControl&) = delete;
        TOSControl& operator=(const TOSControl&) = delete;

        virtual ControlKind Kind() const { return ControlKind::Panel; }

        void SetDriver(IOSDriver* d) { FDriver = d; }
        IOSDriver* Driver() const { return FDriver; }

        virtual void CreateHandle(IOSHandle* parentHandle) {
            if (FHandle) return;
            if (!FDriver) return;

            ControlDesc d;
            d.kind = Kind();
            d.caption = FCaption;
            d.x = FLeft; d.y = FTop; d.w = FWidth; d.h = FHeight;
            d.visible = FVisible;
            d.enabled = FEnabled;
            d.id = FId;
            d.parent = parentHandle;

            FHandle = FDriver->CreateControl(d);
            if (!FHandle) return;

            FDriver->SetEventSink(FHandle.get(), this);
            FDriver->SetText(FHandle.get(), FCaption);
            FDriver->SetVisible(FHandle.get(), FVisible);
            FDriver->SetEnabled(FHandle.get(), FEnabled);
        }

        void CreateHandlesRecursive(IOSHandle* parentHandle) {
            CreateHandle(parentHandle);
            for (auto* c : FChildControls) {
                c->CreateHandlesRecursive(parentHandle);
            }
        }

        void DistributeDriverRecursive() {
            if (!FDriver) return;
            for (auto* c : FChildControls) {
                c->SetDriver(FDriver);
                c->DistributeDriverRecursive();
            }
        }

        void SetBounds(int l, int t, int w, int h) override {
            inherited::SetBounds(l, t, w, h);
            if (FHandle) FDriver->SetBounds(FHandle.get(), l, t, w, h);
        }
        void SetVisible(bool v) override {
            inherited::SetVisible(v);
            if (FHandle) FDriver->SetVisible(FHandle.get(), v);
        }
        void SetCaption(const std::string& c) override {
            inherited::SetCaption(c);
            if (FHandle) FDriver->SetText(FHandle.get(), c);
        }
        void SetEnabled(bool e) override {
            inherited::SetEnabled(e);
            if (FHandle) FDriver->SetEnabled(FHandle.get(), e);
        }
        void Invalidate() override {
            if (FHandle) FDriver->Invalidate(FHandle.get());
            inherited::Invalidate();
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
            return std::string(cls) == "TOSControl" || inherited::InheritsFrom(cls);
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
    //  TForm
    // ============================================================================
    class TForm : public TOSControl {
        INHERITED(TOSControl);
    public:
        explicit TForm(TComponent* owner) : TOSControl(owner) {}
        ~TForm() override = default;

        ControlKind Kind() const override { return ControlKind::Form; }

        TNotifyEvent& OnShow() { return FOnShow; }
        TNotifyEvent& OnHide() { return FOnHide; }
        TCloseEvent& OnClose() { return FOnClose;}

        void CreateHandle() {
            inherited::CreateHandle(nullptr);
            CreateHandlesRecursive(FHandle.get());
        }

        void Show() { inherited::SetVisible(true); }
        void Hide() { inherited::SetVisible(false); }

        void OnOSEvent(OSEvent& e) override {
            switch (e.type) {
            case OSEvent::Close:
                inherited::OnOSEvent(e);
                if (FOnClose) FOnClose(this, e.cancel);
                return;
            case OSEvent::Show:
                inherited::OnOSEvent(e);
                if (FOnShow) FOnShow(this);
                return;
            case OSEvent::Hide:
                inherited::OnOSEvent(e);
                if (FOnHide) FOnHide(this);
                return;
            default:
                inherited::OnOSEvent(e);
                return;
            }
        }

        const char* ClassName() const override { return "TForm"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TForm" || inherited::InheritsFrom(cls);
        }
    };

    // ============================================================================
    //  Простые контролы
    // ============================================================================
    class TLabel : public TOSControl {
        INHERITED(TOSControl);
    public:
        explicit TLabel(TComponent* owner) : TOSControl(owner) {}
        ~TLabel() override = default;

        ControlKind Kind() const override { return ControlKind::Label; }
        const char* ClassName() const override { return "TLabel"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TLabel" || inherited::InheritsFrom(cls);
        }
    };

    class TButton : public TOSControl {
        INHERITED(TOSControl);
    public:
        explicit TButton(TComponent* owner) : TOSControl(owner) {}
        ~TButton() override = default;

        ControlKind Kind() const override { return ControlKind::Button; }
        const char* ClassName() const override { return "TButton"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TButton" || inherited::InheritsFrom(cls);
        }
    };

    class TCheckBox : public TOSControl {
        INHERITED(TOSControl);
    public:
        explicit TCheckBox(TComponent* owner) : TOSControl(owner) {}
        ~TCheckBox() override = default;

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
            return std::string(cls) == "TCheckBox" || inherited::InheritsFrom(cls);
        }
    };

    class TEdit : public TOSControl {
        INHERITED(TOSControl);
    public:
        explicit TEdit(TComponent* owner) : TOSControl(owner) {}
        ~TEdit() override = default;

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
            return std::string(cls) == "TEdit" || inherited::InheritsFrom(cls);
        }
    };

    class TComboBox : public TOSControl {
        INHERITED(TOSControl);
        std::vector<std::string> FPending;
    public:
        explicit TComboBox(TComponent* owner) : TOSControl(owner) {}
        ~TComboBox() override = default;

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

        void CreateHandle(IOSHandle* parentHandle) override {
            inherited::CreateHandle(parentHandle);
            FUpdating = true;
            for (auto& s : FPending) FDriver->AddString(FHandle.get(), s);
            if (!FPending.empty()) FDriver->SetSel(FHandle.get(), 0);
            FUpdating = false;
            FPending.clear();
        }

        const char* ClassName() const override { return "TComboBox"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TComboBox" || inherited::InheritsFrom(cls);
        }
    };

    class TPanel : public TOSControl {
        INHERITED(TOSControl);
    public:
        explicit TPanel(TComponent* owner) : TOSControl(owner) {}
        ~TPanel() override = default;

        ControlKind Kind() const override { return ControlKind::Panel; }

        const char* ClassName() const override { return "TPanel"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TPanel" || inherited::InheritsFrom(cls);
        }
    };

    // ============================================================================
    //  TApplication : TComponent — корень дерева владения.
    // ============================================================================
    class TApplication : public TComponent {
        INHERITED(TComponent);
        std::unique_ptr<IOSDriver> FDriver;
        TForm* FMainForm = nullptr;   // ссылка; владение — через FOwnedComponents
        std::string FTitle;
    public:
        TApplication(TComponent* owner) : TComponent(owner) {}
        ~TApplication() override {
            // Сначала драйвер: окна умрут, sink-и обнулятся.
            // Потом ~TComponent удалит форму и всех детей.
            if (FDriver) FDriver->Shutdown();
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

            FMainForm->SetDriver(FDriver.get());
            FMainForm->DistributeDriverRecursive();

            FMainForm->CreateHandle();

            FMainForm->Show();

            return FDriver->RunMessageLoop();
        }
    };
    // ============================================================================
    //  Windows-драйвер
    // ============================================================================
#if defined(_WIN32)

    class TWindowsCanvas : public TCanvas {
        INHERITED(TCanvas);
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
        struct Win : IOSHandle {
            wil::unique_hwnd hwnd;
            IEventSink* sink = nullptr;
            ITWindowsDriver* owner = nullptr;
            ControlKind      kind = ControlKind::Panel;
            int              id = 0;
            bool             isForm = false;

            ~Win() override {
                sink = nullptr;
            }
        };

        std::map<HWND, Win*> FByHwnd;
        HINSTANCE            FInst = nullptr;
        int                  FNextId = 1000;

        virtual ~ITWindowsDriver() = default;

        bool InitWindowClasses() {
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
                return false;
            }

            WNDCLASSEXW wc2 = wc;
            wc2.lpszClassName = L"VCLPanelClass";
            wc2.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            if (!RegisterClassExW(&wc2) &&
                GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
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

        std::unique_ptr<Win> CreateWin(const ControlDesc& d,
            const wchar_t* cls,
            DWORD style, DWORD exStyle,
            HWND parent,
            int x, int y, int w, int h,
            int ctrlId)
        {
            auto win = std::make_unique<Win>();
            win->kind = d.kind;
            win->id = d.id;
            win->isForm = (d.kind == ControlKind::Form);
            win->owner = this;
            win->hwnd.reset(::CreateWindowExW(
                exStyle, cls, Utf8ToW(d.caption).c_str(), style,
                x, y, w, h, parent, (HMENU)(INT_PTR)ctrlId,
                FInst, win.get()));

            if (!win->hwnd) return nullptr;
            SetWindowLongPtrW(win->hwnd.get(), GWLP_USERDATA, (LONG_PTR)win.get());
            FByHwnd[win->hwnd.get()] = win.get();
            return win;
        }

        virtual void OnCommand(HWND, Win*, WORD, HWND, WORD) {}
        virtual void OnPaint(HWND, Win*, HDC) {}
        virtual bool OnEraseBackground(HWND, Win*, HDC) { return false; }
        virtual void OnShowWindow(HWND, Win*, BOOL) {}
        virtual bool OnClose(HWND, Win*) { return false; }
        virtual void OnSize(HWND, Win*, int, int) {}
        virtual void OnMove(HWND, Win*, int, int) {}
        virtual void OnMouseDown(HWND, Win*, int, int, int) {}
        virtual void OnMouseUp(HWND, Win*, int, int, int) {}
        virtual void OnMouseMove(HWND, Win*, int, int) {}
        virtual void OnKeyDown(HWND, Win*, int) {}
        virtual void OnKeyUp(HWND, Win*, int) {}

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
                if (drv->OnClose(hwnd, w)) return 0;
                break;

            case WM_DESTROY:
                if (!GetParent(hwnd)) {
                    PostQuitMessage(0);
                }
                return 0;

            case WM_NCDESTROY:
                if (w) {
                    SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                    drv->FByHwnd.erase(hwnd);
                    w->hwnd.release();
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
        INHERITED(IOSDriver);

        // Приватный синглтон. Выставляется только конструктором.
        inline static TWindowsDriver* s_instance = nullptr;

    public:
        explicit TWindowsDriver(HINSTANCE hInstance) {
            FInst = hInstance ? hInstance : GetModuleHandleW(nullptr);

            if (s_instance) {
                ShowMessage(L"TWindowsDriver: instance already exists");
                std::terminate();
            }
            s_instance = this;
        }

        ~TWindowsDriver() override {
            Shutdown();
            if (s_instance == this) s_instance = nullptr;
        }

        TWindowsDriver(const TWindowsDriver&) = delete;
        TWindowsDriver& operator=(const TWindowsDriver&) = delete;

        // Только чтение. Менять указатель снаружи нельзя.
        static TWindowsDriver* Instance() { return s_instance; }

        const char* Name() const override { return "Windows"; }

        bool Init() override {
            if (!InitWindowClasses()) return false;
            return true;
        }

        void Shutdown() override {
            UnregisterWindowClasses();
        }

        std::unique_ptr<IOSHandle> CreateControl(const ControlDesc& d) override {
            HWND parent = d.parent ? static_cast<Win*>(d.parent)->hwnd.get() : nullptr;

            DWORD style = 0, exStyle = 0;
            const wchar_t* cls = nullptr;

            switch (d.kind) {
            case ControlKind::Form:
                cls = L"VCLFormClass";
                style = WS_OVERLAPPEDWINDOW;
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

        void SetBounds(IOSHandle* h, int l, int t, int w, int ht) override {
            auto* win = static_cast<Win*>(h);
            if (!win || !win->hwnd) return;
            ::SetWindowPos(win->hwnd.get(), nullptr, l, t, w, ht,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }

        void SetVisible(IOSHandle* h, bool v) override {
            auto* win = static_cast<Win*>(h);
            if (!win) return;
            if (win->hwnd) ::ShowWindow(win->hwnd.get(), v ? SW_SHOW : SW_HIDE);
        }

        void SetText(IOSHandle* h, const std::string& text) override {
            auto* win = static_cast<Win*>(h);
            if (!win || !win->hwnd) return;
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
            if (!win || !win->hwnd) return;
            ::EnableWindow(win->hwnd.get(), e ? TRUE : FALSE);
        }

        void Invalidate(IOSHandle* h) override {
            auto* win = static_cast<Win*>(h);
            if (!win || !win->hwnd) return;
            ::InvalidateRect(win->hwnd.get(), nullptr, TRUE);
        }

        void SetCheck(IOSHandle* h, bool c) override {
            auto* win = static_cast<Win*>(h);
            if (!win || !win->hwnd) return;
            SendMessageW(win->hwnd.get(), BM_SETCHECK,
                c ? BST_CHECKED : BST_UNCHECKED, 0);
        }

        bool GetCheck(IOSHandle* h) const override {
            auto* win = static_cast<Win*>(h);
            if (!win || !win->hwnd) return false;
            return SendMessageW(win->hwnd.get(), BM_GETCHECK, 0, 0) == BST_CHECKED;
        }

        void AddString(IOSHandle* h, const std::string& s) override {
            auto* win = static_cast<Win*>(h);
            if (!win || !win->hwnd) return;
            std::wstring ws = Utf8ToW(s);
            SendMessageW(win->hwnd.get(), CB_ADDSTRING, 0, (LPARAM)ws.c_str());
        }

        void SetSel(IOSHandle* h, int idx) override {
            auto* win = static_cast<Win*>(h);
            if (!win || !win->hwnd) return;
            SendMessageW(win->hwnd.get(), CB_SETCURSEL, idx, 0);
        }

        int GetSel(IOSHandle* h) const override {
            auto* win = static_cast<Win*>(h);
            if (!win || !win->hwnd) return -1;
            return (int)SendMessageW(win->hwnd.get(), CB_GETCURSEL, 0, 0);
        }

        void SetEventSink(IOSHandle* h, IEventSink* sink) override {
            if (auto* w = static_cast<Win*>(h)) w->sink = sink;
        }

        IEventSink* SinkForHwnd(HWND h) {
            auto it = FByHwnd.find(h);
            return it == FByHwnd.end() ? nullptr : it->second->sink;
        }

        std::unique_ptr<TCanvas> CreateCanvas(IOSHandle* h,
            HDC dc = nullptr,
            bool ownsDC = false) override {
            auto* w = static_cast<Win*>(h);
            if (!w || !w->hwnd) return nullptr;
            if (dc) return std::make_unique<TWindowsCanvas>(dc, w->hwnd.get(), ownsDC);
            HDC tmp = GetDC(w->hwnd.get());
            return std::make_unique<TWindowsCanvas>(tmp, w->hwnd.get(), /*ownsDC=*/true);
        }

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

        void OnPaint(HWND, Win* w, HDC dc) override {
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

        void OnCommand(HWND, Win*, WORD code, HWND child, WORD) override {
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

        template <class F>
        static void Emit(Win* w, OSEvent::Type type, F&& setup) {
            if (!w || !w->sink) return;
            OSEvent e;
            e.type = type;
            setup(e);
            w->sink->OnOSEvent(e);
        }

        bool OnEraseBackground(HWND hwnd, Win* w, HDC dc) override {
            if (!w || (w->kind != ControlKind::Panel &&
                w->kind != ControlKind::Form)) {
                return false;
            }

            RECT rc;
            GetClientRect(hwnd, &rc);
            wil::unique_hbrush br(
                CreateSolidBrush(GetSysColor(COLOR_BTNFACE)));
            ::FillRect(dc, &rc, br.get());
            return true;
        }

        bool OnClose(HWND, Win* w) override {
            if (!w || !w->sink) return false;
            OSEvent e;
            e.type = OSEvent::Close;
            w->sink->OnOSEvent(e);
            return e.cancel;
        }

        void OnShowWindow(HWND, Win* w, BOOL shown) override {
            Emit(w, shown ? OSEvent::Show : OSEvent::Hide, [](OSEvent&) {});
        }

        void OnSize(HWND, Win* w, int width, int height) override {
            Emit(w, OSEvent::Resize, [=](OSEvent& e) {
                e.width = width; e.height = height; });
        }

        void OnMove(HWND, Win* w, int x, int y) override {
            Emit(w, OSEvent::Move, [=](OSEvent& e) {
                e.x = x; e.y = y; });
        }

        void OnMouseDown(HWND, Win* w, int x, int y, int button) override {
            Emit(w, OSEvent::MouseDown, [=](OSEvent& e) {
                e.x = x; e.y = y; e.button = button; });
        }

        void OnMouseUp(HWND, Win* w, int x, int y, int button) override {
            Emit(w, OSEvent::MouseUp, [=](OSEvent& e) {
                e.x = x; e.y = y; e.button = button; });
        }

        void OnMouseMove(HWND, Win* w, int x, int y) override {
            Emit(w, OSEvent::MouseMove, [=](OSEvent& e) {
                e.x = x; e.y = y; });
        }

        void OnKeyDown(HWND, Win* w, int vk) override {
            Emit(w, OSEvent::KeyDown, [=](OSEvent& e) { e.key = vk; });
        }

        void OnKeyUp(HWND, Win* w, int vk) override {
            Emit(w, OSEvent::KeyUp, [=](OSEvent& e) { e.key = vk; });
        }
    };

#endif // _WIN32

} // namespace vcl

// ============================================================================
//  wWinMain — точка входа
//
//  ВЛАДЕНИЕ:
//   * app (TApplication : TComponent) владеет формой и всем поддеревом.
//   * form — new TForm(&app). Владение — у app.
//   * panel — new TPanel(form). Владение — у form.
//   * label — new TLabel(panel). Владение — у panel.
//   * button/chk/combo/edit — new T*(form). Владение — у form.
//   * SetParent — визуальная иерархия, ЯВНО.
//
//  ДРАЙВЕР:
//   * Создаётся напрямую: std::make_unique<TWindowsDriver>(hInstance).
//   * hInstance передаётся в конструктор. Никаких SetHInstance снаружи.
//   * Синглтон TWindowsDriver::Instance() выставляется в конструкторе.
// ============================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    using namespace vcl;

    auto driver = std::make_unique<TWindowsDriver>(hInstance);

    TApplication app(nullptr);
    app.SetDriver(std::move(driver));
    app.SetTitle("VCL Demo");

    // --- Главная форма. Владеет app. ---
    auto* form = new TForm(&app);
    form->SetCaption("Hello VCL (Win32)");
    form->SetBounds(200, 200, 480, 320);

    form->OnClose() = [](TObject*, bool& CanClose) -> void {
        int r = MessageBoxW(nullptr,
            L"Точно закрыть приложение?",
            L"Подтверждение",
            MB_YESNO | MB_ICONQUESTION);
        CanClose = r == IDNO;
        };

    // --- Панель. Владеет form. ---
    auto* panel = new TPanel(form);
    panel->SetParent(form);
    panel->SetBounds(10, 10, 460, 80);

    // --- Метка внутри панели. Владеет panel. ---
    auto* label = new TLabel(panel);
    label->SetParent(panel);
    label->SetBounds(20, 30, 400, 24);
    label->SetCaption("Press the button!");

    // --- Кнопка на форме. Владеет form. ---
    auto* button = new TButton(form);
    button->SetParent(form);
    button->SetBounds(20, 120, 160, 40);
    button->SetCaption("Click me");

    button->OnClick() = [label](TObject*) {
        label->SetCaption("Clicked at " + std::to_string(GetTickCount64()));
        };

    auto* chk = new TCheckBox(form);
    chk->SetParent(form);
    chk->SetBounds(200, 120, 200, 30);
    chk->SetCaption("Check me");
    chk->OnChange() = [chk](TObject*) {
        (void)chk->Checked();
        };

    auto* combo = new TComboBox(form);
    combo->SetParent(form);
    combo->SetBounds(20, 180, 200, 200);
    combo->AddItem("Москва");
    combo->AddItem("Петербург");
    combo->AddItem("Новосибирск");
    combo->OnChange() = [combo](TObject*) {
        int idx = combo->SelectedIndex();
        (void)idx;
        };

    auto* edit = new TEdit(form);
    edit->SetParent(form);
    edit->SetBounds(20, 230, 250, 25);
    edit->SetText("Введите текст...");
    edit->OnChange() = [edit](TObject*) {
        std::string s = edit->Text();
        (void)s;
        };

    app.SetMainForm(form);
    return app.Run();
}