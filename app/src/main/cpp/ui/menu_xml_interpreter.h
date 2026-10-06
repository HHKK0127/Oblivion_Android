#pragma once

// Oblivion menu XML interpreter (see docs/MENU_XML_INTERPRETER_DESIGN.md).
//
// Two stages:
//   1. buildMenuDef()  - parse a menu XML file into a MenuDef that keeps every
//                        trait as its raw expression node (lazy).
//   2. resolve()       - evaluate all trait expressions against a runtime
//                        context (screen size, strings table, ...) producing a
//                        tree of ResolvedWidget that the UI layer can consume.
//
// Expression evaluation uses the accumulator model used by the original game:
// the first child of a trait node seeds the accumulator (literal, <copy> or a
// nested expression), each following operator child mutates it (add/sub/mul/...),
// and <onlyif>/<onlyifnot>/<onlynotif> gate whether the next operator applies.
//
// M3 notes (verified against the 89 menus shipped in Data\Menus):
//   - Every child element without a name="" attribute is a trait, including the
//     ones a trait whitelist would drop (clips, clipwindow, repeatvertical,
//     animation, tile, ID, disablefade, returnvalue, ...). All are kept now.
//   - <template>/<prefab> are list-item definitions instantiated by engine code
//     ("list items are added here in code from the template"), not drawn tiles.
//   - Source forms found in the corpus: screen(), me(), parent(), strings(),
//     child(NAME), sibling(NAME), last(), plus plain widget/menu names.

#include "xml_parser.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace oblivion::ui {

// A single named trait definition. `expr` is the raw expression node.
struct TraitDef {
    std::string name;
    XmlNode expr;
    bool has_expr = false;
};

// An unresolved <include src="..."/> together with its insertion point.
struct IncludeDef {
    std::string src;         // path exactly as written in the XML
    std::string owner;       // tile holding the include ("" = menu level)
    std::string menu;        // owning menu name
    std::size_t index = 0;   // position among the owner's tiles (includes excluded)
};

// A widget definition (rect/image/text/nif/...).
struct WidgetDef {
    std::string name;
    std::string type;
    std::map<std::string, TraitDef> traits;
    std::vector<WidgetDef> children;
    std::vector<IncludeDef> includes;  // <include> children of this tile
};

// A <template>/<prefab> list-item definition.
struct TemplateDef {
    std::string name;   // name="" attribute of the definition
    std::string type;   // "template" or "prefab"
    std::string owner;  // tile the definition was declared in ("" = menu level)
    std::vector<WidgetDef> widgets;  // body, instantiated by engine code
    std::vector<IncludeDef> includes;  // <include> written directly in the definition
};

// A parsed menu definition.
struct MenuDef {
    std::string name;
    std::map<std::string, TraitDef> traits;
    std::vector<WidgetDef> widgets;
    std::vector<IncludeDef> includes;     // unresolved <include src="..."/>
    std::vector<TemplateDef> templates;   // list-item definitions (not instantiated)
};

// A typed expression value.
struct EvalValue {
    enum class Type { Null, Number, String, Bool };
    Type type = Type::Null;
    float number = 0.0f;
    std::string string;
    bool boolean = false;

    float toNumber() const;
    bool toBool() const;
    std::string toString() const;

    static EvalValue makeNumber(float v);
    static EvalValue makeString(const std::string& v);
    static EvalValue makeBool(bool v);
};

// Runtime state supplied at resolve() time.
struct MenuEvalContext {
    float screen_width = 0.0f;
    float screen_height = 0.0f;
    const MenuDef* menu_def = nullptr;
    const WidgetDef* widget_def = nullptr;    // current widget (me())
    const WidgetDef* parent_widget = nullptr; // parent widget (parent())
    // widget -> parent map. parent() inside a trait that belongs to a widget
    // referenced from elsewhere then still resolves to that widget's own parent.
    const std::map<const WidgetDef*, const WidgetDef*>* parents = nullptr;
    // True while evaluating a menu-level trait: me() then refers to the menu.
    bool self_is_menu = false;
    const std::map<std::string, std::string>* strings = nullptr;  // strings.xml table
    // Optional texture metadata (native pixel size of a <filename> texture).
    // Enables the filewidth/fileheight traits and image auto sizing.
    std::function<bool(const std::string& filename, float& width, float& height)> texture_size;
    // Optional font metrics (rendered extent of a <string> at a <font>). Text
    // tiles never declare their own <width>, yet the shipped menus centre labels
    // with (width - me().width) / 2, so the measurement has to reach the
    // expression evaluator. Wrap width is not modelled.
    std::function<bool(const std::string& text, const std::string& font, float& width,
                       float& height)>
        text_size;
};

// Expression evaluator (public for testing).
class MenuExpressionEvaluator {
public:
    // Evaluate an expression node to a value.
    static EvalValue evaluate(const XmlNode& expr, const MenuEvalContext& ctx);

    // Resolve a reference such as "screen()"/"me()"/"parent()"/"strings()"/a
    // named widget or menu.
    static EvalValue traitValue(const std::string& source, const std::string& trait,
                                const MenuEvalContext& ctx);
};

// A widget resolved to concrete values.
struct ResolvedWidget {
    std::string name;
    std::string type;
    float x = 0.0f, y = 0.0f;          // top-left, relative to the parent tile
    float width = 0.0f, height = 0.0f;
    float abs_x = 0.0f, abs_y = 0.0f;  // menu origin + parent chain
    float depth = 0.0f;
    float alpha = 1.0f;
    float red = 0.0f, green = 0.0f, blue = 0.0f;
    // <zoom>: negative means "fit the tile rect" (auto), 1..n is a percentage.
    float zoom = 1.0f;         // raw trait value
    float zoom_scale = 1.0f;   // zoom/100 for positive zoom, 1.0 otherwise
    bool fill_rect = false;    // zoom < 0: scale the texture to fit the tile rect
    float texture_width = 0.0f, texture_height = 0.0f;  // native pixel size (0 = unknown)
    float texture_scale = 0.0f;  // uniform zoom<0 fit scale (0 = unknown)
    bool has_texture = false;
    bool has_text_extent = false;  // width/height filled from the measured string
    bool visible = true;
    bool locus = false;        // <locus> flag; layout stays top-left based
    bool clip_window = false;  // <clipwindow>: children are clipped to this tile
    bool clips = false;        // <clips>: this tile is clipped by its parent
    bool clipped = false;      // some ancestor has <clipwindow>
    std::string filename;
    std::string string;
    std::string font;
    // Text only: x/y is the anchor the string grows from, and justify says in
    // which direction (left/centre/right). The shipped menus prove it, e.g.
    // loading_menu.xml centres a label on parent().width / 2 with justify
    // centre, while left justified labels spell out (width - me().width) / 2.
    std::string justify;
    std::map<std::string, EvalValue> user_traits;  // user0..user25 etc.
    std::map<std::string, EvalValue> traits;       // every evaluated trait, by name
    std::vector<IncludeDef> includes;              // unresolved <include> under this tile
    std::vector<ResolvedWidget> children;
};

// A whole menu resolved to concrete values.
struct ResolvedMenu {
    std::string name;
    float x = 0.0f, y = 0.0f;          // menu origin (the menu's own x/y traits)
    float width = 0.0f, height = 0.0f;
    float depth = 0.0f;
    float alpha = 1.0f;
    bool visible = true;
    std::map<std::string, EvalValue> traits;
    std::vector<IncludeDef> includes;
    std::vector<ResolvedWidget> widgets;
};

class MenuXmlInterpreter {
public:
    // Maximum nesting depth honoured by expandIncludes().
    static constexpr int kMaxIncludeDepth = 8;

    // Parse a menu XML file into a MenuDef (traits kept as raw expressions).
    static bool buildMenuDef(const std::string& xml, MenuDef& out, std::string& error);

    // Evaluate a MenuDef against a runtime context.
    static std::vector<ResolvedWidget> resolve(const MenuDef& def, const MenuEvalContext& ctx);

    // Same, but also resolves the menu's own traits (origin/size/alpha).
    static ResolvedMenu resolveMenu(const MenuDef& def, const MenuEvalContext& ctx);

    // --- <include src="..."/> expansion ---------------------------------------
    // Loads an included XML document. Returns false when the source is unknown.
    using XmlSourceLoader = std::function<bool(const std::string& src, std::string& xml_out)>;

    // Splice every <include> in the menu (menu level and tiles) with the XML
    // returned by `loader`, recursively. A source is expanded at most once per
    // branch, which also stops include cycles.
    static bool expandIncludes(MenuDef& def, const XmlSourceLoader& loader,
                               std::string& error, int max_depth = kMaxIncludeDepth);

    // Number of unresolved <include> elements anywhere in the menu.
    static std::size_t countIncludes(const MenuDef& def);

    // List-item templates are instantiated by engine code, so they are exposed
    // instead of being expanded into the widget tree.
    static const TemplateDef* findTemplate(const MenuDef& def, const std::string& name);
};

}  // namespace oblivion::ui
