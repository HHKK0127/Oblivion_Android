// Menu XML interpreter tests - see menu_xml_tests.h for the rationale.

#include "menu_xml_tests.h"

#include "../ui/menu_ui_builder.h"
#include "../ui/menu_xml_interpreter.h"
#include "../ui/xml_parser.h"

#include <algorithm>
#include <chrono>
#include <cmath>

using oblivion::ui::EvalValue;
using oblivion::ui::MenuDef;
using oblivion::ui::MenuEvalContext;
using oblivion::ui::MenuExpressionEvaluator;
using oblivion::ui::MenuXmlInterpreter;
using oblivion::ui::ResolvedMenu;
using oblivion::ui::ResolvedWidget;
using oblivion::ui::WidgetDef;
using oblivion::ui::XmlNode;
using oblivion::ui::XmlParser;

namespace {

constexpr float kEps = 1e-4f;

bool near(float a, float b) {
    return std::fabs(a - b) < kEps;
}

}  // namespace

void MenuXmlTests::record(const std::string& name, bool passed,
                          const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int MenuXmlTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int MenuXmlTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string MenuXmlTests::getSummary() const {
    return "Menu XML Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool MenuXmlTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    // --- 1. XML parser: structure, traits, entities ---------------------------
    {
        std::string err;
        XmlNode root;
        const bool ok = XmlParser::parse(
            "<menu name=\"Test\">"
            "  <!-- a comment -->"
            "  <rect name=\"r\"><x>10</x><y>20</y></rect>"
            "  <text name=\"t\"><string>&next;</string></text>"
            "</menu>",
            root, err);
        record("xml parser accepts a simple menu", ok && err.empty());
        record("xml root name is menu",
               ok && root.name == "menu");
        record("xml root name attribute",
               ok && root.attributes.at("name") == "Test");
        record("xml widget children are found",
               ok && root.children.size() == 2);
        if (ok && root.children.size() >= 2) {
            const XmlNode& r = root.children[0];
            record("xml widget name attribute",
                   r.name == "rect" && r.attributes.at("name") == "r");
            record("xml trait children collected",
                   r.children.size() == 2 && r.children[0].name == "x");
            const XmlNode& t = root.children[1];
            record("xml unknown entity preserved verbatim",
                   t.children.size() == 1 &&
                   t.children[0].name == "string" &&
                   t.children[0].text == "&next;");
        }
        record("xml malformed input is rejected",
               !XmlParser::parse("<menu><a></b></menu>", root, err) &&
               !err.empty());
        record("xml unclosed tag is rejected",
               !XmlParser::parse("<menu><a>", root, err));
    }

    // --- 2. buildMenuDef: trait/widget/include collection ---------------------
    {
        const std::string xml =
            "<menu name=\"Books\">"
            "  <include src=\"tabs.xml\"/>"
            "  <height>600</height>"
            "  <_pagenum><add src=\"Books\" trait=\"user6\"/></_pagenum>"
            "  <rect name=\"book_next\"><x>700</x></rect>"
            "  <image name=\"page1\"><filename>book/page1.dds</filename>"
            "    <rect name=\"inner\"><y>5</y></rect>"
            "  </image>"
            "</menu>";
        std::string err;
        MenuDef def;
        const bool ok = MenuXmlInterpreter::buildMenuDef(xml, def, err);
        record("buildMenuDef parses a menu", ok && err.empty());
        record("menu name is captured", ok && def.name == "Books");
        record("menu includes are collected",
               ok && def.includes.size() == 1 && def.includes[0].src == "tabs.xml" &&
               def.includes[0].index == 0 && def.includes[0].owner.empty() &&
               def.includes[0].menu == "Books");
        record("menu trait is collected",
               ok && def.traits.count("height") == 1);
        record("underscore trait is collected",
               ok && def.traits.count("_pagenum") == 1);
        record("top-level widgets are collected",
               ok && def.widgets.size() == 2);
        if (ok && def.widgets.size() == 2) {
            record("nested widget is collected",
                   def.widgets[1].children.size() == 1 &&
                   def.widgets[1].children[0].name == "inner");
        }
        record("buildMenuDef rejects a non-menu root",
               !MenuXmlInterpreter::buildMenuDef("<rect name=\"x\"/>", def, err));
    }

    // --- 3. Expression evaluation: literals and operators ---------------------
    {
        MenuEvalContext ctx;
        ctx.screen_width = 1280.0f;
        ctx.screen_height = 720.0f;

        // Helper: parse a single expression node body.
        auto eval = [&](const std::string& body) {
            const std::string xml = "<menu name=\"m\"><_t>" + body + "</_t></menu>";
            XmlNode root;
            std::string err;
            XmlParser::parse(xml, root, err);
            const XmlNode& t = root.children[0];
            return MenuExpressionEvaluator::evaluate(t, ctx);
        };

        record("numeric literal", near(eval("42").toNumber(), 42.0f));
        record("string literal", eval("hello").toString() == "hello");
        record("add accumulates", near(eval("<add> 1 </add><add> 2 </add>").toNumber(), 3.0f));
        record("sub with seed", near(eval("10<sub> 4 </sub>").toNumber(), 6.0f));
        record("mul with seed", near(eval("6<mul> 7 </mul>").toNumber(), 42.0f));
        record("mult alias for mul", near(eval("6<mult> 7 </mult>").toNumber(), 42.0f));
        record("div with seed", near(eval("9<div> 3 </div>").toNumber(), 3.0f));
        record("floor", near(eval("7.9<floor> 0 </floor>").toNumber(), 7.0f));
        record("ceil", near(eval("7.1<ceil> 0 </ceil>").toNumber(), 8.0f));
        record("min clamps", near(eval("50<min> 10 </min>").toNumber(), 10.0f));
        record("max clamps", near(eval("5<max> 10 </max>").toNumber(), 10.0f));

        record("screen() width", near(eval("<copy src=\"screen()\" trait=\"width\"/>").toNumber(), 1280.0f));
        record("screen() height", near(eval("<copy src=\"screen()\" trait=\"height\"/>").toNumber(), 720.0f));
        record("operator with src operand",
               near(eval("10<add src=\"screen()\" trait=\"width\"/>").toNumber(), 1290.0f));

        record("entity true resolves to bool",
               eval("<copy> &true; </copy>").toBool() == true);
        record("entity false resolves to bool",
               eval("<copy> &false; </copy>").toBool() == false);
    }

    // --- 4. strings() resolution ----------------------------------------------
    {
        std::map<std::string, std::string> strings;
        strings["_exit"] = "Exit";
        strings["_page"] = "Page";
        MenuEvalContext ctx;
        ctx.strings = &strings;

        const std::string xml = "<menu name=\"m\"><_t><copy src=\"strings()\" trait=\"_exit\"/></_t></menu>";
        XmlNode root;
        std::string err;
        XmlParser::parse(xml, root, err);
        const XmlNode& t = root.children[0];
        const EvalValue v = MenuExpressionEvaluator::evaluate(t, ctx);
        record("strings() resolves a constant", v.toString() == "Exit");
    }

    // --- 5. me()/parent()/named-widget references -----------------------------
    {
        const std::string xml =
            "<menu name=\"m\">"
            "  <rect name=\"panel\">"
            "    <width>200</width>"
            "    <rect name=\"child\">"
            "      <width>150</width>"
            "      <_mywidth><copy src=\"me()\" trait=\"width\"/></_mywidth>"
            "      <_parentwidth><copy src=\"parent()\" trait=\"width\"/></_parentwidth>"
            "    </rect>"
            "  </rect>"
            "  <_panelw><copy src=\"panel\" trait=\"width\"/></_panelw>"
            "</menu>";
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        MenuEvalContext ctx;
        ctx.menu_def = &def;
        ctx.screen_width = 1280.0f;

        // Locate the panel / child WidgetDef nodes to set me()/parent().
        const WidgetDef* panel = nullptr;
        const WidgetDef* child = nullptr;
        for (const auto& w : def.widgets) {
            if (w.name == "panel") {
                panel = &w;
                if (!w.children.empty()) child = &w.children[0];
            }
        }

        MenuEvalContext meCtx = ctx;
        meCtx.widget_def = child;
        meCtx.parent_widget = panel;
        record("me() resolves the current widget",
               panel && child &&
               near(MenuExpressionEvaluator::traitValue(
                        "me()", "width", meCtx).toNumber(), 150.0f));
        record("parent() resolves the parent widget",
               panel && child &&
               near(MenuExpressionEvaluator::traitValue(
                        "parent()", "width", meCtx).toNumber(), 200.0f));

        const auto resolved = MenuXmlInterpreter::resolve(def, ctx);
        record("resolve produces one top-level widget",
               resolved.size() == 1 && resolved[0].name == "panel");
        if (resolved.size() == 1) {
            record("nested widget is resolved",
                   resolved[0].children.size() == 1 &&
                   resolved[0].children[0].name == "child");
        }
        record("named widget reference resolves",
               near(MenuExpressionEvaluator::traitValue("panel", "width", ctx).toNumber(), 200.0f));
        record("unknown reference yields null",
               MenuExpressionEvaluator::traitValue("nope", "width", ctx).type == EvalValue::Type::Null);
    }

    // --- 6. Real book_menu.xml _pagenum accumulator ---------------------------
    // Structure modelled on the real Oblivion book_menu.xml:
    //   _pagenum = BookMenu.user6 + (book_prev clicked ? -2) + (book_next clicked ? +2)
    //   then min(pagecount-even-cap, ...) and max(0, ...).
    // The mul/mult/floor operators and the &entity; literals are real corpus forms.
    {
        auto makeMenu = [](int user6, int prev, int next, int pagecount) {
            return
                "<menu name=\"BookMenu\">"
                "<user6>" + std::to_string(user6) + "</user6>"
                "<_pagenum>"
                "  <add src=\"BookMenu\" trait=\"user6\"/>"
                "  <sub><copy src=\"book_prev\" trait=\"clicked\"/><mul> 2 </mul></sub>"
                "  <add><copy src=\"book_next\" trait=\"clicked\"/><mul> 2 </mul></add>"
                "  <min><copy src=\"book_page_1_text\" trait=\"pagecount\"/>"
                "       <div> 2 </div><floor> 0 </floor><mul> 2 </mul></min>"
                "  <max> 0 </max>"
                "</_pagenum>"
                "<rect name=\"book_prev\"><clicked>" + std::to_string(prev) + "</clicked></rect>"
                "<rect name=\"book_next\"><clicked>" + std::to_string(next) + "</clicked></rect>"
                "<text name=\"book_page_1_text\"><pagecount>" + std::to_string(pagecount) + "</pagecount></text>"
                "</menu>";
        };
        auto evalPagenum = [&](int user6, int prev, int next, int pagecount) {
            std::string err;
            MenuDef def;
            MenuXmlInterpreter::buildMenuDef(makeMenu(user6, prev, next, pagecount), def, err);
            MenuEvalContext ctx;
            ctx.menu_def = &def;
            ctx.screen_width = 1280.0f;
            ctx.screen_height = 720.0f;
            const auto it = def.traits.find("_pagenum");
            MenuEvalContext sub = ctx;
            sub.widget_def = nullptr;
            sub.parent_widget = nullptr;
            return MenuExpressionEvaluator::evaluate(it->second.expr, sub).toNumber();
        };

        // Base: user6 = 0, no clicks -> 0.
        record("pagenum base is user6", near(evalPagenum(0, 0, 0, 10), 0.0f));
        // prev clicked -> 4 - 2 = 2 (one spread back).
        record("pagenum prev click is -2", near(evalPagenum(4, 1, 0, 10), 2.0f));
        // next clicked -> 4 + 2 = 6.
        record("pagenum next click is +2", near(evalPagenum(4, 0, 1, 10), 6.0f));
        // user6 seed shifts the base.
        record("pagenum user6 seed shifts base", near(evalPagenum(4, 0, 0, 10), 4.0f));
        // min clamps against the page-count cap (largest even <= pagecount).
        record("pagenum min cap", near(evalPagenum(20, 0, 0, 10), 10.0f));
        // max(0) never lets the page go negative.
        record("pagenum max floor", near(evalPagenum(0, 1, 0, 10), 0.0f));
        // pagecount clamp takes precedence over the max floor once capped.
        record("pagenum cap at zero pagecount", near(evalPagenum(8, 0, 0, 4), 4.0f));
    }

    // --- 7. onlyif gating -----------------------------------------------------
    {
        MenuEvalContext ctx;
        ctx.screen_width = 1280.0f;

        auto eval = [&](const std::string& body) {
            const std::string xml = "<menu name=\"m\"><_t>" + body + "</_t></menu>";
            XmlNode root;
            std::string err;
            XmlParser::parse(xml, root, err);
            return MenuExpressionEvaluator::evaluate(root.children[0], ctx).toNumber();
        };

        // onlyif false -> the next operator is skipped.
        record("onlyif false skips the next operator",
               near(eval("10<onlyif>&false;</onlyif><add> 5 </add>"), 10.0f));
        // onlyif true -> the next operator applies.
        record("onlyif true keeps the next operator",
               near(eval("10<onlyif>&true;</onlyif><add> 5 </add>"), 15.0f));
        // onlyifnot true -> the next operator is skipped.
        record("onlyifnot true skips the next operator",
               near(eval("10<onlyifnot>&true;</onlyifnot><add> 5 </add>"), 10.0f));
    }

    // --- 8. resolve(): concrete widget values ----------------------------------
    {
        const std::string xml =
            "<menu name=\"m\">"
            "  <rect name=\"r\">"
            "    <x>10</x><y>20</y><width>100</width><height>50</height>"
            "    <alpha>0.5</alpha><visible>&false;</visible>"
            "    <user0>7</user0>"
            "  </rect>"
            "</menu>";
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        MenuEvalContext ctx;
        ctx.screen_width = 1280.0f;
        ctx.screen_height = 720.0f;
        const auto resolved = MenuXmlInterpreter::resolve(def, ctx);
        record("resolve yields the widget", resolved.size() == 1);
        if (resolved.size() == 1) {
            const ResolvedWidget& r = resolved[0];
            record("resolved x", near(r.x, 10.0f));
            record("resolved y", near(r.y, 20.0f));
            record("resolved width", near(r.width, 100.0f));
            record("resolved height", near(r.height, 50.0f));
            record("resolved alpha", near(r.alpha, 0.5f));
            record("resolved visible from entity", r.visible == false);
            record("resolved user trait", near(r.user_traits.at("user0").toNumber(), 7.0f));
        }
    }

    // --- 9. not / rand / onlynotif --------------------------------------------
    {
        MenuEvalContext ctx;
        ctx.screen_width = 1280.0f;
        auto eval = [&](const std::string& body) {
            const std::string xml = "<menu name=\"m\"><_t>" + body + "</_t></menu>";
            XmlNode root;
            std::string err;
            XmlParser::parse(xml, root, err);
            return MenuExpressionEvaluator::evaluate(root.children[0], ctx);
        };

        record("not negates true", eval("<not> &true; </not>").toBool() == false);
        record("not negates false", eval("<not> &false; </not>").toBool() == true);
        // Corpus form: <not> &xenon; </not> in options/main_menu.xml.
        record("not of a non-zero operand is false", !eval("<not> 1 </not>").toBool());

        // <rand> N </rand> is a uniform integer in 1..N.
        bool in_range = true;
        bool saw_lo = false;
        bool saw_hi = false;
        for (int i = 0; i < 400; ++i) {
            const float v = eval("<rand> 4 </rand>").toNumber();
            if (v < 1.0f || v > 4.0f) in_range = false;
            if (near(v, 1.0f)) saw_lo = true;
            if (near(v, 4.0f)) saw_hi = true;
        }
        record("rand stays within 1..N", in_range);
        record("rand reaches both ends of 1..N", saw_lo && saw_hi);
        record("rand of 1 is always 1", near(eval("<rand> 1 </rand>").toNumber(), 1.0f));

        // onlynotif is the spelling used by the shipped blue button prefabs.
        record("onlynotif true skips the next operator",
               near(eval("10<onlynotif>&true;</onlynotif><add> 5 </add>").toNumber(), 10.0f));
        record("onlynotif false keeps the next operator",
               near(eval("10<onlynotif>&false;</onlynotif><add> 5 </add>").toNumber(), 15.0f));
    }

    // --- 10. child() / sibling() / last() references ---------------------------
    {
        const std::string xml =
            "<menu name=\"m\">"
            "  <rect name=\"a\"><user1>11</user1></rect>"
            "  <rect name=\"b\">"
            "    <_prev><copy src=\"last()\" trait=\"user1\"/></_prev>"
            "  </rect>"
            "  <rect name=\"host\">"
            "    <_c1><copy src=\"child(c1)\" trait=\"user1\"/></_c1>"
            "    <rect name=\"c1\"><user1>33</user1></rect>"
            "    <rect name=\"c2\">"
            "      <_sib><copy src=\"sibling(c1)\" trait=\"user1\"/></_sib>"
            "    </rect>"
            "  </rect>"
            "</menu>";
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        MenuEvalContext ctx;
        ctx.screen_width = 1280.0f;
        const auto resolved = MenuXmlInterpreter::resolveMenu(def, ctx);
        record("resolveMenu keeps every top-level tile", resolved.widgets.size() == 3);
        if (resolved.widgets.size() == 3) {
            const ResolvedWidget& b = resolved.widgets[1];
            const ResolvedWidget& host = resolved.widgets[2];
            record("last() reads the previous sibling",
                   b.traits.count("_prev") == 1 &&
                   near(b.traits.at("_prev").toNumber(), 11.0f));
            record("child(NAME) reads a direct child",
                   host.traits.count("_c1") == 1 &&
                   near(host.traits.at("_c1").toNumber(), 33.0f));
            if (host.children.size() == 2) {
                record("sibling(NAME) reads a sibling tile",
                       host.children[1].traits.count("_sib") == 1 &&
                       near(host.children[1].traits.at("_sib").toNumber(), 33.0f));
            }
        }
        record("unknown child() yields null",
               MenuExpressionEvaluator::traitValue("child(nope)", "user1",
                                                   MenuEvalContext{}).type ==
                   EvalValue::Type::Null);
    }

    // --- 11. zoom: percentage vs. fit-the-rect ---------------------------------
    {
        const std::string xml =
            "<menu name=\"m\">"
            "  <image name=\"fit\"><filename>tex.dds</filename>"
            "    <width>100</width><height>100</height><zoom>-1</zoom></image>"
            "  <image name=\"pct\"><filename>tex.dds</filename><zoom>150</zoom></image>"
            "  <image name=\"plain\"><filename>tex.dds</filename></image>"
            "</menu>";
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        MenuEvalContext ctx;
        ctx.screen_width = 1280.0f;
        ctx.texture_size = [](const std::string& src, float& w, float& h) {
            if (src != "tex.dds") return false;
            w = 256.0f;
            h = 64.0f;
            return true;
        };
        const auto resolved = MenuXmlInterpreter::resolve(def, ctx);
        record("zoom resolves all three images", resolved.size() == 3);
        if (resolved.size() == 3) {
            const ResolvedWidget& fit = resolved[0];
            const ResolvedWidget& pct = resolved[1];
            const ResolvedWidget& plain = resolved[2];
            record("negative zoom fits the rect",
                   fit.fill_rect && near(fit.zoom_scale, 1.0f));
            record("fit scale is the smaller axis ratio",
                   near(fit.texture_scale, 0.390625f));
            record("positive zoom is a percentage",
                   !pct.fill_rect && near(pct.zoom_scale, 1.5f));
            record("absent zoom does not scale",
                   !plain.fill_rect && near(plain.zoom_scale, 1.0f));
            record("image without extents takes the texture size",
                   near(plain.width, 256.0f) && near(plain.height, 64.0f));
            record("texture metadata is captured",
                   plain.has_texture && near(plain.texture_width, 256.0f) &&
                   near(plain.texture_height, 64.0f));
        }
    }

    // --- 12. locus, absolute position and clipping ------------------------------
    {
        const std::string xml =
            "<menu name=\"m\">"
            "  <rect name=\"panel\"><x>10</x><y>20</y><width>200</width><height>100</height>"
            "    <rect name=\"marked\"><x>100</x><y>50</y>"
            "      <width>40</width><height>20</height><locus>&true;</locus></rect>"
            "    <rect name=\"plain\"><x>5</x><y>5</y></rect>"
            "  </rect>"
            "</menu>";
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        MenuEvalContext ctx;
        ctx.screen_width = 1280.0f;
        const auto resolved = MenuXmlInterpreter::resolve(def, ctx);
        record("resolve yields the panel", resolved.size() == 1);
        if (resolved.size() == 1 && resolved[0].children.size() == 2) {
            const ResolvedWidget& parent = resolved[0];
            const ResolvedWidget& marked = parent.children[0];
            const ResolvedWidget& plain = parent.children[1];
            record("locus is decoded as a trait",
                   marked.locus && !plain.locus);
            record("locus x/y stay top-left offsets",
                   near(marked.x, 100.0f) && near(marked.y, 50.0f));
            record("non-locus coordinates are kept as written",
                   near(plain.x, 5.0f) && near(plain.y, 5.0f));
            record("absolute x accumulates the parent origin",
                   near(marked.abs_x, 110.0f) && near(marked.abs_y, 70.0f));
            record("absolute position of a non-locus child",
                   near(plain.abs_x, 15.0f) && near(plain.abs_y, 25.0f));
        }
    }

    // --- 12b. Corpus evidence: a locus parent still uses its top-left ----------
    // Menus\main\inventory_menu.xml nests the five locus tabs at x = 28/165/304/
    // 443/578, y = 37 inside the 730x128 locus strip at (0, 594). The tabs only
    // sit side by side inside the strip when those values are left edges.
    {
        const std::string xml =
            "<menu name=\"m\">"
            "  <rect name=\"inv_tabs\"><x>0</x><y>594</y><width>730</width><height>128</height>"
            "    <locus>&true;</locus>"
            "    <rect name=\"inv_tabs_p1\"><x>28</x><y>37</y><width>135</width><height>85</height>"
            "      <locus>&true;</locus>"
            "      <image name=\"inv_tabs_p1_icon\"><x>37</x><y>8</y>"
            "        <width>64</width><height>64</height></image>"
            "    </rect>"
            "    <rect name=\"inv_tabs_p5\"><x>578</x><y>37</y><width>135</width><height>85</height>"
            "      <locus>&true;</locus></rect>"
            "  </rect>"
            "</menu>";
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        MenuEvalContext ctx;
        ctx.screen_width = 1280.0f;
        const auto resolved = MenuXmlInterpreter::resolve(def, ctx);
        if (resolved.size() == 1 && resolved[0].children.size() == 2 &&
            resolved[0].children[0].children.size() == 1) {
            const ResolvedWidget& strip = resolved[0];
            const ResolvedWidget& tab1 = strip.children[0];
            const ResolvedWidget& tab5 = strip.children[1];
            const ResolvedWidget& icon = tab1.children[0];
            record("a locus child is placed from the parent top-left",
                   near(tab1.abs_x, 28.0f) && near(tab1.abs_y, 631.0f));
            record("the tabs stay inside the strip",
                   near(tab5.abs_x + 135.0f, 713.0f) && tab5.abs_x < strip.abs_x + 730.0f);
            record("a non-locus grandchild keeps plain offsets",
                   near(icon.abs_x, 65.0f) && near(icon.abs_y, 639.0f));
        } else {
            record("a locus child is placed from the parent top-left", false);
        }
    }

    // --- 13. clips / clipwindow -------------------------------------------------
    {
        const std::string xml =
            "<menu name=\"m\">"
            "  <rect name=\"scroll\"><clipwindow> &true; </clipwindow>"
            "    <rect name=\"row\"><clips> &true; </clips>"
            "      <rect name=\"cell\"/>"
            "    </rect>"
            "  </rect>"
            "  <rect name=\"free\"/>"
            "</menu>";
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        MenuEvalContext ctx;
        ctx.screen_width = 1280.0f;
        const auto resolved = MenuXmlInterpreter::resolve(def, ctx);
        record("resolve yields scroll and free", resolved.size() == 2);
        if (resolved.size() == 2 && resolved[0].children.size() == 1 &&
            resolved[0].children[0].children.size() == 1) {
            const ResolvedWidget& scroll = resolved[0];
            const ResolvedWidget& row = scroll.children[0];
            const ResolvedWidget& cell = row.children[0];
            record("clipwindow is decoded", scroll.clip_window && !scroll.clips);
            record("clips is decoded", row.clips && !row.clip_window);
            record("a clipwindow is not clipped by itself", !scroll.clipped);
            record("descendants of a clipwindow are clipped", row.clipped && cell.clipped);
            record("an unrelated tile is not clipped", !resolved[1].clipped);
        }
    }

    // --- 14. template / prefab retention ----------------------------------------
    {
        const std::string xml =
            "<menu name=\"m\">"
            "  <template name=\"item\"><rect name=\"item_rect\"><width>10</width></rect>"
            "    <user0> 1 </user0>"
            "  </template>"
            "  <prefab name=\"pf\"><rect name=\"pf_rect\"/></prefab>"
            "  <rect name=\"real\"/>"
            "</menu>";
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        record("template and prefab are collected", def.templates.size() == 2);
        record("templates are not part of the widget tree",
               def.widgets.size() == 1 && def.widgets[0].name == "real");
        const auto* tpl = MenuXmlInterpreter::findTemplate(def, "item");
        record("findTemplate finds the definition by name", tpl != nullptr &&
               tpl->type == "template");
        record("template body tiles are captured",
               tpl && tpl->widgets.size() == 1 && tpl->widgets[0].name == "item_rect");
        record("findTemplate misses an unknown definition",
               MenuXmlInterpreter::findTemplate(def, "nope") == nullptr);
        const auto resolved = MenuXmlInterpreter::resolve(def, MenuEvalContext{});
        record("templates do not resolve into widgets",
               resolved.size() == 1 && resolved[0].name == "real");
    }

    // --- 15. <include> expansion -----------------------------------------------
    {
        const std::string menu_xml =
            "<menu name=\"m\">"
            "  <include src=\"tabs.xml\"/>"
            "  <rect name=\"tail\"/>"
            "</menu>";
        const std::string tabs_xml =
            "<menu name=\"tabs\">"
            "  <rect name=\"tab1\"/><rect name=\"tab2\"/>"
            "  <include src=\"inner.xml\"/>"
            "</menu>";
        const std::string inner_xml =
            "<menu name=\"inner\"><rect name=\"inner_tile\"/></menu>";

        auto loader = [&](const std::string& src, std::string& out) {
            if (src == "tabs.xml") { out = tabs_xml; return true; }
            if (src == "inner.xml") { out = inner_xml; return true; }
            return false;
        };

        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(menu_xml, def, err);
        record("include is recorded with its owner and index",
               def.includes.size() == 1 && def.includes[0].src == "tabs.xml" &&
               def.includes[0].owner.empty() && def.includes[0].index == 0);
        record("countIncludes counts nested includes",
               MenuXmlInterpreter::countIncludes(def) == 1);

        const bool ok = MenuXmlInterpreter::expandIncludes(def, loader, err);
        record("expandIncludes succeeds", ok && err.empty());
        record("included tiles are spliced at the recorded index",
               def.widgets.size() == 4 && def.widgets[0].name == "tab1" &&
               def.widgets[1].name == "tab2" &&
               def.widgets[2].name == "inner_tile" &&
               def.widgets[3].name == "tail");
        record("expanded includes are cleared",
               def.includes.empty() && MenuXmlInterpreter::countIncludes(def) == 0);

        // A widget-level include grows that tile's children.
        const std::string tile_xml =
            "<menu name=\"m\"><rect name=\"host\"><include src=\"tabs.xml\"/></rect></menu>";
        MenuDef tile_def;
        MenuXmlInterpreter::buildMenuDef(tile_xml, tile_def, err);
        const bool tile_ok = MenuXmlInterpreter::expandIncludes(tile_def, loader, err);
        record("a tile-level include grows the tile",
               tile_ok && tile_def.widgets.size() == 1 &&
               tile_def.widgets[0].children.size() == 3 &&
               tile_def.widgets[0].children[0].name == "tab1");

        // Unknown source: expansion fails and reports it.
        MenuDef missing;
        MenuXmlInterpreter::buildMenuDef(
            "<menu name=\"m\"><include src=\"nope.xml\"/></menu>", missing, err);
        const bool missing_ok = MenuXmlInterpreter::expandIncludes(missing, loader, err);
        record("an unknown include source fails",
               !missing_ok && err.find("nope.xml") != std::string::npos);

        // Self-including source is rejected instead of recursing forever.
        auto cycle_loader = [](const std::string& src, std::string& out) {
            if (src != "cycle.xml") return false;
            out = "<menu name=\"c\"><include src=\"cycle.xml\"/></menu>";
            return true;
        };
        MenuDef cycle;
        MenuXmlInterpreter::buildMenuDef(
            "<menu name=\"m\"><include src=\"cycle.xml\"/></menu>", cycle, err);
        const bool cycle_ok = MenuXmlInterpreter::expandIncludes(cycle, cycle_loader, err);
        record("an include cycle is rejected",
               !cycle_ok && err.find("cycle") != std::string::npos);
    }

    // --- 16. unknown traits, filewidth/fileheight ------------------------------
    {
        const std::string xml =
            "<menu name=\"m\">"
            "  <image name=\"tex\">"
            "    <filename>tex.dds</filename>"
            "    <_fw><copy src=\"me()\" trait=\"filewidth\"/></_fw>"
            "    <_fh><copy src=\"me()\" trait=\"fileheight\"/></_fh>"
            "    <disablefade>&true;</disablefade>"
            "    <repeatvertical>4</repeatvertical>"
            "  </image>"
            "</menu>";
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        record("unlisted traits are still collected",
               def.widgets.size() == 1 && def.widgets[0].traits.count("disablefade") == 1 &&
               def.widgets[0].traits.count("repeatvertical") == 1);

        MenuEvalContext ctx;
        ctx.texture_size = [](const std::string& src, float& w, float& h) {
            if (src != "tex.dds") return false;
            w = 512.0f;
            h = 128.0f;
            return true;
        };
        const auto resolved = MenuXmlInterpreter::resolve(def, ctx);
        record("every trait is exposed on the resolved widget",
               resolved.size() == 1 && resolved[0].traits.count("disablefade") == 1 &&
               near(resolved[0].traits.at("repeatvertical").toNumber(), 4.0f));
        if (resolved.size() == 1) {
            record("filewidth reports the native texture width",
                   near(resolved[0].traits.at("_fw").toNumber(), 512.0f));
            record("fileheight reports the native texture height",
                   near(resolved[0].traits.at("_fh").toNumber(), 128.0f));
        }

        const auto no_tex = MenuXmlInterpreter::resolve(def, MenuEvalContext{});
        record("filewidth is zero without texture metadata",
               no_tex.size() == 1 &&
               near(no_tex[0].traits.at("_fw").toNumber(), 0.0f) &&
               !no_tex[0].has_texture);
    }

    // --- 17. Real corpus shape: loading_menu.xml zoom + locus + text -----------
    // Menus\loading_menu.xml sizes load_main from the screen and centres it by
    // hand; <locus> adds nothing on top of the written x/y.
    {
        const std::string xml =
            "<menu name=\"LoadingMenu\">"
            "  <image name=\"load_main\">"
            "    <locus>&true;</locus>"
            "    <filename>Menus\\Loading\\load_in_game_default.dds</filename>"
            "    <x><copy src=\"screen()\" trait=\"width\"/>"
            "       <sub src=\"me()\" trait=\"width\"/><div>2</div></x>"
            "    <y><copy src=\"screen()\" trait=\"height\"/>"
            "       <sub src=\"me()\" trait=\"height\"/><div>2</div></y>"
            "    <width><copy src=\"screen()\" trait=\"height\"/>"
            "       <mul>16</mul><div>9</div></width>"
            "    <height><copy src=\"screen()\" trait=\"height\"/></height>"
            "    <zoom>-1</zoom>"
            "    <depth>1000</depth>"
            "    <text name=\"load_text\">"
            "      <string>Loading</string>"
            "      <font>1</font>"
            "      <justify>&left;</justify>"
            "      <x><copy src=\"parent()\" trait=\"width\"/>"
            "         <sub src=\"me()\" trait=\"width\"/><div>2</div></x>"
            "      <y>250</y>"
            "    </text>"
            "  </image>"
            "</menu>";
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        MenuEvalContext ctx;
        ctx.screen_width = 1280.0f;
        ctx.screen_height = 720.0f;
        ctx.texture_size = [](const std::string& src, float& w, float& h) {
            if (src.find("load_in_game_default.dds") == std::string::npos) return false;
            w = 1600.0f;
            h = 900.0f;
            return true;
        };
        // Font metrics stand in for the engine: 8 px per glyph, 20 px tall.
        ctx.text_size = [](const std::string& text, const std::string& font, float& w, float& h) {
            if (font != "1") return false;
            w = 8.0f * static_cast<float>(text.size());
            h = 20.0f;
            return true;
        };
        const auto resolved = MenuXmlInterpreter::resolve(def, ctx);
        record("load_main resolves", resolved.size() == 1);
        if (resolved.size() == 1 && resolved[0].children.size() == 1) {
            const ResolvedWidget& bg = resolved[0];
            const ResolvedWidget& label = bg.children[0];
            record("the background is centred by the written expression",
                   near(bg.x, 0.0f) && near(bg.y, 0.0f) && bg.locus);
            record("16:9 width comes from the screen height",
                   near(bg.width, 1280.0f) && near(bg.height, 720.0f));
            record("negative zoom fits the native texture",
                   bg.fill_rect && bg.has_texture && near(bg.texture_width, 1600.0f) &&
                       near(bg.texture_height, 900.0f));
            record("zoom keeps its raw value", near(bg.zoom, -1.0f) && near(bg.zoom_scale, 1.0f));
            record("a text tile measures itself for me().width",
                   label.has_text_extent && near(label.width, 56.0f) &&
                       near(label.height, 20.0f));
            record("the label is centred on the parent through its anchor",
                   near(label.x, (1280.0f - 56.0f) / 2.0f) && near(label.y, 250.0f) &&
                       label.justify == "left");
            record("the label keeps the parent origin",
                   near(label.abs_x, label.x) && near(label.abs_y, 250.0f));
        } else {
            record("load_main resolves", false);
        }
    }

    // --- 18. Lenient recovery: miswritten closing tags -------------------------
    // Menus\options\video_menu.xml writes "<y> <copy .../> <y>" where the second
    // tag should be "</y>", so the closer that follows names an ancestor. The
    // parser hands the tag back to that ancestor and the content the stray tag
    // swallowed is lifted into it.
    {
        const std::string xml =
            "<menu name=\"VideoMenu\">"
            "  <rect name=\"main\">"
            "    <text name=\"video_resolution_text\">"
            "      <y><copy src=\"parent()\" trait=\"y\"/><y>"
            "      <string>res</string>"
            "      <clicksound>3</clicksound>"
            "    </text>"
            "    <image name=\"video_antialias_right\"/>"
            "  </rect>"
            "</menu>";
        std::string err;
        XmlNode root;
        const bool ok = XmlParser::parse(xml, root, err, /*lenient=*/true);
        record("lenient parse accepts an adopted closing tag", ok && err.empty());
        record("strict parse still rejects it",
               !XmlParser::parse(xml, root, err, /*lenient=*/false) && !err.empty());

        const XmlNode* main_rect = ok ? &root.children[0] : nullptr;
        record("parsing continues past the repaired element",
               main_rect && main_rect->children.size() == 2 &&
               main_rect->children[1].name == "image" &&
               main_rect->children[1].attributes.at("name") == "video_antialias_right");
        if (main_rect && !main_rect->children.empty()) {
            const XmlNode& text = main_rect->children[0];
            record("the stray tag is dropped, its content lifted",
                   text.children.size() == 3 && text.children[0].name == "y" &&
                   text.children[0].children.size() == 1 &&
                   text.children[1].name == "string" && text.children[2].name == "clicksound");
        }

        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(xml, def, err);
        const auto resolved = MenuXmlInterpreter::resolve(def, MenuEvalContext{});
        record("repaired traits reach the resolved widget",
               resolved.size() == 1 && resolved[0].children.size() == 2 &&
               resolved[0].children[0].traits.at("string").toString() == "res" &&
               near(resolved[0].children[0].traits.at("clicksound").toNumber(), 3.0f));
    }

    // A closer that names an ancestor whose deeper elements are all *named* is
    // not an adoption: the shipped quantity_menu.xml closes an <image> with a
    // "</rect>", so the innermost element is the one that ends.
    {
        const std::string xml =
            "<menu name=\"m\"><rect name=\"r\">"
            "<image name=\"i\"><x>5</x></rect>"
            "<image name=\"i2\"><x>6</x></image>"
            "</menu>";
        std::string err;
        XmlNode root;
        const bool ok = XmlParser::parse(xml, root, err, /*lenient=*/true);
        record("a misnamed closer ends the innermost element", ok && err.empty());
        record("strict parse rejects the misnamed closer",
               !XmlParser::parse(xml, root, err, /*lenient=*/false));
        const XmlNode& r = root.children[0];
        record("the misnamed closer does not swallow the sibling",
               ok && r.children.size() == 2 && r.children[0].name == "image" &&
               r.children[0].children.size() == 1 && r.children[0].children[0].name == "x" &&
               r.children[1].attributes.at("name") == "i2");
    }

    // --- 18b. Lenient recovery in <include> fragments --------------------------
    // Fragments go through parseFragment(), which carries the same recovery and
    // keeps every top level element of the fragment.
    {
        std::string err;
        std::vector<XmlNode> top;
        const std::string frag =
            "<prefab name=\"p\"><rect name=\"inner\">"
            "<y><copy src=\"me()\" trait=\"y\"/><y><string>s</string>"
            "</rect></prefab>"
            "<rect name=\"after\"/>";
        const bool ok = XmlParser::parseFragment(frag, top, err, /*lenient=*/true);
        record("a fragment is parsed past an adopted closing tag",
               ok && top.size() == 2 && top[0].name == "prefab" &&
               top[1].attributes.at("name") == "after");
        if (top.size() == 2) {
            const XmlNode& inner = top[0].children[0];
            record("the fragment keeps the adopted element's content",
                   inner.name == "rect" && inner.children.size() == 2 &&
                   inner.children[1].name == "string");
        }
        const std::string stray = "<a><b/></a></c><d/>";
        record("a stray top level tag is dropped in a fragment",
               XmlParser::parseFragment(stray, top, err, /*lenient=*/true) && top.size() == 2 &&
               top[0].children.size() == 1 && top[1].name == "d");
        record("strict fragment parsing rejects the stray tag",
               !XmlParser::parseFragment(stray, top, err, /*lenient=*/false) && !err.empty());
    }

    // --- 18c. Include fragments: parent traits win, tiles are inserted ---------
    {
        std::string err;
        MenuDef def;
        MenuXmlInterpreter::buildMenuDef(
            "<menu name=\"m\"><x>1</x><include src=\"frag.xml\"/>"
            "<rect name=\"t\"><x>3</x><include src=\"nested.xml\"/>"
            "<rect name=\"own\"/></rect>"
            "</menu>",
            def, err);
        auto loader = [](const std::string& src, std::string& out) {
            if (src == "frag.xml") {
                out = "<x>9</x><y>2</y><rect name=\"tile\"/>";
                return true;
            }
            if (src == "nested.xml") {
                out = "<x>7</x><y>4</y><rect name=\"sub\"/>";
                return true;
            }
            return false;
        };
        const bool ok = MenuXmlInterpreter::expandIncludes(def, loader, err);
        record("fragment traits merge into the menu", ok && err.empty());
        record("the menu's own trait wins over the fragment's",
               def.traits.count("x") == 1 && def.traits.at("x").expr.text == "1");
        record("the fragment's other traits are added",
               def.traits.count("y") == 1 && def.traits.at("y").expr.text == "2");
        record("fragment tiles are inserted where the include was",
               def.widgets.size() == 2 && def.widgets[0].name == "tile" &&
               def.widgets[1].name == "t");
        record("the tile's own trait wins over the fragment's",
               def.widgets[1].traits.count("x") == 1 &&
               def.widgets[1].traits.at("x").expr.text == "3");
        record("a tile-level include keeps the tile's children",
               def.widgets[1].children.size() == 2 && def.widgets[1].children[0].name == "sub" &&
               def.widgets[1].children[1].name == "own");
        const auto resolved = MenuXmlInterpreter::resolve(def, MenuEvalContext{});
        record("merged values resolve", resolved.size() == 2 &&
               near(resolved[1].traits.at("x").toNumber(), 3.0f) &&
               near(resolved[1].traits.at("y").toNumber(), 4.0f));
    }

    // --- 18d. Unterminated comment handling ------------------------------------
    // The shipped menus contain no unterminated comment, so lenient mode is free
    // to treat the remainder of the document as comment text, while strict mode
    // reports it as an error.
    {
        std::string err;
        XmlNode root;
        record("lenient parse tolerates an unterminated comment",
               XmlParser::parse("<menu name=\"m\"><rect/></menu><!-- oops", root, err,
                               /*lenient=*/true));
        record("strict parse rejects an unterminated comment",
               !XmlParser::parse("<menu name=\"m\"><rect/></menu><!-- oops", root, err,
                                 /*lenient=*/false));
    }

    // --- 19. M4: MenuUiBuilder ------------------------------------------------------
        {
            using oblivion::ui::MenuNodeKind;
            using oblivion::ui::MenuScaleMode;
            using oblivion::ui::MenuUiBuilder;
            using oblivion::ui::MenuUiNode;

            // Texture path conversion -----------------------------------------------
            record("M4 texture path: full windows path",
                   MenuUiBuilder::convertTexturePath("Menus\\Loading\\loading_background.dds") ==
                   "textures/ui/loading_background.png");
            record("M4 texture path: forward slashes",
                   MenuUiBuilder::convertTexturePath("Textures/Menus/load_progress.dds") ==
                   "textures/ui/load_progress.png");
            record("M4 texture path: stray padding is trimmed",
                   MenuUiBuilder::convertTexturePath(" Menus\\Loading\\load_main.dds ") ==
                   "textures/ui/load_main.png");
            record("M4 texture path: bare file name",
                   MenuUiBuilder::convertTexturePath("load_symbol.dds") ==
                   "textures/ui/load_symbol.png");
            record("M4 texture path: non dds extension is replaced",
                   MenuUiBuilder::convertTexturePath("Menus\\foo.png") == "textures/ui/foo.png");
            record("M4 texture path: empty value",
                   MenuUiBuilder::convertTexturePath("").empty());
            record("M4 texture path: whitespace only",
                   MenuUiBuilder::convertTexturePath("   ").empty());
            record("M4 texture path: extensionless file",
                   MenuUiBuilder::convertTexturePath("Menus\\Loading\\logo") ==
                   "textures/ui/logo.png");
            // A file name of ".." or "." would collapse to nothing.
            record("M4 texture path: separator only",
                   MenuUiBuilder::convertTexturePath("Menus\\").empty());

            // Font index --------------------------------------------------------------
            record("M4 font index: padded value", MenuUiBuilder::fontIndex(" 1 ") == 1);
            record("M4 font index: empty", MenuUiBuilder::fontIndex("") == 0);
            record("M4 font index: maximum", MenuUiBuilder::fontIndex("5") == 5);
            record("M4 font index: above range falls back", MenuUiBuilder::fontIndex("9") == 0);
            record("M4 font index: negative falls back", MenuUiBuilder::fontIndex("-2") == 0);
            record("M4 font index: non numeric falls back", MenuUiBuilder::fontIndex("daedric") == 0);

            // buildNode: rectangle -----------------------------------------------------
            {
                std::string err;
                MenuDef def;
                const bool built = MenuXmlInterpreter::buildMenuDef(
                    "<menu name=\"m\">"
                    "<rect name=\"fg\"><x>10</x><y>20</y><width>100</width><height>50</height>"
                    "<alpha>48</alpha><red>255</red><green>128</green><blue>0</blue>"
                    "<visible>0</visible></rect>"
                    "</menu>",
                    def, err);
                const auto widgets = MenuXmlInterpreter::resolve(def, MenuEvalContext{});
                const MenuUiNode node =
                    widgets.empty() ? MenuUiNode{} : MenuUiBuilder::buildNode(widgets[0]);
                record("M4 rect: parses and resolves", built && widgets.size() == 1);
                record("M4 rect: kind", node.kind == MenuNodeKind::Rectangle);
                record("M4 rect: position and size",
                       near(node.x, 10.0f) && near(node.y, 20.0f) &&
                       near(node.width, 100.0f) && near(node.height, 50.0f));
                record("M4 rect: alpha is normalised to 0..1", near(node.alpha, 48.0f / 255.0f));
                record("M4 rect: rgb is normalised to 0..1",
                       near(node.red, 1.0f) && near(node.green, 128.0f / 255.0f) &&
                       near(node.blue, 0.0f));
                record("M4 rect: visible flag", !node.visible);
            }
            {
                std::string err;
                MenuDef def;
                MenuXmlInterpreter::buildMenuDef("<menu name=\"m\"><rect name=\"plain\"/></menu>",
                                                 def, err);
                const auto widgets = MenuXmlInterpreter::resolve(def, MenuEvalContext{});
                const MenuUiNode node = MenuUiBuilder::buildNode(widgets[0]);
                // A tile without <alpha> is fully opaque, not 1/255.
                record("M4 rect: alpha defaults to opaque when not authored",
                       near(node.alpha, 1.0f));
            }

            // buildNode: image -----------------------------------------------------------
            {
                std::string err;
                MenuDef def;
                MenuXmlInterpreter::buildMenuDef(
                    "<menu name=\"m\">"
                    "<image name=\"bg\"><filename>Menus\\Loading\\loading_background.dds"
                    "</filename><zoom> -1 </zoom></image>"
                    "</menu>",
                    def, err);
                MenuEvalContext ctx;
                ctx.texture_size = [](const std::string&, float& w, float& h) {
                    w = 1024.0f;
                    h = 768.0f;
                    return true;
                };
                const auto widgets = MenuXmlInterpreter::resolve(def, ctx);
                const MenuUiNode node = MenuUiBuilder::buildNode(widgets[0]);
                record("M4 image: kind", node.kind == MenuNodeKind::Image);
                record("M4 image: texture path", node.texture_path == "textures/ui/loading_background.png");
                record("M4 image: zoom < 0 fills the tile rect", node.fill_rect);
                record("M4 image: zoom < 0 fits inside the rect",
                       node.scale_mode == MenuScaleMode::Fit);
                record("M4 image: native texture size is carried through",
                       near(node.texture_width, 1024.0f) && near(node.texture_height, 768.0f));
                record("M4 image: zoom < 0 keeps unit rescale", near(node.zoom_scale, 1.0f));
            }
            {
                std::string err;
                MenuDef def;
                MenuXmlInterpreter::buildMenuDef(
                    "<menu name=\"m\"><image name=\"big\"><filename>x.dds</filename>"
                    "<zoom> 200 </zoom></image></menu>",
                    def, err);
                const auto widgets = MenuXmlInterpreter::resolve(def, MenuEvalContext{});
                const MenuUiNode node = MenuUiBuilder::buildNode(widgets[0]);
                record("M4 image: zoom > 0 is a percentage", near(node.zoom_scale, 2.0f));
                record("M4 image: zoom > 0 stretches", node.scale_mode == MenuScaleMode::Stretch);
            }

            // buildNode: text --------------------------------------------------------------
            {
                std::string err;
                MenuDef def;
                MenuXmlInterpreter::buildMenuDef(
                    "<menu name=\"m\">"
                    "<text name=\"t\"><string>Hello</string><font> 1 </font>"
                    "<justify> &center; </justify><wrapwidth> 850 </wrapwidth></text>"
                    "</menu>",
                    def, err);
                const auto widgets = MenuXmlInterpreter::resolve(def, MenuEvalContext{});
                const MenuUiNode node = MenuUiBuilder::buildNode(widgets[0]);
                record("M4 text: kind", node.kind == MenuNodeKind::Text);
                record("M4 text: string", node.text == "Hello");
                // <justify> &center; </justify> resolves through the entity reference
                // even without a strings.xml table.
                record("M4 text: justify resolves from an entity",
                       node.justify == "center");
                record("M4 text: font index", node.font_index == 1);
                record("M4 text: wrap width", near(node.wrap_width, 850.0f));
            }

            // buildNode: ignored tiles and recursion -----------------------------------------
            {
                std::string err;
                MenuDef def;
                MenuXmlInterpreter::buildMenuDef(
                    "<menu name=\"m\">"
                    "<mesh name=\"skip\"/>"
                    "<image name=\"a\"><filename>x.dds</filename>"
                    "<rect name=\"b\"><x>1</x><y>2</y></rect>"
                    "<text name=\"c\"><justify> &right; </justify><string>R</string></text>"
                    "</image>"
                    "</menu>",
                    def, err);
                const auto widgets = MenuXmlInterpreter::resolve(def, MenuEvalContext{});
                const MenuUiNode skip = MenuUiBuilder::buildNode(widgets[0]);
                const MenuUiNode image = MenuUiBuilder::buildNode(widgets[1]);
                record("M4 ignore: unknown widgets map to ignored", skip.kind == MenuNodeKind::Ignored);
                record("M4 tree: children are converted recursively",
                       image.children.size() == 2 &&
                       image.children[0].kind == MenuNodeKind::Rectangle &&
                       image.children[0].x == 1.0f && image.children[0].y == 2.0f &&
                       image.children[1].kind == MenuNodeKind::Text &&
                       image.children[1].justify == "right");
            }

            // build(): full menu in loading_menu.xml shape -------------------------------------
            {
                std::string err;
                MenuDef def;
                MenuXmlInterpreter::buildMenuDef(
                    "<menu name=\"Loading\">"
                    "<rect name=\"black\"><x>0</x><y>0</y><width>1920</width><height>1080</height>"
                    "<alpha>255</alpha><red>0</red><green>0</green><blue>0</blue></rect>"
                    "<image name=\"load_title_page\">"
                    "<filename>Menus\\Loading\\load_title_page.dds</filename><zoom> -1 </zoom>"
                    "</image>"
                    "<image name=\"load_main\">"
                    "<filename>Menus\\Loading\\loading_background.dds</filename><zoom> -1 </zoom>"
                    "<text name=\"load_text\"><string>Loading</string><font> 1 </font>"
                    "<justify> &center; </justify><wrapwidth> 850 </wrapwidth></text>"
                    "</image>"
                    "</menu>",
                    def, err);
                MenuEvalContext ctx;
                ctx.texture_size = [](const std::string&, float& w, float& h) {
                    w = 1024.0f;
                    h = 768.0f;
                    return true;
                };
                const ResolvedMenu menu = MenuXmlInterpreter::resolveMenu(def, ctx);
                const std::vector<MenuUiNode> nodes = MenuUiBuilder::build(menu);
                record("M4 build: parses the loading menu shape", menu.widgets.size() == 3);
                record("M4 build: converts every root in document order",
                       nodes.size() == 3 &&
                       nodes[0].kind == MenuNodeKind::Rectangle &&
                       nodes[1].kind == MenuNodeKind::Image &&
                       nodes[2].kind == MenuNodeKind::Image);
                record("M4 build: rect carries the loaded colour set",
                       near(nodes[0].alpha, 1.0f) && near(nodes[0].red, 0.0f));
                record("M4 build: image keeps its asset path and fit mode",
                       nodes[1].texture_path == "textures/ui/load_title_page.png" &&
                       nodes[1].scale_mode == MenuScaleMode::Fit);
                record("M4 build: nested label becomes a text child",
                       nodes[2].children.size() == 1 &&
                       nodes[2].children[0].kind == MenuNodeKind::Text &&
                       nodes[2].children[0].text == "Loading" &&
                       nodes[2].children[0].font_index == 1);
            }
        }

        const auto t1 = std::chrono::high_resolution_clock::now();
        const float totalMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
        record("suite completed", true, "", totalMs);

    return getFailCount() == 0;
}
