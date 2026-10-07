// ============================================================================
//
//  OpenRTL — свободное ядро RAD-разработки на C++
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

    // ---------------------------------------------------------------------------
    //  TStyleColor — RGB + флаг "задано"
    // ---------------------------------------------------------------------------
    struct TStyleColor {
        uint8_t r = 0, g = 0, b = 0;
        bool    valid = false;

        static TStyleColor FromRGB(uint8_t R, uint8_t G, uint8_t B) {
            return { R, G, B, true };
        }
        static TStyleColor None() { return { 0, 0, 0, false }; }
    };

    // ---------------------------------------------------------------------------
    //  TStyleFont
    // ---------------------------------------------------------------------------
    struct TStyleFont {
        std::string face = "Segoe UI";
        int         size = 9;
        bool        bold = false;
        bool        italic = false;
        bool        valid = false;
    };

    // ---------------------------------------------------------------------------
    //  TStyleMetrics
    // ---------------------------------------------------------------------------
    struct TStyleMetrics {
        int  borderWidth = 1;
        int  cornerRadius = 0;
        int  padding = 4;
        int  controlHeight = 26;
        bool valid = false;
    };

    // ---------------------------------------------------------------------------
    //  Роли стиля
    // ---------------------------------------------------------------------------
    namespace StyleRole {
        inline constexpr const char* FormBackground = "Form.Background";
        inline constexpr const char* PanelBackground = "Panel.Background";
        inline constexpr const char* LabelText = "Label.Text";
        inline constexpr const char* EditBackground = "Edit.Background";
        inline constexpr const char* EditText = "Edit.Text";
        inline constexpr const char* EditBorder = "Edit.Border";
        inline constexpr const char* ButtonFace = "Button.Face";
        inline constexpr const char* ButtonText = "Button.Text";
        inline constexpr const char* ButtonHot = "Button.Hot";
        inline constexpr const char* ButtonPressed = "Button.Pressed";
        inline constexpr const char* ComboBackground = "Combo.Background";
        inline constexpr const char* ComboText = "Combo.Text";
        inline constexpr const char* Accent = "Accent";
        inline constexpr const char* Divider = "Divider";

        inline constexpr const char* FontDefault = "Font.Default";
        inline constexpr const char* FontLabel = "Font.Label";
        inline constexpr const char* FontButton = "Font.Button";
        inline constexpr const char* FontTitle = "Font.Title";

        inline constexpr const char* MetricsControl = "Metrics.Control";
    }

    // ---------------------------------------------------------------------------
    //  TStyle
    // ---------------------------------------------------------------------------
    class TStyle {
    public:
        TStyle() = default;
        explicit TStyle(const std::string& name) : FName(name) {}

        const std::string& GetName() const { return FName; }
        void SetName(const std::string& n) { FName = n; }

        void SetColor(const std::string& role, TStyleColor c) {
            FColors[role] = c;
        }
        TStyleColor GetColor(const std::string& role,
            TStyleColor fallback = TStyleColor::None()) const {
            auto it = FColors.find(role);
            return it == FColors.end() ? fallback : it->second;
        }
        bool HasColor(const std::string& role) const {
            return FColors.find(role) != FColors.end();
        }

        void SetFont(const std::string& role, const TStyleFont& f) {
            FFonts[role] = f;
        }
        TStyleFont GetFont(const std::string& role, TStyleFont fallback = {}) const {
            auto it = FFonts.find(role);
            return it == FFonts.end() ? fallback : it->second;
        }

        void SetMetrics(const std::string& role, const TStyleMetrics& m) {
            FMetrics[role] = m;
        }
        TStyleMetrics GetMetrics(const std::string& role,
            TStyleMetrics fallback = {}) const {
            auto it = FMetrics.find(role);
            return it == FMetrics.end() ? fallback : it->second;
        }

        void MergeFrom(const TStyle& base) {
            for (auto& kv : base.FColors)
                if (!HasColor(kv.first)) FColors[kv.first] = kv.second;
            for (auto& kv : base.FFonts)
                if (FFonts.find(kv.first) == FFonts.end())
                    FFonts[kv.first] = kv.second;
            for (auto& kv : base.FMetrics)
                if (FMetrics.find(kv.first) == FMetrics.end())
                    FMetrics[kv.first] = kv.second;
        }

    private:
        std::string FName;
        std::map<std::string, TStyleColor>   FColors;
        std::map<std::string, TStyleFont>    FFonts;
        std::map<std::string, TStyleMetrics> FMetrics;
    };

    // ---------------------------------------------------------------------------
    //  Встроенные темы
    // ---------------------------------------------------------------------------
    inline std::shared_ptr<TStyle> MakeLightStyle() {
        auto s = std::make_shared<TStyle>("Light");
        s->SetColor(StyleRole::FormBackground, TStyleColor::FromRGB(250, 250, 250));
        s->SetColor(StyleRole::PanelBackground, TStyleColor::FromRGB(240, 244, 248));
        s->SetColor(StyleRole::LabelText, TStyleColor::FromRGB(28, 28, 30));
        s->SetColor(StyleRole::EditBackground, TStyleColor::FromRGB(255, 255, 255));
        s->SetColor(StyleRole::EditText, TStyleColor::FromRGB(20, 20, 20));
        s->SetColor(StyleRole::EditBorder, TStyleColor::FromRGB(200, 205, 210));
        s->SetColor(StyleRole::ButtonFace, TStyleColor::FromRGB(238, 242, 247));
        s->SetColor(StyleRole::ButtonText, TStyleColor::FromRGB(20, 20, 20));
        s->SetColor(StyleRole::ButtonHot, TStyleColor::FromRGB(225, 235, 248));
        s->SetColor(StyleRole::ButtonPressed, TStyleColor::FromRGB(210, 225, 245));
        s->SetColor(StyleRole::ComboBackground, TStyleColor::FromRGB(255, 255, 255));
        s->SetColor(StyleRole::ComboText, TStyleColor::FromRGB(20, 20, 20));
        s->SetColor(StyleRole::Accent, TStyleColor::FromRGB(0, 120, 215));
        s->SetColor(StyleRole::Divider, TStyleColor::FromRGB(210, 215, 220));

        s->SetFont(StyleRole::FontDefault, TStyleFont{ "Segoe UI",  9, false, false, true });
        s->SetFont(StyleRole::FontLabel, TStyleFont{ "Segoe UI",  9, false, false, true });
        s->SetFont(StyleRole::FontButton, TStyleFont{ "Segoe UI",  9, false, false, true });
        s->SetFont(StyleRole::FontTitle, TStyleFont{ "Segoe UI", 12, true,  false, true });

        s->SetMetrics(StyleRole::MetricsControl, TStyleMetrics{ 1, 0, 4, 26, true });
        return s;
    }

    inline std::shared_ptr<TStyle> MakeDarkStyle() {
        auto s = std::make_shared<TStyle>("Dark");
        s->SetColor(StyleRole::FormBackground, TStyleColor::FromRGB(32, 34, 38));
        s->SetColor(StyleRole::PanelBackground, TStyleColor::FromRGB(42, 46, 52));
        s->SetColor(StyleRole::LabelText, TStyleColor::FromRGB(230, 232, 236));
        s->SetColor(StyleRole::EditBackground, TStyleColor::FromRGB(52, 56, 62));
        s->SetColor(StyleRole::EditText, TStyleColor::FromRGB(235, 238, 242));
        s->SetColor(StyleRole::EditBorder, TStyleColor::FromRGB(90, 96, 104));
        s->SetColor(StyleRole::ButtonFace, TStyleColor::FromRGB(60, 66, 74));
        s->SetColor(StyleRole::ButtonText, TStyleColor::FromRGB(235, 238, 242));
        s->SetColor(StyleRole::ButtonHot, TStyleColor::FromRGB(74, 84, 96));
        s->SetColor(StyleRole::ButtonPressed, TStyleColor::FromRGB(46, 54, 64));
        s->SetColor(StyleRole::ComboBackground, TStyleColor::FromRGB(52, 56, 62));
        s->SetColor(StyleRole::ComboText, TStyleColor::FromRGB(235, 238, 242));
        s->SetColor(StyleRole::Accent, TStyleColor::FromRGB(90, 170, 255));
        s->SetColor(StyleRole::Divider, TStyleColor::FromRGB(80, 86, 94));

        s->SetFont(StyleRole::FontDefault, TStyleFont{ "Segoe UI",  9, false, false, true });
        s->SetFont(StyleRole::FontLabel, TStyleFont{ "Segoe UI",  9, false, false, true });
        s->SetFont(StyleRole::FontButton, TStyleFont{ "Segoe UI",  9, false, false, true });
        s->SetFont(StyleRole::FontTitle, TStyleFont{ "Segoe UI", 12, true,  false, true });

        s->SetMetrics(StyleRole::MetricsControl, TStyleMetrics{ 1, 0, 4, 26, true });
        return s;
    }

    inline std::shared_ptr<TStyle> MakeClassicStyle() {
        auto s = std::make_shared<TStyle>("Classic");
        s->SetColor(StyleRole::FormBackground, TStyleColor::FromRGB(212, 208, 200));
        s->SetColor(StyleRole::PanelBackground, TStyleColor::FromRGB(212, 208, 200));
        s->SetColor(StyleRole::LabelText, TStyleColor::FromRGB(0, 0, 0));
        s->SetColor(StyleRole::EditBackground, TStyleColor::FromRGB(255, 255, 255));
        s->SetColor(StyleRole::EditText, TStyleColor::FromRGB(0, 0, 0));
        s->SetColor(StyleRole::EditBorder, TStyleColor::FromRGB(128, 128, 128));
        s->SetColor(StyleRole::ButtonFace, TStyleColor::FromRGB(212, 208, 200));
        s->SetColor(StyleRole::ButtonText, TStyleColor::FromRGB(0, 0, 0));
        s->SetColor(StyleRole::ButtonHot, TStyleColor::FromRGB(228, 224, 216));
        s->SetColor(StyleRole::ButtonPressed, TStyleColor::FromRGB(192, 188, 180));
        s->SetColor(StyleRole::ComboBackground, TStyleColor::FromRGB(255, 255, 255));
        s->SetColor(StyleRole::ComboText, TStyleColor::FromRGB(0, 0, 0));
        s->SetColor(StyleRole::Accent, TStyleColor::FromRGB(10, 36, 106));
        s->SetColor(StyleRole::Divider, TStyleColor::FromRGB(128, 128, 128));

        s->SetFont(StyleRole::FontDefault, TStyleFont{ "MS Sans Serif",  8, false, false, true });
        s->SetFont(StyleRole::FontLabel, TStyleFont{ "MS Sans Serif",  8, false, false, true });
        s->SetFont(StyleRole::FontButton, TStyleFont{ "MS Sans Serif",  8, false, false, true });
        s->SetFont(StyleRole::FontTitle, TStyleFont{ "MS Sans Serif", 10, true,  false, true });

        s->SetMetrics(StyleRole::MetricsControl, TStyleMetrics{ 1, 0, 2, 24, true });
        return s;
    }

    // ---------------------------------------------------------------------------
    //  TStyleManager
    // ---------------------------------------------------------------------------
    class TStyleManager {
    public:
        using TStyleChangeEvent = std::function<void(const TStyle&)>;

        TStyleManager() = default;
        ~TStyleManager() = default;

        TStyleManager(const TStyleManager&) = delete;
        TStyleManager& operator=(const TStyleManager&) = delete;

        void RegisterStyle(const std::string& name, std::shared_ptr<TStyle> s) {
            if (!s) return;
            s->SetName(name);
            FStyles[name] = std::move(s);
        }

        std::shared_ptr<TStyle> GetStyle(const std::string& name) const {
            auto it = FStyles.find(name);
            return it == FStyles.end() ? nullptr : it->second;
        }

        std::vector<std::string> GetStyleNames() const {
            std::vector<std::string> names;
            names.reserve(FStyles.size());
            for (auto& kv : FStyles) names.push_back(kv.first);
            return names;
        }

        void SetActiveStyle(const std::string& name) {
            auto s = GetStyle(name);
            if (!s) return;
            FActiveName = name;
            FActive = s;
            NotifyChange();
        }

        std::shared_ptr<TStyle> GetActiveStyle() const { return FActive; }
        const std::string& GetActiveStyleName() const { return FActiveName; }

        std::size_t Subscribe(TStyleChangeEvent e) {
            std::size_t id = FNextSubId++;
            FSubs[id] = std::move(e);
            return id;
        }
        void Unsubscribe(std::size_t id) { FSubs.erase(id); }

        void NotifyChange() {
            if (!FActive) return;

            std::shared_ptr<TStyle> active = FActive;

            std::vector<TStyleChangeEvent> snapshot;
            snapshot.reserve(FSubs.size());
            for (auto& kv : FSubs)
                snapshot.push_back(kv.second);

            for (auto& fn : snapshot)
                if (fn) fn(*active);
        }

        void InstallBuiltins() {
            RegisterStyle("Light", MakeLightStyle());
            RegisterStyle("Dark", MakeDarkStyle());
            RegisterStyle("Classic", MakeClassicStyle());
            if (!FActive) SetActiveStyle("Light");
        }

    private:
        std::map<std::string, std::shared_ptr<TStyle>> FStyles;
        std::map<std::size_t, TStyleChangeEvent>       FSubs;
        std::size_t        FNextSubId = 1;
        std::string        FActiveName;
        std::shared_ptr<TStyle> FActive;
    };

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

        const std::string& GetMessage() const { return FMessage; }
        void SetMessage(const std::string& m) { FMessage = m; }

        int  GetHelpContext() const { return FHelpContext; }
        void SetHelpContext(int h) { FHelpContext = h; }

        const Exception* GetInnerException() const { return FInnerException.get(); }

        Exception* GetBaseException() {
            Exception* e = this;
            while (e->FInnerException)
                e = e->FInnerException.get();
            return e;
        }
        const Exception* GetBaseException() const {
            return const_cast<Exception*>(this)->GetBaseException();
        }

        const std::string& GetStackTrace() const { return FStackTrace; }
        void SetStackTrace(const std::string& s) { FStackTrace = s; }

        virtual const char* ClassName() const { return "Exception"; }

        virtual std::string ToString() const {
            std::ostringstream os;
            os << ClassName() << ": " << FMessage;

            const Exception* inner = FInnerException.get();
            while (inner) {
                os << "\n  caused by " << inner->ClassName()
                    << ": " << inner->GetMessage();
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

        TComponent* GetOwner() const { return FOwner; }

        const std::string& GetName() const { return FName; }
        void SetName(const std::string& n) { FName = n; }

        TComponent* FindComponent(const std::string& name) {
            for (auto&& c : FOwnedComponents) {
                if (c->FName == name) return c.get();
                if (auto* r = c->FindComponent(name)) return r;
            }
            return nullptr;
        }

        std::size_t GetOwnedCount() const { return FOwnedComponents.size(); }

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
        IEventSink* sink = nullptr;
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

        virtual const char* GetName() const = 0;
        virtual int  RunMessageLoop() = 0;

        virtual void SetBounds(IOSHandle* h, int l, int t, int w, int ht) = 0;
        virtual void GetBounds(IOSHandle* h, int& l, int& t, int& w, int& ht) const = 0;

        virtual void SetVisible(IOSHandle* h, bool v) = 0;
        virtual bool GetVisible(IOSHandle* h) const = 0;

        virtual void SetEnabled(IOSHandle* h, bool e) = 0;
        virtual bool GetEnabled(IOSHandle* h) const = 0;

        virtual void SetText(IOSHandle* h, const std::string& text) = 0;
        virtual std::string GetText(IOSHandle* h) const = 0;

        virtual void SetCaption(IOSHandle* h, const std::string& text) = 0;
        virtual std::string GetCaption(IOSHandle* h) const = 0;

        virtual void Invalidate(IOSHandle* h) = 0;
        virtual void Update(IOSHandle* h) = 0;

        virtual void BringToFront(IOSHandle* h) = 0;
        virtual void SendToBack(IOSHandle* h) = 0;

        virtual void SetFocus(IOSHandle* h) = 0;
        virtual bool HasFocus(IOSHandle* h) const = 0;

        virtual void SetCheck(IOSHandle* h, bool c) = 0;
        virtual bool GetCheck(IOSHandle* h) const = 0;

        virtual void AddString(IOSHandle* h, const std::string& s) = 0;
        virtual void ClearStrings(IOSHandle* h) = 0;
        virtual int  GetCount(IOSHandle* h) const = 0;
        virtual std::string GetString(IOSHandle* h, int idx) const = 0;

        virtual void SetSel(IOSHandle* h, int idx) = 0;
        virtual int  GetSel(IOSHandle* h) const = 0;

        virtual void SetParent(IOSHandle* h, IOSHandle* parent) = 0;
        virtual IOSHandle* GetParent(IOSHandle* h) const = 0;

        virtual void SetId(IOSHandle* h, int id) = 0;
        virtual int  GetId(IOSHandle* h) const = 0;
        virtual void SetFont(IOSHandle* h, const std::string& face,
            int size, bool bold, bool italic) = 0;
        virtual void Close(IOSHandle* h) = 0;
        virtual void ApplyStyle(IOSHandle* h, const TStyle& style, ControlKind kind) = 0;
    };

    // ============================================================================
    //  TEventDispatcher — поле TControl, реализует IEventSink.
    // ============================================================================
    class TEventDispatcher final : public IEventSink {
        using TNotify = std::function<void(OSEvent&)>;
    public:

        TEventDispatcher() = default;
        ~TEventDispatcher() override = default;

        TEventDispatcher(const TEventDispatcher&) = delete;
        TEventDispatcher& operator=(const TEventDispatcher&) = delete;

        void SetNotify(TNotify n) { FNotify = std::move(n); }
        void ClearNotify() { FNotify = nullptr; }

        void OnOSEvent(OSEvent& e) override {
            if (FNotify) FNotify(e);
        }

    private:
        TNotify FNotify;
    };

    // ============================================================================
    //  TControl
    // ============================================================================
    class TControl : public TComponent {
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
        bool FVisible = false;
        bool FEnabled = true;
        std::string FCaption;

        TControl* FParent = nullptr;
        TEventDispatcher FDispatcher;

        std::string FStyleName;
        TStyleManager* FStyleManager = nullptr;
    public:
        explicit TControl(TComponent* owner) : TComponent(owner) {
            FDispatcher.SetNotify([this](OSEvent& e) { OnOSEvent(e); });
        }

        ~TControl() override = default;

        int GetLeft()   const { return FLeft; }
        int GetTop()    const { return FTop; }
        int GetWidth()  const { return FWidth; }
        int GetHeight() const { return FHeight; }

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

        bool IsVisible() const {
            return FVisible && (FParent ? FParent->IsVisible() : true);
        }

        virtual void SetVisible(bool v) {
            if (FVisible == v) return;
            FVisible = v;
            OnVisibleChanged();
        }

        bool GetEnabled() const { return FEnabled; }
        virtual void SetEnabled(bool e) { FEnabled = e; }

        const std::string& GetCaption() const { return FCaption; }
        virtual void SetCaption(const std::string& c) { FCaption = c; }

        TControl* GetParent() const { return FParent; }

        TNotifyEvent& OnResize() { return FOnResize; }
        TNotifyEvent& OnClick() { return FOnClick; }
        TMouseEvent& OnMouseDown() { return FOnMouseDown; }
        TMouseEvent& OnMouseUp() { return FOnMouseUp; }
        TMouseEvent& OnMouseMove() { return FOnMouseMove; }

        TEventDispatcher& Dispatcher() { return FDispatcher; }

        virtual void OnMove() {}
        virtual void OnVisibleChanged() {}
        virtual void OnPaint(TCanvas* Canvas) {}
        virtual void Invalidate() {}

        const std::string& GetStyleName() const { return FStyleName; }
        virtual void SetStyleName(const std::string& n) {
            FStyleName = n;
            ApplyStyleRecursive();
        }

        virtual void SetStyleManager(TStyleManager* m) {
            FStyleManager = m;
        }
        TStyleManager* GetStyleManager() const { return FStyleManager; }

        std::shared_ptr<TStyle> ResolveStyle() const {
            if (!FStyleName.empty() && FStyleManager) {
                auto s = FStyleManager->GetStyle(FStyleName);
                if (s) return s;
            }
            if (FParent) return FParent->ResolveStyle();
            if (FStyleManager) return FStyleManager->GetActiveStyle();
            return nullptr;
        }
        virtual void ApplyStyleRecursive() {}
        const char* ClassName() const override { return "TControl"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TControl" || inherited::InheritsFrom(cls);
        }

        virtual void OnOSEvent(OSEvent& e) {
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
        std::vector<TOSControl*> FChildControls;
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
        std::size_t FStyleSubId = 0;
    public:
        explicit TOSControl(TComponent* owner) : TControl(owner) {}

        ~TOSControl() override {
            if (FStyleSubId && FStyleManager) {
                FStyleManager->Unsubscribe(FStyleSubId);
                FStyleSubId = 0;
            }
            FStyleManager = nullptr;
        }

        void SetStyleManager(TStyleManager* m) override {
            if (FStyleManager == m) return;

            if (FStyleSubId && FStyleManager) {
                FStyleManager->Unsubscribe(FStyleSubId);
                FStyleSubId = 0;
            }

            FStyleManager = m;

            if (FStyleManager) {
                FStyleSubId = FStyleManager->Subscribe(
                    [this](const TStyle&) { ApplyStyleRecursive(); });
            }

            for (auto* c : FChildControls)
                c->SetStyleManager(m);
        }

        TCloseEvent& OnClose() { return FOnClose; }
        TNotifyEvent& OnChange() { return FOnChange; }
        TKeyEvent& OnKeyDown() { return FOnKeyDown; }

        void SetParent(TOSControl* p) {
            if (FParent == p) return;
            if (FParent) RemoveChildControl(this);
            FParent = p;
            if (p) {
                p->AddChildControl(this);
                if (p->GetStyleManager())
                    SetStyleManager(p->GetStyleManager());
            }
        }

        virtual void PaintTree(TCanvas* c) {
            if (!IsVisible()) return;
            OnPaint(c);
            for (auto* child : FChildControls) child->PaintTree(c);
        }

        TOSControl(const TOSControl&) = delete;
        TOSControl& operator=(const TOSControl&) = delete;

        virtual ControlKind GetKind() const { return ControlKind::Panel; }

        void SetDriver(IOSDriver* d) { FDriver = d; }
        IOSDriver* GetDriver() const { return FDriver; }

        virtual void CreateHandle(IOSHandle* parentHandle) {
            if (FHandle) return;
            if (!FDriver) return;

            ControlDesc d;
            d.kind = GetKind();
            d.caption = FCaption;
            d.x = FLeft; d.y = FTop; d.w = FWidth; d.h = FHeight;
            d.visible = IsVisible();
            d.enabled = FEnabled;
            d.id = FId;
            d.parent = parentHandle;
            d.sink = &Dispatcher();

            FHandle = FDriver->CreateControl(d);
            if (!FHandle) return;

            FDriver->SetText(FHandle.get(), FCaption);
            FDriver->SetVisible(FHandle.get(), FVisible);
            FDriver->SetEnabled(FHandle.get(), FEnabled);
            ApplyStyleToHandle();
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
            if (FHandle && FDriver)
                FDriver->SetVisible(FHandle.get(), IsVisible());
            for (auto* c : FChildControls)
                c->SetVisible(v);
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

        void ApplyStyleRecursive() override {
            ApplyStyleToHandle();
            for (auto* c : FChildControls)
                c->ApplyStyleRecursive();
        }

        void ApplyStyleToHandle() {
            if (!FHandle || !FDriver) return;
            auto style = ResolveStyle();
            if (!style) return;
            FDriver->ApplyStyle(FHandle.get(), *style, GetKind());
        }

        void OnOSEvent(OSEvent& e) override {
            switch (e.type) {
            case OSEvent::KeyDown: {
                int key = e.key;
                if (FOnKeyDown) FOnKeyDown(this, key, 0);
                break;
            }
            case OSEvent::Change:
                if (FOnChange) FOnChange(this);
                break;
            case OSEvent::Close:
                if (FOnClose) FOnClose(this, e.cancel);
                return;
            case OSEvent::Show:      FVisible = true;  break;
            case OSEvent::Hide:      FVisible = false; break;
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

        ControlKind GetKind() const override { return ControlKind::Form; }

        void CreateHandle() {
            inherited::CreateHandle(nullptr);
            CreateHandlesRecursive(FHandle.get());
        }

        void Show() { inherited::SetVisible(true); }
        void Hide() { inherited::SetVisible(false); }

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

        ControlKind GetKind() const override { return ControlKind::Label; }
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

        ControlKind GetKind() const override { return ControlKind::Button; }
        const char* ClassName() const override { return "TButton"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TButton" || inherited::InheritsFrom(cls);
        }
    };

    class TCheckBox : public TOSControl {
        INHERITED(TOSControl);
        bool IsChecked = false;
    public:
        explicit TCheckBox(TComponent* owner) : TOSControl(owner) {}
        ~TCheckBox() override = default;

        ControlKind GetKind() const override { return ControlKind::CheckBox; }

        void SetChecked(bool c) {
            IsChecked = c;
            if (FHandle) FDriver->SetCheck(FHandle.get(), IsChecked);
        }
        bool GetChecked() const {
            return FHandle ? FDriver->GetCheck(FHandle.get()) : IsChecked;
        }

        const char* ClassName() const override { return "TCheckBox"; }
        bool InheritsFrom(const char* cls) const override {
            return std::string(cls) == "TCheckBox" || inherited::InheritsFrom(cls);
        }
    };

    class TEdit : public TOSControl {
        INHERITED(TOSControl);
        std::string FText = "";
    public:
        explicit TEdit(TComponent* owner) : TOSControl(owner) {}
        ~TEdit() override = default;

        ControlKind GetKind() const override { return ControlKind::Edit; }

        std::string GetText() const {
            return FHandle ? FDriver->GetText(FHandle.get()) : FText;
        }

        void SetText(const std::string& s) {
            FText = s;
            if (FHandle) FDriver->SetText(FHandle.get(), s);
        }

        void CreateHandle(IOSHandle* parentHandle) override {
            inherited::CreateHandle(parentHandle);
            if (FHandle) FDriver->SetText(FHandle.get(), FText);
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

        ControlKind GetKind() const override { return ControlKind::ComboBox; }

        void AddItem(const std::string& s) {
            if (FHandle) FDriver->AddString(FHandle.get(), s);
            else FPending.push_back(s);
        }
        int  GetSelectedIndex() const {
            return FHandle ? FDriver->GetSel(FHandle.get()) : -1;
        }
        void SetSelectedIndex(int i) {
            if (FHandle) FDriver->SetSel(FHandle.get(), i);
        }

        std::string GetString(int idx) const {
            return FHandle ? FDriver->GetString(FHandle.get(), idx) : std::string{};
        }

        void CreateHandle(IOSHandle* parentHandle) override {
            inherited::CreateHandle(parentHandle);
            if (!FHandle) return;
            for (auto& s : FPending) FDriver->AddString(FHandle.get(), s);
            if (!FPending.empty()) FDriver->SetSel(FHandle.get(), 0);
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

        ControlKind GetKind() const override { return ControlKind::Panel; }

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
        IOSDriver* FDriver = nullptr;
        TStyleManager* FStyleManager = nullptr;
        TForm* FMainForm = nullptr;
        std::string FTitle;
    public:
        bool MainFormOnTaskBar = false;

        TApplication(TComponent* owner) : TComponent(owner) {}

        ~TApplication() override = default;

        // --- Стили (невладеющий указатель) ---
        void SetStyleManager(TStyleManager* m) { FStyleManager = m; }
        TStyleManager* GetStyleManager() const { return FStyleManager; }

        void SetStyle(const std::string& name) {
            if (FStyleManager) FStyleManager->SetActiveStyle(name);
        }
        std::string GetStyle() const {
            return FStyleManager ? FStyleManager->GetActiveStyleName()
                : std::string{};
        }
        void RegisterStyle(std::shared_ptr<TStyle> s) {
            if (!s || !FStyleManager) return;
            FStyleManager->RegisterStyle(s->GetName(), std::move(s));
        }

        virtual void Initialize() {
            if (FStyleManager) FStyleManager->InstallBuiltins();
        }

        // --- Драйвер ---
        void SetDriver(IOSDriver* d) { FDriver = d; }
        IOSDriver* GetDriver() const { return FDriver; }

        const std::string& GetTitle() const { return FTitle; }
        void SetTitle(const std::string& t) { FTitle = t; }

        void CreateForm(TForm* f) { FMainForm = f; }
        TForm* GetMainForm() const { return FMainForm; }

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
            if (!FStyleManager) {
                std::cerr << "[TApplication] No style manager set!\n";
                return 1;
            }
            if (!FMainForm) {
                std::cerr << "[TApplication] No main form!\n";
                return 1;
            }

            FMainForm->SetDriver(FDriver);
            FMainForm->DistributeDriverRecursive();
            FMainForm->SetStyleManager(FStyleManager);

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

inline COLORREF ToCOLORREF(const vcl::TStyleColor& c) {
    return RGB(c.r, c.g, c.b);
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

        COLORREF styleText = CLR_INVALID;
        COLORREF styleBackground = CLR_INVALID;
        COLORREF styleHot = CLR_INVALID;
        COLORREF stylePressed = CLR_INVALID;

        wil::unique_hbrush styleBrush;
        wil::unique_hbrush styleHotBrush;
        wil::unique_hbrush stylePressedBrush;

        bool isHot = false;
        bool isPressed = false;

        void RefreshStyleBrushes() {
            if (styleBackground != CLR_INVALID)
                styleBrush.reset(CreateSolidBrush(styleBackground));
            else
                styleBrush.reset();

            if (styleHot != CLR_INVALID)
                styleHotBrush.reset(CreateSolidBrush(styleHot));
            else
                styleHotBrush.reset();

            if (stylePressed != CLR_INVALID)
                stylePressedBrush.reset(CreateSolidBrush(stylePressed));
            else
                stylePressedBrush.reset();
        }

        ~Win() override = default;
    };

    virtual ~ITWindowsDriver() = default;

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

    static void AttachWin(HWND hwnd, Win* w) {
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)w);
    }
    static Win* WinOf(HWND hwnd) {
        return reinterpret_cast<Win*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

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

        case WM_KEYDOWN:
            drv->OnKeyDown(hwnd, w, (int)wp);
            return 0;

        case WM_KEYUP:
            drv->OnKeyUp(hwnd, w, (int)wp);
            return 0;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN: {
            Win* child = WinOf((HWND)lp);
            if (child) {
                HDC dc = (HDC)wp;
                if (child->styleText != CLR_INVALID)
                    SetTextColor(dc, child->styleText);
                if (child->styleBackground != CLR_INVALID) {
                    SetBkColor(dc, child->styleBackground);
                    SetBkMode(dc, OPAQUE);
                    if (child->styleBrush)
                        return (LRESULT)child->styleBrush.get();
                }
                else {
                    SetBkMode(dc, TRANSPARENT);
                    HWND parent = GetParent((HWND)lp);
                    Win* pw = WinOf(parent);
                    if (pw && pw->styleBrush)
                        return (LRESULT)pw->styleBrush.get();
                }
            }
            break;
        }

        case WM_DRAWITEM: {
            auto* dis = (DRAWITEMSTRUCT*)lp;
            Win* bw = WinOf(dis->hwndItem);
            if (!bw) break;

            bool pressed = (dis->itemState & ODS_SELECTED) != 0;
            bool focused = (dis->itemState & ODS_FOCUS) != 0;

            COLORREF bg = GetSysColor(COLOR_BTNFACE);
            if (pressed) {
                if (bw->stylePressed != CLR_INVALID) bg = bw->stylePressed;
                else if (bw->styleBackground != CLR_INVALID) bg = bw->styleBackground;
            }
            else if (bw->isHot && bw->styleHot != CLR_INVALID) {
                bg = bw->styleHot;
            }
            else if (bw->styleBackground != CLR_INVALID) {
                bg = bw->styleBackground;
            }

            wil::unique_hbrush br(CreateSolidBrush(bg));
            FillRect(dis->hDC, &dis->rcItem, br.get());

            wchar_t buf[512];
            GetWindowTextW(dis->hwndItem, buf, 512);
            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC,
                bw->styleText != CLR_INVALID ? bw->styleText
                : GetSysColor(COLOR_BTNTEXT));
            DrawTextW(dis->hDC, buf, -1, &dis->rcItem,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            if (focused) {
                RECT r = dis->rcItem;
                InflateRect(&r, -2, -2);
                DrawFocusRect(dis->hDC, &r);
            }
            return TRUE;
        }

        case WM_MOUSEMOVE: {
            if (w->kind == ControlKind::Button && !w->isHot) {
                w->isHot = true;
                InvalidateRect(hwnd, nullptr, TRUE);

                TRACKMOUSEEVENT tme{};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
            }
            drv->OnMouseMove(hwnd, w, GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            return 0;
        }

        case WM_MOUSELEAVE: {
            if (w->kind == ControlKind::Button && w->isHot) {
                w->isHot = false;
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;
        }
        }

        return DefWindowProcW(hwnd, msg, wp, lp);
    }
};

// ============================================================================
//  TWindowsDriver
// ============================================================================
class TWindowsDriver : public IOSDriver, public ITWindowsDriver {

    std::set<std::wstring> FRegisteredClasses;
    HINSTANCE              FInst = nullptr;
    int                    FNextId = 1000;

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
        win->sink = d.sink;

        win->hwnd.reset(::CreateWindowExW(
            exStyle, cls, Utf8ToW(d.caption).c_str(), style,
            x, y, w, h, parent, (HMENU)(INT_PTR)ctrlId,
            FInst, nullptr));

        if (!win->hwnd) return nullptr;

        AttachWin(win->hwnd.get(), win.get());
        return win;
    }

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

    const char* GetName() const override { return "Windows"; }

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
            style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW;
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

    inline static HWND HwndOf(IOSHandle* h) noexcept {
        auto* win = static_cast<Win*>(h);
        if (!win) return nullptr;
        return win->hwnd.get();
    }

    void SetBounds(IOSHandle* h, int l, int t, int w, int ht) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::SetWindowPos(hwnd, nullptr, l, t, w, ht,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void GetBounds(IOSHandle* h, int& l, int& t, int& w, int& ht) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        RECT r;
        ::GetWindowRect(hwnd, &r);
        l = r.left; t = r.top;
        w = r.right - r.left;
        ht = r.bottom - r.top;
    }

    void SetVisible(IOSHandle* h, bool v) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::ShowWindow(hwnd, v ? SW_SHOW : SW_HIDE);
    }

    bool GetVisible(IOSHandle* h) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return false;
        return ::IsWindowVisible(hwnd) != FALSE;
    }

    void SetEnabled(IOSHandle* h, bool e) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::EnableWindow(hwnd, e ? TRUE : FALSE);
    }

    bool GetEnabled(IOSHandle* h) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return false;
        return ::IsWindowEnabled(hwnd) != FALSE;
    }

    void SetText(IOSHandle* h, const std::string& text) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::SetWindowTextW(hwnd, Utf8ToW(text).c_str());
    }

    std::string GetText(IOSHandle* h) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return {};
        int len = GetWindowTextLengthW(hwnd);
        if (len <= 0) return {};
        std::wstring s(len + 1, L'\0');
        int got = GetWindowTextW(hwnd, &s[0], len + 1);
        s.resize(got > 0 ? got : 0);
        return WToUtf8(s);
    }

    void SetCaption(IOSHandle* h, const std::string& text) override {
        SetText(h, text);
    }

    std::string GetCaption(IOSHandle* h) const override {
        return GetText(h);
    }

    void Invalidate(IOSHandle* h) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::InvalidateRect(hwnd, nullptr, TRUE);
    }

    void Update(IOSHandle* h) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::UpdateWindow(hwnd);
    }

    void BringToFront(IOSHandle* h) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    void SendToBack(IOSHandle* h) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    void SetFocus(IOSHandle* h) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::SetFocus(hwnd);
    }

    bool HasFocus(IOSHandle* h) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return false;
        return ::GetFocus() == hwnd;
    }

    void SetCheck(IOSHandle* h, bool c) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        SendMessageW(hwnd, BM_SETCHECK, c ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    bool GetCheck(IOSHandle* h) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return false;
        return SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }

    void AddString(IOSHandle* h, const std::string& s) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        std::wstring ws = Utf8ToW(s);
        SendMessageW(hwnd, CB_ADDSTRING, 0, (LPARAM)ws.c_str());
    }

    void ClearStrings(IOSHandle* h) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        SendMessageW(hwnd, CB_RESETCONTENT, 0, 0);
    }

    int GetCount(IOSHandle* h) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return 0;
        return (int)SendMessageW(hwnd, CB_GETCOUNT, 0, 0);
    }

    std::string GetString(IOSHandle* h, int idx) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return {};
        int len = (int)SendMessageW(hwnd, CB_GETLBTEXTLEN, idx, 0);
        if (len < 0) return {};
        std::wstring s(len + 1, L'\0');
        SendMessageW(hwnd, CB_GETLBTEXT, idx, (LPARAM)&s[0]);
        s.resize(len);
        return WToUtf8(s);
    }

    void SetSel(IOSHandle* h, int idx) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        SendMessageW(hwnd, CB_SETCURSEL, idx, 0);
    }

    int GetSel(IOSHandle* h) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return -1;
        return (int)SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
    }

    void SetParent(IOSHandle* h, IOSHandle* parent) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        HWND ph = parent ? HwndOf(parent) : nullptr;
        ::SetParent(hwnd, ph);
    }

    IOSHandle* GetParent(IOSHandle* h) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return nullptr;
        HWND ph = ::GetParent(hwnd);
        if (!ph) return nullptr;
        return reinterpret_cast<IOSHandle*>(WinOf(ph));
    }

    void SetId(IOSHandle* h, int id) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::SetWindowLongPtrW(hwnd, GWLP_ID, id);
    }

    int GetId(IOSHandle* h) const override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return 0;
        return (int)::GetWindowLongPtrW(hwnd, GWLP_ID);
    }

    void SetFont(IOSHandle* h, const std::string& face,
        int size, bool bold, bool italic) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;

        LOGFONTW lf{};
        HDC dc = GetDC(hwnd);
        lf.lfHeight = -MulDiv(size, GetDeviceCaps(dc, LOGPIXELSY), 72);
        ReleaseDC(hwnd, dc);

        lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
        lf.lfItalic = italic ? TRUE : FALSE;
        std::wstring wface = Utf8ToW(face);
        wcsncpy_s(lf.lfFaceName, wface.c_str(), _TRUNCATE);

        HFONT hFont = CreateFontIndirectW(&lf);
        SendMessageW(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
    }

    void Close(IOSHandle* h) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;
        ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
    }

    std::unique_ptr<TCanvas> CreateCanvas(IOSHandle* h) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return nullptr;
        return std::make_unique<TWindowsCanvas>(hwnd);
    }

    static IEventSink* SinkForHwnd(HWND h) {
        Win* w = WinOf(h);
        return w ? w->sink : nullptr;
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

        if (w->styleBackground != CLR_INVALID) {
            if (!w->styleBrush)
                w->styleBrush.reset(CreateSolidBrush(w->styleBackground));
            ::FillRect(dc, &rc, w->styleBrush.get());
        }
        else {
            wil::unique_hbrush br(CreateSolidBrush(GetSysColor(COLOR_BTNFACE)));
            ::FillRect(dc, &rc, br.get());
        }
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

    void OnPaint(HWND, Win* w, HDC) override {
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
        if (w && w->kind == ControlKind::Button) {
            w->isPressed = true;
            InvalidateRect(w->hwnd.get(), nullptr, TRUE);
        }
        Emit(w, OSEvent::MouseDown, [=](OSEvent& e) {
            e.x = x; e.y = y; e.button = button; });
    }

    void OnMouseUp(HWND, Win* w, int x, int y, int button) override {
        if (w && w->kind == ControlKind::Button) {
            w->isPressed = false;
            InvalidateRect(w->hwnd.get(), nullptr, TRUE);
        }
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

    void ApplyStyle(IOSHandle* h, const TStyle& s, ControlKind kind) override {
        HWND hwnd = HwndOf(h);
        if (!hwnd) return;

        Win* w = static_cast<Win*>(h);

        auto bg = [&](const char* role, COLORREF def) -> COLORREF {
            auto c = s.GetColor(role);
            return c.valid ? ToCOLORREF(c) : def;
            };
        auto fg = [&](const char* role, COLORREF def) -> COLORREF {
            auto c = s.GetColor(role);
            return c.valid ? ToCOLORREF(c) : def;
            };
        auto applyFont = [&](const char* role) {
            auto f = s.GetFont(role);
            if (f.valid)
                SetFont(h, f.face, f.size, f.bold, f.italic);
            };

        switch (kind) {
        case ControlKind::Form:
        case ControlKind::Panel: {
            w->styleBackground = bg(StyleRole::PanelBackground,
                GetSysColor(COLOR_BTNFACE));
            w->RefreshStyleBrushes();
            applyFont(StyleRole::FontDefault);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        }
        case ControlKind::Label: {
            w->styleText = fg(StyleRole::LabelText,
                GetSysColor(COLOR_WINDOWTEXT));
            w->styleBackground = bg(StyleRole::PanelBackground, CLR_INVALID);
            w->RefreshStyleBrushes();
            applyFont(StyleRole::FontLabel);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        }
        case ControlKind::Edit: {
            w->styleText = fg(StyleRole::EditText,
                GetSysColor(COLOR_WINDOWTEXT));
            w->styleBackground = bg(StyleRole::EditBackground,
                GetSysColor(COLOR_WINDOW));
            w->RefreshStyleBrushes();
            applyFont(StyleRole::FontDefault);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        }
        case ControlKind::Button: {
            w->styleText = fg(StyleRole::ButtonText,
                GetSysColor(COLOR_BTNTEXT));
            w->styleBackground = bg(StyleRole::ButtonFace,
                GetSysColor(COLOR_BTNFACE));
            w->styleHot = bg(StyleRole::ButtonHot, w->styleBackground);
            w->stylePressed = bg(StyleRole::ButtonPressed, w->styleBackground);
            w->RefreshStyleBrushes();
            applyFont(StyleRole::FontButton);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        }
        case ControlKind::CheckBox: {
            w->styleText = fg(StyleRole::LabelText,
                GetSysColor(COLOR_WINDOWTEXT));
            w->styleBackground = bg(StyleRole::PanelBackground, CLR_INVALID);
            w->RefreshStyleBrushes();
            applyFont(StyleRole::FontLabel);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        }
        case ControlKind::ComboBox:
        case ControlKind::ListBox: {
            w->styleText = fg(StyleRole::ComboText,
                GetSysColor(COLOR_WINDOWTEXT));
            w->styleBackground = bg(StyleRole::ComboBackground,
                GetSysColor(COLOR_WINDOW));
            w->RefreshStyleBrushes();
            applyFont(StyleRole::FontDefault);
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        }
        }
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
        SetCaption("VCL Demo — Enhanced");
        SetBounds(200, 200, 520, 400);

        auto* header = new TPanel(this);
        header->SetParent(this);
        header->SetBounds(0, 0, 520, 50);
        header->SetCaption("");

        auto* title = new TLabel(header);
        title->SetParent(header);
        title->SetBounds(16, 14, 480, 24);
        title->SetCaption("OpenRTL / VCL Demonstration");

        auto* greetLabel = new TLabel(this);
        greetLabel->SetParent(this);
        greetLabel->SetBounds(20, 70, 480, 24);
        greetLabel->SetCaption("Enter your name and click \"Greet\".");

        auto* nameEdit = new TEdit(this);
        nameEdit->SetParent(this);
        nameEdit->SetBounds(20, 105, 320, 26);
        nameEdit->SetText("");

        auto* greetBtn = new TButton(this);
        greetBtn->SetParent(this);
        greetBtn->SetBounds(350, 105, 150, 26);
        greetBtn->SetCaption("Greet");

        auto* politeChk = new TCheckBox(this);
        politeChk->SetParent(this);
        politeChk->SetBounds(20, 145, 300, 24);
        politeChk->SetCaption("Polite form (formal \"you\")");
        politeChk->SetChecked(true);

        auto* cityLabel = new TLabel(this);
        cityLabel->SetParent(this);
        cityLabel->SetBounds(20, 185, 120, 24);
        cityLabel->SetCaption("City:");

        auto* cityCombo = new TComboBox(this);
        cityCombo->SetParent(this);
        cityCombo->SetBounds(140, 182, 260, 200);
        cityCombo->AddItem("Moscow");
        cityCombo->AddItem("Saint Petersburg");
        cityCombo->AddItem("Novosibirsk");
        cityCombo->AddItem("Yekaterinburg");
        cityCombo->AddItem("Kazan");
        cityCombo->SetSelectedIndex(0);

        auto* clearBtn = new TButton(this);
        clearBtn->SetParent(this);
        clearBtn->SetBounds(20, 230, 150, 30);
        clearBtn->SetCaption("Clear");

        auto* exitBtn = new TButton(this);
        exitBtn->SetParent(this);
        exitBtn->SetBounds(180, 230, 150, 30);
        exitBtn->SetCaption("Exit");

        auto* lightBtn = new TButton(this);
        lightBtn->SetParent(this);
        lightBtn->SetBounds(20, 275, 100, 28);
        lightBtn->SetCaption("Light");

        auto* darkBtn = new TButton(this);
        darkBtn->SetParent(this);
        darkBtn->SetBounds(130, 275, 100, 28);
        darkBtn->SetCaption("Dark");

        auto* classicBtn = new TButton(this);
        classicBtn->SetParent(this);
        classicBtn->SetBounds(240, 275, 100, 28);
        classicBtn->SetCaption("Classic");

        auto* status = new TLabel(this);
        status->SetParent(this);
        status->SetBounds(20, 320, 480, 24);
        status->SetCaption("Ready.");

        auto dirty = std::make_shared<bool>(false);

        OnClose() = [dirty](TObject*, bool& CanClose) {
            if (!*dirty) {
                CanClose = false;
                return;
            }
            int r = MessageBoxW(nullptr,
                L"There is unsaved data. Close the application?",
                L"Confirmation",
                MB_YESNO | MB_ICONQUESTION);
            CanClose = (r == IDNO);
            };

        auto updateGreeting = [=]() {
            std::string name = nameEdit->GetText();
            if (name.empty()) {
                greetLabel->SetCaption("Enter your name and click \"Greet\".");
                return;
            }
            std::string city = cityCombo->GetString(cityCombo->GetSelectedIndex());
            std::string who = politeChk->GetChecked() ? "Hello, " : "Hi, ";
            greetLabel->SetCaption(who + name + "!  (city: " + city + ")");
            *dirty = true;
            };

        auto showStatus = [=](const std::string& prefix) {
            std::string name = nameEdit->GetText();
            status->SetCaption(prefix
                + " | name: \"" + (name.empty() ? std::string("<empty>") : name)
                + "\" | city: " + cityCombo->GetString(cityCombo->GetSelectedIndex())
                + " | polite: " + (politeChk->GetChecked() ? "yes" : "no"));
            };

        greetBtn->OnClick() = [=](TObject*) {
            updateGreeting();
            showStatus("Greeting generated");
            };

        clearBtn->OnClick() = [=](TObject*) {
            nameEdit->SetText("");
            politeChk->SetChecked(false);
            cityCombo->SetSelectedIndex(0);
            greetLabel->SetCaption("Enter your name and click \"Greet\".");
            status->SetCaption("Cleared.");
            *dirty = false;
            };

        exitBtn->OnClick() = [=](TObject*) {
            PostMessageW(nullptr, WM_CLOSE, 0, 0);
            };

        nameEdit->OnChange() = [=](TObject*) {
            *dirty = true;
            showStatus("Input");
            };

        politeChk->OnClick() = [=](TObject*) {
            *dirty = true;
            updateGreeting();
            showStatus("Mode changed");
            };

        cityCombo->OnChange() = [=](TObject*) {
            *dirty = true;
            updateGreeting();
            showStatus("City changed");
            };

        // --- Лямбды переключения темы ---
        lightBtn->OnClick() = [=](TObject*) {
            if (auto* app = dynamic_cast<TApplication*>(GetOwner()))
                app->SetStyle("Light");
            status->SetCaption("Theme: Light");
            };

        darkBtn->OnClick() = [=](TObject*) {
            if (auto* app = dynamic_cast<TApplication*>(GetOwner()))
                app->SetStyle("Dark");
            status->SetCaption("Theme: Dark");
            };

        classicBtn->OnClick() = [=](TObject*) {
            if (auto* app = dynamic_cast<TApplication*>(GetOwner()))
                app->SetStyle("Classic");
            status->SetCaption("Theme: Classic");
            };
    }

    void PaintTree(TCanvas* c) override {
        if (!c) return;
        auto style = ResolveStyle();
        TColor color;
        if (style) {
            auto accent = style->GetColor(StyleRole::Accent);
            color = std::move(TColor::FromRGB(accent.r, accent.g, accent.b));
        }
        else {
            color = std::move(TColor::FromRGB(0, 120, 215));
        }
        c->SetColor(color);
        c->Line(0, 50, 520, 50);
        inherited::PaintTree(c);
    }

    ~TForm1() override = default;

    const char* ClassName() const override { return "TForm1"; }
    bool InheritsFrom(const char* cls) const override {
        return std::string(cls) == "TForm1" || inherited::InheritsFrom(cls);
    }
};

std::unique_ptr<TStyleManager> gStyleManager;
std::unique_ptr<IOSDriver>     gDriver;
std::unique_ptr<TApplication>  Application;
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
    gStyleManager = std::make_unique<TStyleManager>();
    gDriver = std::make_unique<TWindowsDriver>(hInstance);
    Application = std::make_unique<TApplication>(nullptr);
    Application->SetStyleManager(gStyleManager.get());
    Application->SetDriver(gDriver.get());
    Application->SetTitle("VCL Demo — Styled");
    Form1 = new TForm1(Application.get());
    return _tWinMain(hInstance, nullptr, nullptr, 0);
}