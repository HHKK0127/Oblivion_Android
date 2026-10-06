#include "menu_xml_interpreter.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <map>
#include <set>

namespace oblivion::ui {

namespace {

constexpr int kMaxEvalDepth = 32;

bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && isWhitespace(s[b])) ++b;
    while (e > b && isWhitespace(s[e - 1])) --e;
    return s.substr(b, e - b);
}

bool isOperatorElement(const std::string& name) {
    // <not> and <rand> are unary value expressions, not accumulator operators,
    // so they are handled by evaluateNode() instead of evaluateSequence().
    static const char* kOps[] = {
        "add", "sub", "mul", "mult", "div", "mod", "max", "min", "floor", "ceil",
        "eq", "gt", "gte", "lt", "lte", "neq", "and", "or"
    };
    for (const char* o : kOps) {
        if (name == o) return true;
    }
    return false;
}

bool isGateElement(const std::string& name) {
    return name == "onlyif" || name == "onlyifnot" || name == "onlynotif";
}

// "name(arg)" -> {"name", "arg"}; "name()" yields an empty argument.
struct SourceCall {
    std::string fn;
    std::string arg;
    bool is_call = false;
};

SourceCall parseSourceCall(const std::string& source) {
    SourceCall call;
    const size_t open = source.find('(');
    if (open == std::string::npos || source.size() < open + 2 ||
        source[source.size() - 1] != ')') {
        return call;
    }
    const std::string fn = trim(source.substr(0, open));
    if (fn.empty()) return call;
    for (char c : fn) {
        if (!std::isalpha(static_cast<unsigned char>(c)) && c != '_') return call;
    }
    call.fn = fn;
    call.arg = trim(source.substr(open + 1, source.size() - open - 2));
    call.is_call = true;
    return call;
}

// Find a direct child of a tile by name.
const WidgetDef* findDirectChild(const WidgetDef& parent, const std::string& name) {
    for (const auto& c : parent.children) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

// Find a direct child tile of a menu by name.
const WidgetDef* findMenuChild(const std::vector<WidgetDef>& widgets, const std::string& name) {
    for (const auto& w : widgets) {
        if (w.name == name) return &w;
    }
    return nullptr;
}

// Find a widget by name anywhere under a menu's widget tree.
const WidgetDef* findWidget(const MenuDef* menu, const std::string& name) {
    if (!menu) return nullptr;
    struct Finder {
        const std::string& name;
        const WidgetDef* result = nullptr;
        void walk(const WidgetDef& w) {
            if (result) return;
            if (w.name == name) { result = &w; return; }
            for (const auto& c : w.children) walk(c);
        }
    } f{name};
    for (const auto& w : menu->widgets) f.walk(w);
    return f.result;
}

// The tile that immediately precedes `w` among its parent's tiles.
const WidgetDef* findPreviousSibling(const WidgetDef& w, const MenuEvalContext& ctx) {
    if (ctx.parent_widget) {
        const auto& siblings = ctx.parent_widget->children;
        for (size_t i = 0; i < siblings.size(); ++i) {
            if (&siblings[i] == &w) return i > 0 ? &siblings[i - 1] : nullptr;
        }
        return nullptr;
    }
    if (ctx.menu_def) {
        const auto& siblings = ctx.menu_def->widgets;
        for (size_t i = 0; i < siblings.size(); ++i) {
            if (&siblings[i] == &w) return i > 0 ? &siblings[i - 1] : nullptr;
        }
    }
    return nullptr;
}

void linkParents(const std::vector<WidgetDef>& widgets, const WidgetDef* parent,
                 std::map<const WidgetDef*, const WidgetDef*>& out) {
    for (const auto& w : widgets) {
        out[&w] = parent;
        linkParents(w.children, &w, out);
    }
}

// Resolve a literal (number / string / &entity;) to a value.
EvalValue literalValue(const std::string& raw, const MenuEvalContext& ctx) {
    const std::string t = trim(raw);
    if (t.empty()) return EvalValue::makeString("");

    if (t.size() >= 2 && t.front() == '&' && t.back() == ';') {
        const std::string name = t.substr(1, t.size() - 2);
        if (name == "true") return EvalValue::makeBool(true);
        if (name == "false") return EvalValue::makeBool(false);
        if (ctx.strings) {
            const auto it = ctx.strings->find(name);
            if (it != ctx.strings->end()) return EvalValue::makeString(it->second);
        }
        return EvalValue::makeString(name);
    }

    char* end = nullptr;
    const float v = std::strtof(t.c_str(), &end);
    if (end && *end == '\0' && end != t.c_str()) return EvalValue::makeNumber(v);
    return EvalValue::makeString(t);
}

const std::string& attrOf(const XmlNode& node, const char* key,
                          const std::string& fallback) {
    const auto it = node.attributes.find(key);
    return it != node.attributes.end() ? it->second : fallback;
}

class Evaluator {
public:
    static EvalValue evaluateNode(const XmlNode& node, const MenuEvalContext& ctx, int depth) {
        if (depth > kMaxEvalDepth) return EvalValue{};
        if (node.name == "copy" || node.name == "ref") {
            const std::string src = attrOf(node, "src", "");
            if (!src.empty()) {
                return traitValue(src, attrOf(node, "trait", ""), ctx, depth + 1);
            }
        }
        // <not> negates its own operand (e.g. <not>&xbox;</not> or
        // <not src="me()" trait="user1"/>); it never touches the accumulator.
        if (node.name == "not") {
            return EvalValue::makeBool(!unaryOperand(node, ctx, depth).toBool());
        }
        // <rand> N </rand> yields a uniform integer in 1..N. Its operand may
        // itself be an expression (e.g. <rand><copy src="parent()" .../></rand>).
        if (node.name == "rand") {
            const int n = static_cast<int>(unaryOperand(node, ctx, depth).toNumber());
            if (n < 1) return EvalValue::makeNumber(0.0f);
            if (n == 1) return EvalValue::makeNumber(1.0f);
            return EvalValue::makeNumber(static_cast<float>(std::rand() % n + 1));
        }
        return evaluateContent(node, ctx, depth);
    }

    // The operand of a unary <not>/<rand>: its `src` trait when present,
    // otherwise its own content.
    static EvalValue unaryOperand(const XmlNode& node, const MenuEvalContext& ctx, int depth) {
        const std::string src = attrOf(node, "src", "");
        if (!src.empty()) {
            return traitValue(src, attrOf(node, "trait", ""), ctx, depth);
        }
        return evaluateContent(node, ctx, depth);
    }

    static EvalValue evaluateContent(const XmlNode& node, const MenuEvalContext& ctx, int depth) {
        if (node.children.empty()) return literalValue(node.text, ctx);
        // Mixed content: the node's own non-whitespace text acts as a leading
        // literal seed (e.g. <x>10<add> 5 </add></x>). Whitespace-only text is
        // ignored so pure operator chains keep their zero seed.
        if (!trim(node.text).empty()) {
            XmlNode seed;
            seed.name = "text";
            seed.text = node.text;
            std::vector<XmlNode> seq;
            seq.reserve(node.children.size() + 1);
            seq.push_back(std::move(seed));
            for (const auto& c : node.children) seq.push_back(c);
            return evaluateSequence(seq, ctx, depth);
        }
        return evaluateSequence(node.children, ctx, depth);
    }

    static EvalValue evaluateSequence(const std::vector<XmlNode>& children,
                                      const MenuEvalContext& ctx, int depth) {
        if (depth > kMaxEvalDepth) return EvalValue{};
        EvalValue acc;
        bool initialized = false;
        bool skipNext = false;

        for (const XmlNode& c : children) {
            if (isGateElement(c.name)) {
                const std::string src = attrOf(c, "src", "");
                const std::string trait = attrOf(c, "trait", "");
                const bool cond = src.empty()
                    ? evaluateNode(c, ctx, depth + 1).toBool()
                    : traitValue(src, trait, ctx, depth + 1).toBool();
                // <onlynotif> is the spelling used by the shipped blue button
                // prefabs and equals <onlyifnot>.
                const bool apply = (c.name == "onlyif") ? cond : !cond;
                skipNext = !apply;
                continue;
            }
            if (skipNext) {
                if (isOperatorElement(c.name)) continue;  // skip this operator
                skipNext = false;
            }
            if (isOperatorElement(c.name)) {
                if (!initialized) {
                    // An operator as the first child applies to a zero seed.
                    acc = EvalValue::makeNumber(0.0f);
                    initialized = true;
                }
                acc = applyOperator(acc, c, ctx, depth + 1);
            } else {
                acc = evaluateNode(c, ctx, depth + 1);
                initialized = true;
            }
        }
        return initialized ? acc : EvalValue::makeString("");
    }

    static EvalValue applyOperator(const EvalValue& acc, const XmlNode& op,
                                   const MenuEvalContext& ctx, int depth) {
        const std::string name = op.name;
        if (name == "floor") return EvalValue::makeNumber(std::floor(acc.toNumber()));
        if (name == "ceil") return EvalValue::makeNumber(std::ceil(acc.toNumber()));

        EvalValue operand;
        const std::string src = attrOf(op, "src", "");
        if (!src.empty()) {
            operand = traitValue(src, attrOf(op, "trait", ""), ctx, depth);
        } else {
            operand = evaluateNode(op, ctx, depth);
        }

        const float a = acc.toNumber();
        const float b = operand.toNumber();
        if (name == "add") return EvalValue::makeNumber(a + b);
        if (name == "sub") return EvalValue::makeNumber(a - b);
        if (name == "mul" || name == "mult") return EvalValue::makeNumber(a * b);
        if (name == "div") return EvalValue::makeNumber(b == 0.0f ? 0.0f : a / b);
        if (name == "mod") return EvalValue::makeNumber(b == 0.0f ? 0.0f : std::fmod(a, b));
        if (name == "max") return EvalValue::makeNumber(std::max(a, b));
        if (name == "min") return EvalValue::makeNumber(std::min(a, b));
        if (name == "eq") return EvalValue::makeBool(a == b);
        if (name == "neq") return EvalValue::makeBool(a != b);
        if (name == "gt") return EvalValue::makeBool(a > b);
        if (name == "gte") return EvalValue::makeBool(a >= b);
        if (name == "lt") return EvalValue::makeBool(a < b);
        if (name == "lte") return EvalValue::makeBool(a <= b);
        if (name == "and") return EvalValue::makeBool(acc.toBool() && operand.toBool());
        if (name == "or") return EvalValue::makeBool(acc.toBool() || operand.toBool());
        return operand;
    }

    static EvalValue traitValue(const std::string& source, const std::string& trait,
                                const MenuEvalContext& ctx, int depth) {
        if (depth > kMaxEvalDepth) return EvalValue{};
        const std::string src = trim(source);
        if (src.empty()) return EvalValue{};

        if (src == "screen()") {
            if (trait == "width") return EvalValue::makeNumber(ctx.screen_width);
            if (trait == "height") return EvalValue::makeNumber(ctx.screen_height);
            return EvalValue::makeNumber(0.0f);
        }
        if (src == "me()") {
            if (ctx.self_is_menu) return menuTrait(trait, ctx, depth + 1);
            if (!ctx.widget_def) return EvalValue{};
            return widgetTrait(*ctx.widget_def, trait, ctx, depth + 1);
        }
        if (src == "parent()") {
            if (!ctx.parent_widget) return EvalValue{};
            return widgetTrait(*ctx.parent_widget, trait, ctx, depth + 1);
        }
        if (src == "strings()") {
            if (ctx.strings) {
                const auto it = ctx.strings->find(trait);
                if (it != ctx.strings->end()) return EvalValue::makeString(it->second);
            }
            return EvalValue::makeString(trait);
        }

        const SourceCall call = parseSourceCall(src);
        if (call.is_call) {
            if (call.fn == "child") {
                if (!ctx.widget_def) return EvalValue{};
                if (const WidgetDef* c = findDirectChild(*ctx.widget_def, call.arg)) {
                    return widgetTrait(*c, trait, ctx, depth + 1);
                }
                return EvalValue{};
            }
            if (call.fn == "sibling") {
                if (ctx.parent_widget) {
                    if (const WidgetDef* s = findDirectChild(*ctx.parent_widget, call.arg)) {
                        return widgetTrait(*s, trait, ctx, depth + 1);
                    }
                } else if (ctx.menu_def) {
                    // Top-level tiles have the menu as their parent.
                    if (const WidgetDef* s = findMenuChild(ctx.menu_def->widgets, call.arg)) {
                        return widgetTrait(*s, trait, ctx, depth + 1);
                    }
                }
                return EvalValue{};
            }
            if (call.fn == "last") {
                // "last()" = the tile declared just before the current one.
                if (!ctx.widget_def) return EvalValue{};
                const WidgetDef* prev = findPreviousSibling(*ctx.widget_def, ctx);
                if (!prev) return EvalValue{};
                return widgetTrait(*prev, trait, ctx, depth + 1);
            }
            return EvalValue{};
        }

        if (ctx.menu_def && src == ctx.menu_def->name) {
            return menuTrait(trait, ctx, depth + 1);
        }
        if (const WidgetDef* w = findWidget(ctx.menu_def, src)) {
            return widgetTrait(*w, trait, ctx, depth + 1);
        }
        return EvalValue{};
    }

    // The tile that owns w, or null when w is a top-level menu tile.
    static const WidgetDef* parentOf(const WidgetDef& w, const MenuEvalContext& ctx) {
        if (ctx.parents) {
            const auto it = ctx.parents->find(&w);
            if (it != ctx.parents->end()) return it->second;
        }
        if (ctx.widget_def == &w) return ctx.parent_widget;
        return nullptr;
    }

    static EvalValue widgetTrait(const WidgetDef& w, const std::string& trait,
                                 const MenuEvalContext& ctx, int depth) {
        if (depth > kMaxEvalDepth) return EvalValue{};
        const auto it = w.traits.find(trait);
        if (it != w.traits.end() && it->second.has_expr) {
            MenuEvalContext sub = ctx;
            sub.widget_def = &w;
            sub.parent_widget = parentOf(w, ctx);
            sub.self_is_menu = false;
            return evaluateNode(it->second.expr, sub, depth + 1);
        }
        // Native texture size, when the caller supplied texture metadata.
        if (trait == "filewidth" || trait == "fileheight") {
            if (!ctx.texture_size) return EvalValue::makeNumber(0.0f);
            const auto fit = w.traits.find("filename");
            if (fit == w.traits.end() || !fit->second.has_expr) return EvalValue::makeNumber(0.0f);
            MenuEvalContext sub = ctx;
            sub.widget_def = &w;
            sub.parent_widget = parentOf(w, ctx);
            sub.self_is_menu = false;
            const std::string filename =
                evaluateNode(fit->second.expr, sub, depth + 1).toString();
            float tex_w = 0.0f, tex_h = 0.0f;
            if (filename.empty() || !ctx.texture_size(filename, tex_w, tex_h)) {
                return EvalValue::makeNumber(0.0f);
            }
            return EvalValue::makeNumber(trait == "filewidth" ? tex_w : tex_h);
        }
        // A text tile has no declared extents, so me().width means the measured
        // string. Menus\class_menu.xml centres class_information_text with
        // parent().width / 2 at justify centre while left justified labels use
        // (parent().width - me().width) / 2, which only lines up if x is the
        // anchor point and the measurement is available here.
        if (w.type == "text" && (trait == "width" || trait == "height") && ctx.text_size) {
            const auto sit = w.traits.find("string");
            if (sit == w.traits.end() || !sit->second.has_expr) {
                return EvalValue::makeNumber(0.0f);
            }
            MenuEvalContext sub = ctx;
            sub.widget_def = &w;
            sub.parent_widget = parentOf(w, ctx);
            sub.self_is_menu = false;
            const std::string text = evaluateNode(sit->second.expr, sub, depth + 1).toString();
            std::string font;
            const auto fit = w.traits.find("font");
            if (fit != w.traits.end() && fit->second.has_expr) {
                font = evaluateNode(fit->second.expr, sub, depth + 1).toString();
            }
            float text_w = 0.0f, text_h = 0.0f;
            if (text.empty() || !ctx.text_size(text, font, text_w, text_h)) {
                return EvalValue::makeNumber(0.0f);
            }
            return EvalValue::makeNumber(trait == "width" ? text_w : text_h);
        }
        if (trait == "alpha") return EvalValue::makeNumber(1.0f);
        if (trait == "visible") return EvalValue::makeBool(true);
        if (trait == "zoom") return EvalValue::makeNumber(1.0f);
        if (trait == "locus") return EvalValue::makeBool(false);
        if (trait == "x" || trait == "y" || trait == "width" || trait == "height" ||
            trait == "depth" || trait == "red" || trait == "green" || trait == "blue") {
            return EvalValue::makeNumber(0.0f);
        }
        return EvalValue{};
    }

    static EvalValue menuTrait(const std::string& trait, const MenuEvalContext& ctx, int depth) {
        if (depth > kMaxEvalDepth || !ctx.menu_def) return EvalValue{};
        const auto it = ctx.menu_def->traits.find(trait);
        if (it != ctx.menu_def->traits.end() && it->second.has_expr) {
            MenuEvalContext sub = ctx;
            sub.widget_def = nullptr;
            sub.parent_widget = nullptr;
            sub.self_is_menu = true;  // me() inside a menu trait is the menu
            return evaluateNode(it->second.expr, sub, depth + 1);
        }
        if (trait == "alpha") return EvalValue::makeNumber(1.0f);
        if (trait == "visible") return EvalValue::makeBool(true);
        if (trait == "x" || trait == "y" || trait == "width" || trait == "height" ||
            trait == "depth") {
            return EvalValue::makeNumber(0.0f);
        }
        return EvalValue{};
    }
};

void buildWidget(const XmlNode& node, WidgetDef& w, std::vector<TemplateDef>& templates,
                 const std::string& menu_name);

// Reads a single child element of a <menu> or tile node. A child carrying a
// `name` attribute is a tile; <include> and <template>/<prefab> are structural;
// every other element is a trait.
void classifyChild(const XmlNode& child, std::map<std::string, TraitDef>& traits,
                   std::vector<WidgetDef>& tiles, std::vector<IncludeDef>& includes,
                   std::vector<TemplateDef>& templates, const std::string& owner,
                   const std::string& menu_name) {
    if (child.name == "include") {
        const std::string src = attrOf(child, "src", "");
        if (!src.empty()) {
            IncludeDef inc;
            inc.src = src;
            inc.owner = owner;
            inc.menu = menu_name;
            inc.index = tiles.size();
            includes.push_back(std::move(inc));
        }
        return;
    }
    if (child.name == "template" || child.name == "prefab") {
        TemplateDef tpl;
        tpl.name = attrOf(child, "name", "");
        tpl.type = child.name;
        tpl.owner = owner;
        for (const auto& body : child.children) {
            if (body.name == "include") {
                const std::string src = attrOf(body, "src", "");
                if (!src.empty()) {
                    IncludeDef inc;
                    inc.src = src;
                    inc.owner = tpl.name;
                    inc.menu = menu_name;
                    inc.index = tpl.widgets.size();
                    tpl.includes.push_back(std::move(inc));
                }
                continue;
            }
            // Name-less body elements are traits of the definition, not tiles.
            if (!body.attributes.count("name")) continue;
            WidgetDef bw;
            buildWidget(body, bw, templates, menu_name);
            tpl.widgets.push_back(std::move(bw));
        }
        templates.push_back(std::move(tpl));
        return;
    }
    if (child.attributes.count("name")) {
        WidgetDef w;
        buildWidget(child, w, templates, menu_name);
        tiles.push_back(std::move(w));
        return;
    }
    TraitDef t;
    t.name = child.name;
    t.expr = child;
    t.has_expr = true;
    traits[t.name] = std::move(t);
}

void buildWidget(const XmlNode& node, WidgetDef& w, std::vector<TemplateDef>& templates,
                 const std::string& menu_name) {
    w.name = attrOf(node, "name", "");
    w.type = node.name;
    for (const auto& child : node.children) {
        classifyChild(child, w.traits, w.children, w.includes, templates, w.name, menu_name);
    }
}

bool traitIsTrue(const std::map<std::string, EvalValue>& traits, const char* name) {
    const auto it = traits.find(name);
    return it != traits.end() && it->second.toBool();
}

void fillResolved(const WidgetDef& w, const MenuEvalContext& ctx, const WidgetDef* parent,
                  float origin_x, float origin_y, bool parent_clips,
                  ResolvedWidget& rw, int depth) {
    rw.name = w.name;
    rw.type = w.type;
    MenuEvalContext sub = ctx;
    sub.widget_def = &w;
    sub.parent_widget = parent;
    sub.self_is_menu = false;

    bool has_width = false;
    bool has_height = false;
    bool has_zoom = false;
    float zoom_raw = 1.0f;
    for (const auto& kv : w.traits) {
        const TraitDef& t = kv.second;
        if (!t.has_expr) continue;
        const EvalValue v = Evaluator::evaluateNode(t.expr, sub, depth + 1);
        if (t.name == "x") rw.x = v.toNumber();
        else if (t.name == "y") rw.y = v.toNumber();
        else if (t.name == "width") { rw.width = v.toNumber(); has_width = true; }
        else if (t.name == "height") { rw.height = v.toNumber(); has_height = true; }
        else if (t.name == "depth") rw.depth = v.toNumber();
        else if (t.name == "alpha") rw.alpha = v.toNumber();
        else if (t.name == "red") rw.red = v.toNumber();
        else if (t.name == "green") rw.green = v.toNumber();
        else if (t.name == "blue") rw.blue = v.toNumber();
        else if (t.name == "zoom") { zoom_raw = v.toNumber(); has_zoom = true; }
        else if (t.name == "visible") rw.visible = v.toBool();
        else if (t.name == "locus") rw.locus = v.toBool();
        else if (t.name == "filename") rw.filename = v.toString();
        else if (t.name == "string") rw.string = v.toString();
        else if (t.name == "font") rw.font = v.toString();
        else if (t.name == "justify") rw.justify = v.toString();
        if (t.name.rfind("user", 0) == 0) rw.user_traits[t.name] = v;
        rw.traits[t.name] = v;  // unknown traits stay visible to the UI layer
    }
    if (has_zoom) {
        rw.zoom = zoom_raw;
        rw.fill_rect = zoom_raw < 0.0f;                       // < 0 fits the rect
        rw.zoom_scale = rw.fill_rect ? 1.0f : zoom_raw / 100.0f;  // > 0 is a percentage
    }

    if (!rw.filename.empty() && ctx.texture_size) {
        float tex_w = 0.0f, tex_h = 0.0f;
        if (ctx.texture_size(rw.filename, tex_w, tex_h) && tex_w > 0.0f && tex_h > 0.0f) {
            rw.has_texture = true;
            rw.texture_width = tex_w;
            rw.texture_height = tex_h;
        }
    }
    // An image without explicit extents takes the native texture size.
    if (rw.has_texture && (rw.type == "image" || rw.type == "nif")) {
        if (!has_width) rw.width = rw.texture_width;
        if (!has_height) rw.height = rw.texture_height;
    }
    // A text tile without explicit extents takes the measured string extent.
    if (rw.type == "text" && (!has_width || !has_height) && ctx.text_size) {
        float text_w = 0.0f, text_h = 0.0f;
        if (!rw.string.empty() && ctx.text_size(rw.string, rw.font, text_w, text_h)) {
            if (!has_width) rw.width = text_w;
            if (!has_height) rw.height = text_h;
            rw.has_text_extent = true;
        }
    }
    if (rw.fill_rect && rw.has_texture && rw.width > 0.0f && rw.height > 0.0f) {
        rw.texture_scale = std::min(rw.width / rw.texture_width,
                                    rw.height / rw.texture_height);
    }

    // <locus> carries no layout meaning in the shipped data: x/y stay top-left
    // offsets from the parent tile's top-left whether or not locus is set.
    // Evidence: Menus\main\inventory_menu.xml places the five locus tabs at
    // x = 28/165/304/443/578, y = 37 inside the 730x128 locus strip, which only
    // tile side by side when the values are left edges; Menus\loading_menu.xml
    // has <locus> with <x>0</x> on a full screen image. Centring is written out
    // by hand instead, e.g. (screen().width - me().width) / 2. The flag is still
    // reported so the UI layer can read it.
    rw.abs_x = origin_x + rw.x;
    rw.abs_y = origin_y + rw.y;
    rw.clip_window = traitIsTrue(rw.traits, "clipwindow");
    rw.clips = traitIsTrue(rw.traits, "clips");
    rw.clipped = parent_clips;
    rw.includes = w.includes;

    for (const auto& c : w.children) {
        ResolvedWidget rc;
        fillResolved(c, sub, &w, rw.abs_x, rw.abs_y, parent_clips || rw.clip_window,
                     rc, depth + 1);
        rw.children.push_back(std::move(rc));
    }
}

}  // namespace

// --- EvalValue ---------------------------------------------------------------

float EvalValue::toNumber() const {
    if (type == Type::Number) return number;
    if (type == Type::Bool) return boolean ? 1.0f : 0.0f;
    if (type == Type::String) {
        const char* b = string.c_str();
        char* end = nullptr;
        const float v = std::strtof(b, &end);
        if (end && end != b && *end == '\0') return v;
        return 0.0f;
    }
    return 0.0f;
}

bool EvalValue::toBool() const {
    if (type == Type::Bool) return boolean;
    return toNumber() != 0.0f;
}

std::string EvalValue::toString() const {
    if (type == Type::String) return string;
    if (type == Type::Bool) return boolean ? "true" : "false";
    if (type == Type::Number) {
        if (number == std::floor(number) && std::abs(number) < 1e15f) {
            return std::to_string(static_cast<long long>(number));
        }
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%g", number);
        return buf;
    }
    return "";
}

EvalValue EvalValue::makeNumber(float v) {
    EvalValue e;
    e.type = Type::Number;
    e.number = v;
    return e;
}

EvalValue EvalValue::makeString(const std::string& v) {
    EvalValue e;
    e.type = Type::String;
    e.string = v;
    return e;
}

EvalValue EvalValue::makeBool(bool v) {
    EvalValue e;
    e.type = Type::Bool;
    e.boolean = v;
    return e;
}

// --- Evaluator entry points --------------------------------------------------

EvalValue MenuExpressionEvaluator::evaluate(const XmlNode& expr, const MenuEvalContext& ctx) {
    return Evaluator::evaluateNode(expr, ctx, 0);
}

EvalValue MenuExpressionEvaluator::traitValue(const std::string& source,
                                              const std::string& trait,
                                              const MenuEvalContext& ctx) {
    return Evaluator::traitValue(source, trait, ctx, 0);
}

// --- Interpreter -------------------------------------------------------------

bool MenuXmlInterpreter::buildMenuDef(const std::string& xml, MenuDef& out,
                                      std::string& error) {
    XmlNode root;
    if (!XmlParser::parse(xml, root, error, /*lenient=*/true)) return false;
    if (root.name != "menu") {
        error = "root element is not <menu>: " + root.name;
        return false;
    }
    out = MenuDef{};
    out.name = attrOf(root, "name", "");
    for (const auto& child : root.children) {
        classifyChild(child, out.traits, out.widgets, out.includes, out.templates,
                      "", out.name);
    }
    return true;
}

std::vector<ResolvedWidget> MenuXmlInterpreter::resolve(const MenuDef& def,
                                                        const MenuEvalContext& ctx) {
    ResolvedMenu menu = resolveMenu(def, ctx);
    return std::move(menu.widgets);
}

ResolvedMenu MenuXmlInterpreter::resolveMenu(const MenuDef& def,
                                             const MenuEvalContext& ctx) {
    ResolvedMenu out;
    out.name = def.name;

    MenuEvalContext base = ctx;
    base.menu_def = &def;
    base.widget_def = nullptr;
    base.parent_widget = nullptr;
    base.self_is_menu = true;  // me() inside a menu trait refers to the menu

    std::map<const WidgetDef*, const WidgetDef*> parents;
    linkParents(def.widgets, nullptr, parents);
    base.parents = &parents;

    for (const auto& kv : def.traits) {
        if (!kv.second.has_expr) continue;
        const EvalValue v = Evaluator::evaluateNode(kv.second.expr, base, 1);
        out.traits[kv.first] = v;
        if (kv.first == "x") out.x = v.toNumber();
        else if (kv.first == "y") out.y = v.toNumber();
        else if (kv.first == "width") out.width = v.toNumber();
        else if (kv.first == "height") out.height = v.toNumber();
        else if (kv.first == "depth") out.depth = v.toNumber();
        else if (kv.first == "alpha") out.alpha = v.toNumber();
        else if (kv.first == "visible") out.visible = v.toBool();
    }
    for (const auto& w : def.widgets) {
        ResolvedWidget rw;
        fillResolved(w, base, nullptr, out.x, out.y, false, rw, 0);
        out.widgets.push_back(std::move(rw));
    }
    out.includes = def.includes;
    return out;
}

std::size_t MenuXmlInterpreter::countIncludes(const MenuDef& def) {
    std::size_t total = def.includes.size();
    for (const auto& tpl : def.templates) total += tpl.includes.size();
    struct Counter {
        static std::size_t walk(const WidgetDef& w) {
            std::size_t n = w.includes.size();
            for (const auto& c : w.children) n += walk(c);
            return n;
        }
    };
    for (const auto& w : def.widgets) total += Counter::walk(w);
    return total;
}

const TemplateDef* MenuXmlInterpreter::findTemplate(const MenuDef& def,
                                                    const std::string& name) {
    for (const auto& t : def.templates) {
        if (t.name == name) return &t;
    }
    return nullptr;
}

// --- <include> expansion -----------------------------------------------------

namespace {

// Owns one expansion pass. `active` holds the sources currently being expanded,
// which both bounds recursion and breaks include cycles.
struct IncludeExpander {
    const MenuXmlInterpreter::XmlSourceLoader* loader = nullptr;
    std::vector<TemplateDef>* templates = nullptr;
    int max_depth = MenuXmlInterpreter::kMaxIncludeDepth;
    std::set<std::string> active;
    std::string error;

    bool fail(const std::string& message) {
        if (error.empty()) error = message;
        return false;
    }

    // Where the content of an <include> has to be merged: the traits and the
    // child tiles of the widget holding the <include> (the menu itself for a
    // menu level include).
    struct MergeTarget {
        std::map<std::string, TraitDef>* traits = nullptr;
        std::vector<WidgetDef>* tiles = nullptr;
        std::string owner;  // tile holding the <include> ("" = menu level)
        std::string menu;   // owning menu name
    };

    // Read one included document and merge its content into `target`.
    //
    // Every <include> in the shipped menus points at a fragment in
    // Menus\prefabs (button_short.xml, generic_background.xml, ...) that holds
    // the traits - and sometimes the child tiles - the including widget should
    // have. `<image name="book_take"><include src="button_short.xml"/><id>32</id>`
    // therefore means "give book_take the traits of button_short"; the traits the
    // including XML spells out itself win over the ones the fragment provides.
    bool mergeSource(const std::string& src, const MergeTarget& target, int depth) {
        if (depth > max_depth) return fail("include depth exceeded: " + src);
        if (active.count(src)) return fail("include cycle: " + src);
        if (!loader || !*loader) return fail("no include loader supplied");
        std::string xml;
        if (!(*loader)(src, xml)) return fail("include not found: " + src);
        std::vector<XmlNode> nodes;
        std::string parse_error;
        if (!XmlParser::parseFragment(xml, nodes, parse_error, /*lenient=*/true)) {
            return fail("include parse error in " + src + ": " + parse_error);
        }
        active.insert(src);
        bool ok = true;
        if (nodes.size() == 1 && nodes[0].name == "menu") {
            // A <menu> wrapper contributes its content only: the wrapper's own
            // traits belong to that standalone menu, not to the including menu.
            for (const auto& child : nodes[0].children) {
                if (!mergeNode(child, target, depth + 1)) { ok = false; break; }
            }
        } else {
            for (const auto& node : nodes) {
                if (!mergeNode(node, target, depth + 1)) { ok = false; break; }
            }
        }
        active.erase(src);
        return ok;
    }

    // Merge one element of an included document into `target`, recursing for
    // nested includes so that the document order of the fragment is kept.
    bool mergeNode(const XmlNode& node, const MergeTarget& target, int depth) {
        if (node.name == "include") {
            const std::string src = attrOf(node, "src", "");
            if (src.empty()) return true;
            return mergeSource(src, target, depth + 1);
        }
        if (node.name == "template" || node.name == "prefab") {
            if (templates) {
                std::map<std::string, TraitDef> unused_traits;
                std::vector<WidgetDef> unused_tiles;
                std::vector<IncludeDef> unused_includes;
                classifyChild(node, unused_traits, unused_tiles, unused_includes,
                              *templates, target.owner, target.menu);
            }
            return true;
        }
        if (node.attributes.count("name")) {
            if (target.tiles) {
                WidgetDef w;
                buildWidget(node, w, *templates, target.menu);
                target.tiles->push_back(std::move(w));
            }
            return true;
        }
        if (target.traits && !target.traits->count(node.name)) {
            TraitDef t;
            t.name = node.name;
            t.expr = node;
            t.has_expr = true;
            (*target.traits)[t.name] = std::move(t);
        }
        return true;
    }

    // Expand the includes of one owner (a tile, or the menu when owner is
    // empty). The sources are merged in document order; the tiles already merged
    // shift the recorded index of the next include.
    bool expandOwner(std::vector<IncludeDef>& includes, std::vector<WidgetDef>& tiles,
                     std::map<std::string, TraitDef>* traits, const std::string& owner,
                     const std::string& menu, int depth) {
        std::vector<IncludeDef> pending;
        pending.swap(includes);
        std::size_t merged = 0;
        for (const auto& inc : pending) {
            MergeTarget target;
            target.traits = traits;
            target.owner = owner;
            target.menu = menu;
            std::vector<WidgetDef> loaded;
            target.tiles = &loaded;
            if (!mergeSource(inc.src, target, depth)) return false;
            const std::size_t want = inc.index + merged;
            const std::size_t at = want < tiles.size() ? want : tiles.size();
            merged += loaded.size();
            tiles.insert(tiles.begin() + static_cast<std::ptrdiff_t>(at),
                         std::make_move_iterator(loaded.begin()),
                         std::make_move_iterator(loaded.end()));
        }
        for (auto& tile : tiles) {
            if (!expandTile(tile, menu, depth)) return false;
        }
        return true;
    }

    bool expandTile(WidgetDef& w, const std::string& menu, int depth) {
        return expandOwner(w.includes, w.children, &w.traits, w.name, menu, depth);
    }
};

}  // namespace

bool MenuXmlInterpreter::expandIncludes(MenuDef& def, const XmlSourceLoader& loader,
                                        std::string& error, int max_depth) {
    error.clear();
    if (!loader) {
        error = "no include loader supplied";
        return false;
    }
    IncludeExpander expander;
    expander.loader = &loader;
    expander.templates = &def.templates;
    expander.max_depth = max_depth;
    if (!expander.expandOwner(def.includes, def.widgets, &def.traits, "", def.name, 0)) {
        error = expander.error;
        return false;
    }
    return true;
}

}  // namespace oblivion::ui
