
// ============================================================================
//
//  OpenRTL — свободное ядро RAD-разработки на C++
//
// ============================================================================
//
//  ЧТО ЭТО
//  --------
//
//  OpenRTL — это свободная реализация ядра RTL/VCL-парадигмы
//  на стандартном C++ (C++17/20), без проприетарных компиляторов,
//  без закрытых библиотек, без лицензий
//
//  ЧТО РЕАЛИЗОВАНО
//  --------------
//
//  RTL-ядро:
//    - TObject            корень иерархии, ручной RTTI
//                         (ClassName, InheritsFrom)
//    - Exception          Message, HelpContext, InnerException,
//                         GetBaseException, StackTrace
//    - ERuntimeError      иерархия исключений в стиле RTL
//    - EInvalidArgument
//    - EConvertError
//    - INHERITED(Base)    макрос для inherited:: в стиле Object Pascal
//    - TComponent         владение через FOwner + FOwnedComponents
//
//  VCL-слой:
//    - TControl → TOSControl → TCustomForm → TForm → TForm1
//    - события: TNotifyEvent, TCloseEvent, TMouseEvent, TKeyEvent
//    - контролы: TPanel, TLabel, TButton, TCheckBox, TEdit, TComboBox
//    - визуальная иерархия (FChildControls, FParent) отделена
//      от владения (FOwner, FOwnedComponents)
//
//  Платформенный адаптер:
//    - IOSDriver / IOSHandle / IEventSink / OSEvent — bridge pattern
//    - TWindowsDriver — честный Win32 API под капотом:
//        CreateWindowExW, WndProc, GWLP_USERDATA, WM_PAINT, WM_COMMAND
//    - TCanvas — абстракция рисования (сейчас GDI, дальше — GDI+/Direct2D)
//
//  IDE-стиль:
//    - Application, Form1, _tWinMain — как в сгенерированном коде
//    - конструктор TForm1 создаёт контролы через
//      new + SetParent + SetBounds
//    - события вешаются лямбдами вместо __published методов
//    - TApplication::CreateForm + Run + RunMessageLoop
//
// ============================================================================
//
//  ЧТО ПОД КАПОТОМ
//  --------------
//
//    Win32 message
//        → WndProc
//            → Win* (GWLP_USERDATA)
//                → ITWindowsDriver::OnXxx()
//                    → Emit() → IEventSink::OnOSEvent(OSEvent)
//                        → TControl::OnOSEvent / TOSControl::OnOSEvent
//                            → FOnClick / FOnChange / FOnKeyDown
//
//  Ты можешь ткнуть пальцем в любую строчку этой цепочки
//  и сказать: «вот здесь приходит WM_PAINT, вот здесь он превращается
//  в OSEvent::Paint, вот здесь вызывается OnPaint формы,
//  вот здесь PaintTree рисует детей».
//
//
// ============================================================================
//
//  ЧТО ДАЛЬШЕ
//  ----------
//
//    [ ] DFM-парсер (замок №3)
//    [ ] конвертер DFM → нативный формат
//    [ ] __published-эмуляция через макросы
//    [ ] TInterfacedObject с подсчётом ссылок
//    [ ] TThread + Synchronize + интеграция с message loop
//    [ ] TStringList, TList, TDictionary
//    [ ] TRegistry, TIniFile
//    [ ] RTTI-таблица с published-свойствами
//    [ ] визуальный дизайнер форм (замок №4)
//    [ ] GTK4-драйвер (кроссплатформенность)
//    [ ] Cocoa-драйвер
//    [ ] совместимость с VCL-компонентами (замок №5)
//
// ============================================================================
//
//  ЛИЦЕНЗИЯ
//  --------
//
//  MIT
//
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <sstream>

// ============================================================================
//  INHERITED(Base) — псевдоним базового класса внутри текущего.
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
    //  Exception
    // ============================================================================
    class Exception : public std::exception {
    public:
        explicit Exception(const std::string& msg)
            : FMessage(msg) {
        }

        Exception(const std::string& msg, int helpContext)
            : FMessage(msg), FHelpContext(helpContext) {
        }

        Exception(const std::string& msg, int helpContext,
            std::unique_ptr<Exception> inner)
            : FMessage(msg)
            , FHelpContext(helpContext)
            , FInnerException(std::move(inner)) {
        }

        ~Exception() override = default;

        Exception(const Exception&) = delete;
        Exception& operator=(const Exception&) = delete;

        const std::string& Message() const { return FMessage; }
        void SetMessage(const std::string& m) { FMessage = m; }

        int  HelpContext() const { return FHelpContext; }
        void SetHelpContext(int h) { FHelpContext = h; }

        const Exception* InnerException() const { return FInnerException.get(); }

        Exception* GetBaseException() {
            Exception* e = this;
            while (e->FInnerException)
                e = e->FInnerException.get();
            return e;
        }
        const Exception* GetBaseException() const {
            return const_cast<Exception*>(this)->GetBaseException();
        }

        const std::string& StackTrace() const { return FStackTrace; }
        void SetStackTrace(const std::string& s) { FStackTrace = s; }

        virtual const char* ClassName() const { return "Exception"; }

        virtual std::string ToString() const {
            std::ostringstream os;
            os << ClassName() << ": " << FMessage;

            const Exception* inner = FInnerException.get();
            while (inner) {
                os << "\n  caused by " << inner->ClassName()
                    << ": " << inner->Message();
                inner = inner->FInnerException.get();
            }
            return os.str();
        }

        const char* what() const noexcept override {
            return FMessage.c_str();
        }

    private:
        std::string FMessage;
        int         FHelpContext = 0;
        std::string FStackTrace;

        std::unique_ptr<Exception> FInnerException;
    };

    class ERuntimeError : public Exception {
    public:
        using Exception::Exception;
        const char* ClassName() const override { return "ERuntimeError"; }
    };

    class EInvalidArgument : public Exception {
    public:
        using Exception::Exception;
        const char* ClassName() const override { return "EInvalidArgument"; }
    };

    class EConvertError : public Exception {
    public:
        using Exception::Exception;
        const char* ClassName() const override { return "EConvertError"; }
    };

    [[noreturn]] inline void RaiseOuterException(std::unique_ptr<Exception> e) {
        throw std::runtime_error(e->ToString());
    }

    // ============================================================================
    //  TComponent
    // ============================================================================
    class TComponent : public TObject {
        INHERITED(TObject);
    public:
        using TComponentList = std::vector<std::unique_ptr<TComponent>>;

    protected:
        TComponent* FOwner = nullptr;
        TComponentList  FOwnedComponents;
        std::string     FName;

        void AdoptThis(TComponent* c) {
            if (!c) return;
            c->FOwner = this;
            FOwnedComponents.emplace_back(c);
        }
    public:
        explicit TComponent(TComponent* owner) : FOwner(owner) {
            if (!owner) return;
            owner->AdoptThis(this);
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

        std::size_t OwnedCount() const { return FOwnedComponents.size(); }

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
    protected:
        virtual void Init() = 0;
        virtual void Shutdown() = 0;
    public:
        virtual ~IOSDriver() = default;

        virtual std::unique_ptr<IOSHandle>
            CreateControl(const ControlDesc& d) = 0;
        virtual std::unique_ptr<TCanvas>
            CreateCanvas(IOSHandle* h) = 0;

        virtual const char* Name() const = 0;
        virtual int  RunMessageLoop() = 0;
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
    };

    // ============================================================================
    //  TControl
    // ============================================================================
    class TControl : public TComponent, public IEventSink {
        INHERITED(TComponent);
    private:

        TNotifyEvent FOnClick;
        TNotifyEvent FOnResize;
        TMouseEvent  FOnMouseDown;
        TMouseEvent  FOnMouseUp;
        TMouseEvent  FOnMouseMove;

        static TMouseButton ToButton(int b) {
            switch (b) {
            case 1: return mbLeft;
            case 2: return mbRight;
            case 3: return mbMiddle;
            default: return mbLeft;
            }
        }
    protected:
        int  FLeft = 0, FTop = 0;
        int  FWidth = 0, FHeight = 0;
        bool FVisible = true;
        bool FEnabled = true;
        std::string FCaption;

        TControl* FParent = nullptr;              // визуальный, НЕ владеет

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

        TNotifyEvent& OnResize() { return FOnResize; }
        TNotifyEvent& OnClick() { return FOnClick; }
        TMouseEvent& OnMouseDown() { return FOnMouseDown; }
        TMouseEvent& OnMouseUp() { return FOnMouseUp; }
        TMouseEvent& OnMouseMove() { return FOnMouseMove; }

        virtual void OnMove() {}
        virtual void OnVisibleChanged() {}
        virtual void OnPaint(TCanvas* Canvas) {}
        virtual void Invalidate() {}

        const char* ClassName() const override { return "TControl"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TControl" || inherited::InheritsFrom(cls);
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
            case OSEvent::Resize:
                FWidth = e.width;
                FHeight = e.height;
                if (FOnResize) FOnResize(this);
                break;
            case OSEvent::Move:
                FLeft = e.x;
                FTop = e.y;
                OnMove();
                break;
            case OSEvent::Command:
                if (FOnClick) FOnClick(this);
                break;
            case OSEvent::Show:      FVisible = true;  break;
            case OSEvent::Hide:      FVisible = false; break;
            }
        }

    };

    // ============================================================================
    //  TOSControl
    // ============================================================================
    class TOSControl : public TControl {
        INHERITED(TControl);
    protected:
        std::unique_ptr<IOSHandle> FHandle;
        std::vector<TOSControl*> FChildControls;    // дети-контролы, НЕ владеет
        IOSDriver* FDriver = nullptr;
        int        FId = 0;
    private:

        void AddChildControl(TOSControl* c) {
            if (!c) return;
            if (std::find(FChildControls.begin(),
                FChildControls.end(), c) != FChildControls.end())
                return;
            FChildControls.push_back(c);
        }

        void RemoveChildControl(TOSControl* c) {
            FChildControls.erase(std::remove(FChildControls.begin(), FChildControls.end(), c),
                FChildControls.end());
        }

        TNotifyEvent FOnChange;
        TCloseEvent  FOnClose;
        TKeyEvent    FOnKeyDown;

    public:
        explicit TOSControl(TComponent* owner) : TControl(owner) {}

        ~TOSControl() override = default;

        bool FUpdating = false;

        TCloseEvent& OnClose() { return FOnClose; }
        TNotifyEvent& OnChange() { return FOnChange; }
        TKeyEvent& OnKeyDown() { return FOnKeyDown; }

        void SetParent(TOSControl* p) {
            if (FParent == p) return;
            if (FParent) RemoveChildControl(this);
            FParent = p;
            if (p) p->AddChildControl(this);
        }

        virtual void PaintTree(TCanvas* c) {
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
                c->CreateHandlesRecursive(FHandle.get());
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
            if (!FHandle) return;
            FDriver->SetBounds(FHandle.get(), l, t, w, h);
        }
        void SetVisible(bool v) override {
            inherited::SetVisible(v);
            if (!FHandle) return;
            FDriver->SetVisible(FHandle.get(), v);
        }
        void SetCaption(const std::string& c) override {
            inherited::SetCaption(c);
            if (!FHandle) return;
            FDriver->SetText(FHandle.get(), c);
        }
        void SetEnabled(bool e) override {
            inherited::SetEnabled(e);
            if (!FHandle) return;
            FDriver->SetEnabled(FHandle.get(), e);
        }
        void Invalidate() override {
            if (FHandle) FDriver->Invalidate(FHandle.get());
            inherited::Invalidate();
        }

        void OnOSEvent(OSEvent& e) override {
            switch (e.type) {
            case OSEvent::KeyDown: {
                int key = e.key;
                if (FOnKeyDown) FOnKeyDown(this, key, 0);
                break;
            }
            case OSEvent::Change:
                if (!FUpdating && FOnChange) FOnChange(this);
                break;
            case OSEvent::Show:      FVisible = true;  break;
            case OSEvent::Hide:      FVisible = false; break;
            case OSEvent::Close:
                if (FOnClose) FOnClose(this, e.cancel);
                return;
            default:
                inherited::OnOSEvent(e);
                return;
            }
        }

        const char* ClassName() const override { return "TOSControl"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TOSControl" || inherited::InheritsFrom(cls);
        }
    };

    // ============================================================================
    //  TCustomForm
    // ============================================================================
    class TCustomForm : public TOSControl {
        INHERITED(TOSControl);
    private:
        std::unique_ptr<TCanvas> Canvas;
        TNotifyEvent FOnHide;
        TNotifyEvent FOnPaint;
        TNotifyEvent FOnShow;
    public:
        TNotifyEvent& OnHide() { return FOnHide; }
        TNotifyEvent& OnPaint() { return FOnPaint; }
        TNotifyEvent& OnShow() { return FOnShow; }

        void CreateCanvas() {
            if (!Canvas) Canvas = FDriver->CreateCanvas(FHandle.get());
        };

        explicit TCustomForm(TComponent* owner) : TOSControl(owner) {}
        ~TCustomForm() override = default;

        void OnOSEvent(OSEvent& e) override {
            switch (e.type) {
            case OSEvent::Show:
                inherited::OnOSEvent(e);
                if (FOnShow) FOnShow(this);
                return;
            case OSEvent::Hide:
                inherited::OnOSEvent(e);
                if (FOnHide) FOnHide(this);
                return;
            case OSEvent::Paint:
                if (FOnPaint) FOnPaint(this);
                PaintTree(Canvas.get());
                break;
            default:
                inherited::OnOSEvent(e);
                return;
            }
        }
    };

    // ============================================================================
    //  TForm
    // ============================================================================
    class TForm : public TCustomForm {
        INHERITED(TCustomForm);
    public:
        explicit TForm(TComponent* owner) : TCustomForm(owner) {}
        ~TForm() override = default;

        ControlKind Kind() const override { return ControlKind::Form; }

        void CreateHandle() {
            inherited::CreateHandle(nullptr);
            CreateHandlesRecursive(FHandle.get());
        }

        void Show() { inherited::SetVisible(true); }
        void Hide() { inherited::SetVisible(false); }

        void PaintTree(TCanvas* c) override {
            if (!c) return;
            c->Line(0, 0, 400, 400);
            inherited::PaintTree(c);
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
            if (!FHandle) return;
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
    //  TApplication
    // ============================================================================
    class TApplication : public TComponent {
        INHERITED(TComponent);
        IOSDriver* FDriver = nullptr;     // НЕ владеет
        TForm* FMainForm = nullptr;       // ссылка; владение — через FOwnedComponents
        std::string FTitle;
    public:

        // Заглушки в стиле VCL
        bool MainFormOnTaskBar = false;
        void Initialize() {}

        TApplication(TComponent* owner) : TComponent(owner) {}

        ~TApplication() override = default;

        void SetDriver(IOSDriver* d) { FDriver = d; }
        IOSDriver* Driver() const { return FDriver; }

        const std::string& Title() const { return FTitle; }
        void SetTitle(const std::string& t) { FTitle = t; }

        void CreateForm(TForm* f) { FMainForm = f; }
        TForm* MainForm() const { return FMainForm; }

        // Показ исключения — в стиле VCL ShowException.
        void ShowException(Exception* e) {
            if (!e) return;
            std::string msg = e->ToString();
            std::cerr << "[Application Error] " << msg << "\n";
        }

        int Run() {
            if (!FDriver) {
                std::cerr << "[TApplication] No driver set!\n";
                return 1;
            }
            if (!FMainForm) {
                std::cerr << "[TApplication] No main form!\n";
                return 1;
            }

            FMainForm->SetDriver(FDriver);
            FMainForm->DistributeDriverRecursive();

            FMainForm->CreateHandle();
            FMainForm->CreateCanvas();
            FMainForm->Show();

            return FDriver->RunMessageLoop();
        }
    };

} // namespace vcl

// ============================================================================
//  Windows-драйвер
// ============================================================================
#if defined(_WIN32)

// ============================================================================
//  UTF-8 <-> UTF-16 helpers — ВЫНЕСЕНЫ ВЫШЕ, чтобы их видел ShowException.
// ============================================================================
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <windowsx.h>
#  undef min
#  undef max

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

#include "include/wil/resource.h"

using namespace vcl;

inline void ShowMessage(const wchar_t* msg)
{
    MessageBoxW(nullptr, msg, L"Ошибка", MB_ICONERROR);
}

class TWindowsCanvas : public TCanvas {
    INHERITED(TCanvas);
    HWND     FHwnd = nullptr;

    wil::unique_hdc_window FDc;
    wil::unique_hbrush FBrush;
    wil::unique_hpen   FPen;
    COLORREF FColor = RGB(0, 0, 0);

public:
    TWindowsCanvas(HWND hwnd) : FHwnd(hwnd), FDc(wil::GetDC(hwnd)) {}
    ~TWindowsCanvas() override {}

    void SetColor(TColor c) override {
        FColor = RGB(c.r, c.g, c.b);
        FBrush.reset(CreateSolidBrush(FColor));
        FPen.reset(CreatePen(PS_SOLID, 1, FColor));
    }

    void FillRect(int l, int t, int w, int h) override {
        RECT r{ l, t, l + w, t + h };
        HBRUSH b = FBrush ? FBrush.get() : (HBRUSH)GetStockObject(BLACK_BRUSH);
        ::FillRect(FDc.get(), &r, b);
    }

    void DrawRect(int l, int t, int w, int h) override {
        HBRUSH oldB = (HBRUSH)SelectObject(FDc.get(), GetStockObject(NULL_BRUSH));
        HPEN   oldP = (HPEN)SelectObject(FDc.get(), FPen ? FPen.get()
            : GetStockObject(BLACK_PEN));
        ::Rectangle(FDc.get(), l, t, l + w, t + h);
        SelectObject(FDc.get(), oldB);
        SelectObject(FDc.get(), oldP);
    }

    void DrawTextOut(int x, int y, const std::string& text) override {
        SetTextColor(FDc.get(), FColor);
        SetBkMode(FDc.get(), TRANSPARENT);
        std::wstring w = Utf8ToW(text);
        ::TextOutW(FDc.get(), x, y, w.c_str(), (int)w.size());
    }

    void Line(int x1, int y1, int x2, int y2) override {
        HPEN oldP = (HPEN)SelectObject(FDc.get(), FPen ? FPen.get()
            : GetStockObject(BLACK_PEN));
        MoveToEx(FDc.get(), x1, y1, nullptr);
        ::LineTo(FDc.get(), x2, y2);
        SelectObject(FDc.get(), oldP);
    }

    void Clear(TColor c) override {
        RECT r;
        GetClientRect(FHwnd, &r);
        wil::unique_hbrush b(CreateSolidBrush(RGB(c.r, c.g, c.b)));
        ::FillRect(FDc.get(), &r, b.get());
    }
};

class ITWindowsDriver {
public:
    struct Win : IOSHandle {
        wil::unique_hwnd hwnd;
        ITWindowsDriver* driver = nullptr;
        IEventSink* sink = nullptr;
        ControlKind      kind = ControlKind::Panel;
        int              id = 0;
        bool             isForm = false;

        ~Win() override = default;
    };

    std::set<std::wstring> FRegisteredClasses;
    HINSTANCE              FInst = nullptr;
    int                    FNextId = 1000;

    virtual ~ITWindowsDriver() = default;

    static void AttachWin(HWND hwnd, Win* w) {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)w);
    }
    static Win* WinOf(HWND hwnd) {
        return reinterpret_cast<Win*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    void EnsureClass(const wchar_t* cls) {
        if (!cls) return;
        if (FRegisteredClasses.count(cls)) return;

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = &ITWindowsDriver::WndProc;
        wc.hInstance = FInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = cls;

        if (!RegisterClassExW(&wc) &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return;
        }

        FRegisteredClasses.insert(cls);
    }

    std::unique_ptr<Win> CreateWin(const ControlDesc& d,
        const wchar_t* cls,
        DWORD style, DWORD exStyle,
        HWND parent,
        int x, int y, int w, int h,
        int ctrlId)
    {
        auto win = std::make_unique<Win>();
        win->driver = this;
        win->kind = d.kind;
        win->id = d.id;
        win->isForm = (d.kind == ControlKind::Form);

        win->hwnd.reset(::CreateWindowExW(
            exStyle, cls, Utf8ToW(d.caption).c_str(), style,
            x, y, w, h, parent, (HMENU)(INT_PTR)ctrlId,
            FInst, nullptr));

        if (!win->hwnd) return nullptr;

        AttachWin(win->hwnd.get(), win.get());
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
        Win* w = WinOf(hwnd);
        if (!w || !w->driver) return DefWindowProcW(hwnd, msg, wp, lp);

        ITWindowsDriver* drv = w->driver;

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
            if (!GetParent(hwnd)) PostQuitMessage(0);
            return 0;

        case WM_NCDESTROY:
            if (w) {
                AttachWin(hwnd, nullptr);
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

// ============================================================================
//  TWindowsDriver
// ============================================================================
class TWindowsDriver : public IOSDriver, public ITWindowsDriver {

public:
    explicit TWindowsDriver(HINSTANCE hInstance) {
        FInst = hInstance ? hInstance : GetModuleHandleW(nullptr);
        Init();
    }

    ~TWindowsDriver() override {
        Shutdown();
    }

    TWindowsDriver(const TWindowsDriver&) = delete;
    TWindowsDriver& operator=(const TWindowsDriver&) = delete;

    const char* Name() const override { return "Windows"; }

    void Init() override {}
    void Shutdown() override {}

    std::unique_ptr<IOSHandle> CreateControl(const ControlDesc& d) override {
        HWND parent = d.parent ? static_cast<Win*>(d.parent)->hwnd.get() : nullptr;

        DWORD style = 0, exStyle = 0;
        const wchar_t* cls = nullptr;

        switch (d.kind) {
        case ControlKind::Form:
            cls = L"VCLFormClass";
            style = WS_OVERLAPPEDWINDOW;
            EnsureClass(cls);
            break;
        case ControlKind::Panel:
            cls = L"VCLPanelClass";
            style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
            EnsureClass(cls);
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
        auto* w = static_cast<Win*>(h);
        if (!w) return;
        w->sink = sink;
    }

    static IEventSink* SinkForHwnd(HWND h) {
        Win* w = WinOf(h);
        return w ? w->sink : nullptr;
    }

    std::unique_ptr<TCanvas> CreateCanvas(IOSHandle* h) override {
        auto* w = static_cast<Win*>(h);
        if (!w || !w->hwnd) return nullptr;
        return std::make_unique<TWindowsCanvas>(w->hwnd.get());
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

    void OnCommand(HWND, Win*, WORD code, HWND child, WORD) override {
        if (!child) return;
        auto* sink = SinkForHwnd(child);
        if (!sink) return;

        OSEvent e; e.key = (int)code;
        switch (code) {
        case BN_CLICKED:
            e.type = OSEvent::Command;
            sink->OnOSEvent(e);
            break;
        case EN_CHANGE:
            if (SendMessage(child, EM_GETMODIFY, 0, 0))
            {
                e.type = OSEvent::Change;
                sink->OnOSEvent(e);
            }
            break;
        case CBN_SELCHANGE:
            e.type = OSEvent::Change;
            sink->OnOSEvent(e);
            break;
        case CBN_EDITCHANGE:
            if (GetFocus() == child)
            {
                e.type = OSEvent::Change;
                sink->OnOSEvent(e);
            }
            break;
        default:
            break;
        }
    }

    bool OnEraseBackground(HWND hwnd, Win* w, HDC dc) override {
        if (!w) return false;
        if (w->kind != ControlKind::Panel && w->kind != ControlKind::Form)
            return false;

        RECT rc;
        GetClientRect(hwnd, &rc);
        wil::unique_hbrush br(
            CreateSolidBrush(GetSysColor(COLOR_BTNFACE)));
        ::FillRect(dc, &rc, br.get());
        return true;
    }

    template <class F>
    static void Emit(Win* w, OSEvent::Type type, F&& setup) {
        if (!w || !w->sink) return;
        OSEvent e;
        e.type = type;
        setup(e);
        w->sink->OnOSEvent(e);
    }

    bool OnClose(HWND, Win* w) override {
        if (!w || !w->sink) return false;
        OSEvent e;
        e.type = OSEvent::Close;
        w->sink->OnOSEvent(e);
        return e.cancel;
    }

    void OnPaint(HWND, Win* w, HDC dc) override {
        Emit(w, OSEvent::Paint, [](OSEvent&) {});
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

// ============================================================================
//  TForm1 — "сгенерированная IDE" форма.
// ============================================================================
using namespace vcl;

class TForm1 : public TForm {
    INHERITED(TForm);
public:
    explicit TForm1(TComponent* owner) : TForm(owner) {
        SetCaption("Hello VCL (Win32)");
        SetBounds(200, 200, 480, 320);

        OnClose() = [](TObject*, bool& CanClose) {
            int r = MessageBoxW(nullptr,
                L"Точно закрыть приложение?",
                L"Подтверждение",
                MB_YESNO | MB_ICONQUESTION);
            CanClose = r == IDNO;
            };

        auto* panel = new TPanel(this);
        panel->SetParent(this);
        panel->SetBounds(10, 10, 460, 80);

        auto* label = new TLabel(panel);
        label->SetParent(panel);
        label->SetBounds(20, 30, 400, 24);
        label->SetCaption("Press the button!");

        auto* button = new TButton(this);
        button->SetParent(this);
        button->SetBounds(20, 120, 160, 40);
        button->SetCaption("Click me");
        button->OnClick() = [label](TObject*) {
            label->SetCaption("Clicked at " + std::to_string(GetTickCount64()));
            };

        auto* chk = new TCheckBox(this);
        chk->SetParent(this);
        chk->SetBounds(200, 120, 200, 30);
        chk->SetCaption("Check me");
        chk->OnChange() = [chk](TObject*) {
            (void)chk->Checked();
            };

        auto* combo = new TComboBox(this);
        combo->SetParent(this);
        combo->SetBounds(20, 180, 200, 200);
        combo->AddItem("Москва");
        combo->AddItem("Петербург");
        combo->AddItem("Новосибирск");
        combo->OnChange() = [combo](TObject*) {
            int idx = combo->SelectedIndex();
            std::wstring elem = L"Индекс элемента:" + std::to_wstring(idx);
            ShowMessage(elem.c_str());
            };

        auto* edit = new TEdit(this);
        edit->SetParent(this);
        edit->SetBounds(20, 230, 250, 25);
        edit->SetText("Введите текст...");
        edit->OnChange() = [edit](TObject*) {
            std::wstring s = Utf8ToW(edit->Text());
            ShowMessage(s.c_str());
            };
    }

    ~TForm1() override = default;

    const char* ClassName() const override { return "TForm1"; }
    bool InheritsFrom(const char* cls) const override {
        return std::string(cls) == "TForm1" || inherited::InheritsFrom(cls);
    }
};

// ============================================================================
//  Глобальные объекты — как в IDE-сгенерированном коде VCL.
// ============================================================================
std::unique_ptr<TApplication> Application;
TForm1* Form1 = nullptr;

// ============================================================================
//  _tWinMain — билдеровский вход в стиле IDE.
// ============================================================================
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    try
    {
        Application->Initialize();
        Application->MainFormOnTaskBar = true;
        Application->CreateForm(Form1);
        Application->Run();
    }
    catch (Exception& exception)
    {
        Application->ShowException(&exception);
    }
    catch (...)
    {
        try
        {
            throw Exception("");
        }
        catch (Exception& exception)
        {
            Application->ShowException(&exception);
        }
    }
    return 0;
}

// ============================================================================
//  wWinMain — настоящая точка входа CRT.
// ============================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    TWindowsDriver driver(hInstance);
    Application = std::make_unique<TApplication>(nullptr);
    Application->SetDriver(&driver);
    Application->SetTitle("VCL Demo");
    Form1 = new TForm1(Application.get());
    return _tWinMain(hInstance, nullptr, nullptr, 0);
}