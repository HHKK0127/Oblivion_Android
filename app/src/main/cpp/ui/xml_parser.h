#pragma once

// Minimal subset XML parser for Oblivion menu definition files (menus/*.xml).
//
// The Oblivion menu format is a small declarative UI markup: a <menu> root with
// <rect>/<image>/<text>/<nif> widgets, operator elements (copy/add/sub/mul/...)
// and named traits. This parser deliberately covers only the subset needed for
// those files: elements, attributes, text, comments and the standard XML
// entities. Unknown entities (&name;) are preserved verbatim in the text so the
// menu evaluator can resolve them against strings.xml.
//
// Element and attribute names are lower cased: the shipped data mixes cases for
// the same element (<IMAGE NAME=...> in hud_main_menu.xml, <Text> elsewhere).
//
// Lenient mode (used for menus/*.xml and Menus\prefabs\*.xml) recovers from the
// markup errors present in the shipped data:
//   * a closing tag that names an enclosing element, with nothing but
//     anonymous traits opened inside it, closes every tag up to that element;
//     those traits stay attached to the element being closed (video_menu.xml
//     writes a stray "<y>" where "</y>" was meant);
//   * any other mismatched closing tag terminates the innermost element, so a
//     misnamed closer does not swallow the widgets that follow it
//     (quantity_menu.xml closes its scroll image with "</rect>");
//   * elements left open at end of input keep the children collected so far;
//   * stray closing tags at the top level are dropped.
// Strict mode (the default) rejects all of the above.

#include <map>
#include <string>
#include <vector>

namespace oblivion::ui {

// A parsed XML node tree. `text` holds the concatenated text content for leaf
// nodes (numeric/string literals and preserved entities).
struct XmlNode {
    std::string name;
    std::map<std::string, std::string> attributes;
    std::vector<XmlNode> children;
    std::string text;
    // Lenient mode only: set when this element was terminated by a closing tag
    // that names an enclosing element, i.e. the shipped data mistyped a closing
    // tag into an opening one. The value is the name of the element that tag
    // really closes; the element itself is spurious and its content belongs to
    // that ancestor. XmlParser lifts the content and drops the element.
    std::string lenient_adopted_by;
};

class XmlParser {
public:
    // Parse `xml` into a single-root node tree. Returns false and fills
    // `error` on malformed input (unclosed tags, mismatched closing tags, ...).
    //
    // `lenient` reproduces the loose reading the shipped engine applies to its
    // own data (see parseFragment). Keep it off for hand written XML.
    static bool parse(const std::string& xml, XmlNode& root, std::string& error,
                      bool lenient = false);

    // Parse zero or more top level elements. Menus\prefabs\*.xml are fragments
    // (several elements, no <menu> wrapper) that the includer merges into the
    // widget holding the <include>, so they cannot be read with parse().
    // Character data between the elements is skipped.
    //
    // `lenient` implements the recovery the shipped data needs (see the
    // XmlParser::kLenient* notes in the header banner); a strict parse still
    // rejects mismatched tags.
    static bool parseFragment(const std::string& xml, std::vector<XmlNode>& nodes,
                              std::string& error, bool lenient = false);
};

}  // namespace oblivion::ui
