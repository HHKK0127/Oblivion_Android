#include "xml_parser.h"

#include <cctype>

namespace oblivion::ui {

namespace {

bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Element and attribute names are matched case insensitively in the shipped
// data, so they are folded to lower case on the way in.
std::string lowerAscii(const std::string& in) {
    std::string out = in;
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

// Decode the five standard XML entities. Everything else (&name;) is left
// untouched so the evaluator can resolve it against strings.xml later.
std::string decodeEntities(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '&') {
            const size_t semi = in.find(';', i);
            if (semi != std::string::npos) {
                const std::string ent = in.substr(i + 1, semi - i - 1);
                if (ent == "lt") { out += '<'; i = semi; continue; }
                if (ent == "gt") { out += '>'; i = semi; continue; }
                if (ent == "amp") { out += '&'; i = semi; continue; }
                if (ent == "quot") { out += '"'; i = semi; continue; }
                if (ent == "apos") { out += '\''; i = semi; continue; }
            }
        }
        out += in[i];
    }
    return out;
}

// Recursive-descent scanner over the input string.
struct Scanner {
    const std::string& s;
    size_t pos = 0;
    std::string error{};
    bool lenient = false;
    // Set by parseElement() in lenient mode when it stopped at a closing tag
    // that names an enclosing element: the tag is left unconsumed so each
    // ancestor can decide whether it belongs to it.
    std::string pending_close{};
    // (name, has-name-attribute) for every element currently open, outermost
    // first. Only used in lenient mode to tell widgets from anonymous traits
    // when a closing tag does not match the element it terminates.
    std::vector<std::pair<std::string, bool>> open_stack{};

    // Skip whitespace, comments and processing instructions/declarations.
    // Returns false (with `error` set) in strict mode when a comment or
    // declaration is never terminated; lenient mode treats the remainder of the
    // document as comment text, which is what the shipped menus rely on.
    bool skipMisc() {
        while (pos < s.size()) {
            if (s.compare(pos, 4, "<!--") == 0) {
                const size_t end = s.find("-->", pos + 4);
                if (end == std::string::npos) {
                    pos = s.size();
                    if (!lenient) error = "unterminated comment";
                    return lenient;
                }
                pos = end + 3;
                continue;
            }
            if (s.compare(pos, 2, "<?") == 0 || s.compare(pos, 2, "<!") == 0) {
                const size_t end = s.find('>', pos + 2);
                if (end == std::string::npos) {
                    pos = s.size();
                    if (!lenient) error = "unterminated declaration";
                    return lenient;
                }
                pos = end + 1;
                continue;
            }
            if (isWhitespace(s[pos])) {
                ++pos;
                continue;
            }
            break;
        }
        return true;
    }

    std::string readName() {
        const size_t start = pos;
        while (pos < s.size() && (std::isalnum(static_cast<unsigned char>(s[pos])) ||
                                  s[pos] == '_' || s[pos] == '-' || s[pos] == ':') ) {
            ++pos;
        }
        return s.substr(start, pos - start);
    }

    // Read a sequence of attributes into `attrs`. Sets `error` on failure.
    void readAttributes(std::map<std::string, std::string>& attrs) {
        while (true) {
            while (pos < s.size() && isWhitespace(s[pos])) ++pos;
            if (pos >= s.size()) { error = "unexpected end of tag"; return; }
            if (s[pos] == '>' || (s[pos] == '/' && pos + 1 < s.size() && s[pos + 1] == '>')) return;
            const std::string name = readName();
            if (name.empty()) { ++pos; continue; }
            while (pos < s.size() && isWhitespace(s[pos])) ++pos;
            std::string value;
            if (pos < s.size() && s[pos] == '=') {
                ++pos;
                while (pos < s.size() && isWhitespace(s[pos])) ++pos;
                if (pos < s.size() && (s[pos] == '"' || s[pos] == '\'')) {
                    const char q = s[pos++];
                    const size_t end = s.find(q, pos);
                    if (end == std::string::npos) { error = "unterminated attribute value"; return; }
                    value = s.substr(pos, end - pos);
                    pos = end + 1;
                } else {
                    const size_t start = pos;
                    while (pos < s.size() && !isWhitespace(s[pos]) && s[pos] != '>') ++pos;
                    value = s.substr(start, pos - start);
                }
            }
            attrs[lowerAscii(name)] = decodeEntities(value);
        }
    }

    // Parse one element (including its children) into `node`, keeping the open
    // element stack in sync for the lenient recovery below.
    bool parseElement(XmlNode& node) {
        const size_t mark = open_stack.size();
        const bool ok = parseElementInner(node);
        open_stack.resize(mark);
        return ok;
    }

    bool parseElementInner(XmlNode& node) {
        if (!skipMisc()) return false;
        if (pos >= s.size() || s[pos] != '<') { error = "expected '<'"; return false; }
        ++pos;  // '<'
        if (pos < s.size() && s[pos] == '/') { error = "unexpected closing tag"; return false; }
        node.name = lowerAscii(readName());
        if (node.name.empty()) { error = "empty tag name"; return false; }
        readAttributes(node.attributes);
        if (!error.empty()) return false;
        if (pos < s.size() && s[pos] == '/') {
            if (pos + 1 >= s.size() || s[pos + 1] != '>') { error = "expected '/>'"; return false; }
            pos += 2;
            return true;  // self-closing
        }
        if (pos >= s.size() || s[pos] != '>') { error = "expected '>'"; return false; }
        ++pos;  // '>'
        open_stack.emplace_back(node.name, node.attributes.count("name") != 0);

        while (true) {
            if (!skipMisc()) return false;
            if (pos >= s.size()) {
                // Lenient: keep whatever the element managed to collect.
                if (lenient) return true;
                error = "unexpected end of input in <" + node.name + ">";
                return false;
            }
            if (s[pos] == '<') {
                if (pos + 1 < s.size() && s[pos + 1] == '/') {
                    const size_t tag_start = pos;
                    pos += 2;
                    const std::string closing = lowerAscii(readName());
                    if (closing.empty() && lenient) return true;  // "</>" closes this element
                    const size_t end = s.find('>', pos);
                    if (end == std::string::npos) { error = "unterminated closing tag"; return false; }
                    if (closing == node.name) {
                        pos = end + 1;
                        // This element owned a tag a descendant handed back;
                        // the elements counting on it resume after it.
                        pending_close.clear();
                        return true;
                    }
                    if (!lenient) {
                        error = "mismatched closing tag </" + closing + "> expected </" + node.name + ">";
                        return false;
                    }
                    // Lenient: the shipped menus get closing tag names wrong in
                    // two ways. Either the closer names an enclosing element and
                    // everything opened inside it are anonymous traits that
                    // belong to it (video_menu.xml writes a stray "<y>" where
                    // "</y>" was meant) - then the tag is handed back and the
                    // ancestors are closed; or the closer is simply misnamed
                    // while the nesting is right (quantity_menu.xml closes its
                    // scroll image with "</rect>") - then it terminates this
                    // element and a named widget in between is preserved.
                    bool adopt = false;
                    for (size_t k = open_stack.size(); k-- > 0;) {
                        if (open_stack[k].first != closing) continue;
                        adopt = true;
                        for (size_t j = k + 1; j < open_stack.size(); ++j) {
                            if (open_stack[j].second) { adopt = false; break; }
                        }
                        break;
                    }
                    if (adopt) {
                        pos = tag_start;
                        pending_close = closing;
                        // The data meant this tag to close an enclosing element,
                        // so this element is a mistyping of it. Marking the name
                        // lets the repair pass move the element's content to the
                        // element that tag really closed.
                        node.lenient_adopted_by = closing;
                        return true;
                    }
                    pos = end + 1;
                    return true;
                }
                XmlNode child;
                if (!parseElement(child)) return false;
                node.children.push_back(std::move(child));
                // A descendant handed a closing tag back to an ancestor. The
                // element that owns it must keep parsing so it consumes the tag
                // itself; the elements in between are done.
                if (lenient && !pending_close.empty() && pending_close != node.name) return true;
            } else {
                const size_t start = pos;
                const size_t end = s.find('<', pos);
                if (end == std::string::npos) {
                    if (lenient) {
                        node.text += decodeEntities(s.substr(start));
                        pos = s.size();
                        return true;
                    }
                    error = "unexpected end of input in text";
                    return false;
                }
                node.text += decodeEntities(s.substr(start, end - start));
                pos = end;
            }
        }
    }
};

}  // namespace

namespace {

// Content written after a mistyped tag belongs to the element that tag really
// closed, not to the anonymous trait it was nested in. Returns the nodes still
// waiting for an ancestor whose name matches, as (target element name, node).
std::vector<std::pair<std::string, XmlNode>> repairAdoptions(XmlNode& node) {
    std::vector<XmlNode> kept;
    kept.reserve(node.children.size());
    std::vector<std::pair<std::string, XmlNode>> pending;

    for (XmlNode& child : node.children) {
        std::vector<std::pair<std::string, XmlNode>> up = repairAdoptions(child);
        if (!child.lenient_adopted_by.empty()) {
            const std::string target = child.lenient_adopted_by;
            for (XmlNode& grand : child.children) {
                pending.emplace_back(target, std::move(grand));
            }
            continue;  // the mistyped element itself is dropped
        }
        for (auto& item : up) {
            pending.push_back(std::move(item));
        }
        kept.push_back(std::move(child));
    }

    // Absorbing in place would reorder content that came after the mistyped
    // tag, so waiting nodes are appended once the element's own children are
    // complete.
    for (auto it = pending.begin(); it != pending.end();) {
        if (it->first == node.name) {
            kept.push_back(std::move(it->second));
            it = pending.erase(it);
        } else {
            ++it;
        }
    }
    node.children = std::move(kept);
    return pending;
}

}  // namespace

bool XmlParser::parse(const std::string& xml, XmlNode& root, std::string& error,
                      bool lenient) {
    Scanner sc{xml};
    sc.lenient = lenient;
    XmlNode node;
    if (!sc.parseElement(node)) {
        error = sc.error.empty() ? "parse error" : sc.error;
        return false;
    }
    root = std::move(node);
    if (lenient) {
        // Content whose mistyped owner was never found is kept at the top level
        // rather than dropped.
        for (auto& item : repairAdoptions(root)) {
            root.children.push_back(std::move(item.second));
        }
    }
    if (!sc.skipMisc()) {
        error = sc.error;
        return false;
    }
    if (sc.pos < sc.s.size()) {
        if (!lenient) {
            error = "trailing content after root element";
            return false;
        }
        // Lenient: surplus closing tags of a deeply mistyped document are
        // dropped rather than failing the whole menu.
    }
    return true;
}

bool XmlParser::parseFragment(const std::string& xml, std::vector<XmlNode>& nodes,
                              std::string& error, bool lenient) {
    nodes.clear();
    Scanner sc{xml};
    sc.lenient = lenient;
    while (true) {
        if (!sc.skipMisc()) {
            error = sc.error;
            return false;
        }
        if (sc.pos >= sc.s.size()) break;
        if (sc.s[sc.pos] != '<') {
            // Character data between top level elements has no owner; skip it.
            const size_t next = sc.s.find('<', sc.pos);
            if (next == std::string::npos) break;
            sc.pos = next;
            continue;
        }
        if (sc.pos + 1 < sc.s.size() && sc.s[sc.pos + 1] == '/') {
            if (!lenient) { error = "unexpected closing tag"; return false; }
            const size_t end = sc.s.find('>', sc.pos);
            if (end == std::string::npos) break;
            sc.pos = end + 1;
            sc.pending_close.clear();
            continue;
        }
        XmlNode node;
        if (!sc.parseElement(node)) {
            error = sc.error.empty() ? "parse error" : sc.error;
            return false;
        }
        if (lenient) {
            for (auto& item : repairAdoptions(node)) {
                node.children.push_back(std::move(item.second));
            }
        }
        nodes.push_back(std::move(node));
    }
    return true;
}

}  // namespace oblivion::ui
