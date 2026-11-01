// NIF parsing is now delegated to the Rust `nif-ffi` C ABI (see
// oblivion-re-tools/crates/nif-ffi). This file is a thin C++ shim that keeps the
// original NIFParser public API: it drives the Rust handle and materialises the
// cached NIFHeader / NIFNode / NIFGeometry members that the inline getters hand
// out. The heavy block walking, geometry decoding and Havok/skin/animation
// readers live in Rust; on the C++ side they are either FFI-backed or inert.

#include "nif_parser.h"
#include "nif_ffi.h"

#include <android/log.h>
#include <cstring>
#include <fstream>

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#define LOG_TAG "NIFParser"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

// snprintf-style string retrieval: query the needed length (including the
// terminating NUL), then fill a buffer of exactly that size. Every FFI text
// accessor follows this convention, so each needs its own typed wrapper because
// the function signatures differ in their index arguments.

std::string copyHeaderText(void* handle, int field) {
    const int need = nif_header_text(handle, field, nullptr, 0);
    if (need <= 0) return std::string();
    std::vector<char> buf(static_cast<size_t>(need));
    if (nif_header_text(handle, field, buf.data(), buf.size()) < 0) return std::string();
    return std::string(buf.data());
}

std::string copyHeaderBlockTypeName(void* handle, uint32_t typeIndex) {
    const int need = nif_header_block_type_name(handle, typeIndex, nullptr, 0);
    if (need <= 0) return std::string();
    std::vector<char> buf(static_cast<size_t>(need));
    if (nif_header_block_type_name(handle, typeIndex, buf.data(), buf.size()) < 0) return std::string();
    return std::string(buf.data());
}

std::string copyNodeText(void* handle, uint32_t nodeIndex, int field) {
    const int need = nif_node_text(handle, nodeIndex, field, nullptr, 0);
    if (need <= 0) return std::string();
    std::vector<char> buf(static_cast<size_t>(need));
    if (nif_node_text(handle, nodeIndex, field, buf.data(), buf.size()) < 0) return std::string();
    return std::string(buf.data());
}

std::string copyGeometryText(void* handle, uint32_t geomIndex, int field) {
    const int need = nif_geometry_text(handle, geomIndex, field, nullptr, 0);
    if (need <= 0) return std::string();
    std::vector<char> buf(static_cast<size_t>(need));
    if (nif_geometry_text(handle, geomIndex, field, buf.data(), buf.size()) < 0) return std::string();
    return std::string(buf.data());
}

std::string copyCollisionText(void* handle, int field) {
    const int need = nif_collision_text(handle, field, nullptr, 0);
    if (need <= 0) return std::string();
    std::vector<char> buf(static_cast<size_t>(need));
    if (nif_collision_text(handle, field, buf.data(), buf.size()) < 0) return std::string();
    return std::string(buf.data());
}

// The FFI transform array is ordered rotation(9) + translation(3) + scale(1),
// matching the C++ NIFTransform layout the original reader produced.
NIFTransform makeTransform(const float t[13]) {
    NIFTransform xf;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            xf.rotation.m[i][j] = t[i * 3 + j];
        }
    }
    xf.translation = NIFVector3(t[9], t[10], t[11]);
    xf.scale = t[12];
    return xf;
}

NIFMatrix3x3 makeMatrix3x3(const float r[9]) {
    NIFMatrix3x3 m;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            m.m[i][j] = r[i * 3 + j];
        }
    }
    return m;
}

CollisionShape convertShape(const nif_shape& s) {
    CollisionShape shape;
    shape.type = static_cast<CollisionShapeType>(s.type);
    shape.center = NIFVector3(s.center[0], s.center[1], s.center[2]);
    shape.halfExtents = NIFVector3(s.half_extents[0], s.half_extents[1], s.half_extents[2]);
    shape.radius = s.radius;
    shape.height = s.height;

    if (s.vertices && s.vertex_count) {
        shape.vertices.reserve(s.vertex_count);
        for (uint32_t i = 0; i < s.vertex_count; i++) {
            shape.vertices.push_back(NIFVector3(s.vertices[i * 3 + 0],
                                                s.vertices[i * 3 + 1],
                                                s.vertices[i * 3 + 2]));
        }
    }
    if (s.triangles && s.triangle_count) {
        shape.triangles.reserve(s.triangle_count);
        for (uint32_t i = 0; i < s.triangle_count; i++) {
            NIFTriangle tri;
            tri.v0 = s.triangles[i * 3 + 0];
            tri.v1 = s.triangles[i * 3 + 1];
            tri.v2 = s.triangles[i * 3 + 2];
            shape.triangles.push_back(tri);
        }
    }
    if (s.mopp_data && s.mopp_data_size) {
        shape.moppData.assign(s.mopp_data, s.mopp_data + s.mopp_data_size);
    }
    shape.moppDataSize = s.mopp_data_size;
    shape.childShapeRef = s.child_shape_ref;
    if (s.strips_data_refs && s.strips_ref_count) {
        shape.stripsDataRefs.assign(s.strips_data_refs, s.strips_data_refs + s.strips_ref_count);
    }
    if (s.sub_shape_refs && s.sub_shape_count) {
        shape.subShapeRefs.assign(s.sub_shape_refs, s.sub_shape_refs + s.sub_shape_count);
    }
    return shape;
}

RigidBodyInfo convertBody(const nif_rigid_body& b) {
    RigidBodyInfo info;
    info.mass = b.mass;
    info.friction = b.friction;
    info.restitution = b.restitution;
    info.linearVelocity = NIFVector3(b.linear_velocity[0], b.linear_velocity[1], b.linear_velocity[2]);
    info.angularVelocity = NIFVector3(b.angular_velocity[0], b.angular_velocity[1], b.angular_velocity[2]);
    info.transform = makeTransform(b.transform);
    info.collisionGroup = b.collision_group;
    info.collisionFilter = b.collision_filter;
    info.isTrigger = b.is_trigger != 0;
    return info;
}

NIFControllerSequence convertSequence(const nif_controller_sequence& src) {
    NIFControllerSequence seq;
    seq.name = src.name ? src.name : "";
    seq.targetName = src.target_name ? src.target_name : "";
    seq.controllerManagerIndex = src.controller_manager_index;
    seq.duration = src.duration;
    seq.frequency = src.frequency;
    seq.phase = src.phase;
    seq.startTime = src.start_time;
    seq.stopTime = src.stop_time;
    seq.loop = src.is_loop != 0;

    if (src.controlled_blocks && src.controlled_block_count) {
        seq.controlledBlocks.reserve(src.controlled_block_count);
        for (uint32_t i = 0; i < src.controlled_block_count; i++) {
            const nif_controlled_block& cb = src.controlled_blocks[i];
            NIFKeyframeController kfc;
            kfc.interpolatorIndex = cb.interpolator_index;
            kfc.controllerIndex = cb.controller_index;
            kfc.priority = cb.priority;
            kfc.stringPaletteIndex = cb.string_palette_index;
            kfc.nodeNameOffset = cb.node_name_offset;
            kfc.propertyTypeOffset = cb.property_type_offset;
            kfc.controllerTypeOffset = cb.controller_type_offset;
            kfc.controllerIdOffset = cb.controller_id_offset;
            kfc.interpolatorIdOffset = cb.interpolator_id_offset;
            kfc.keyframeDataIndex = cb.keyframe_data_index;
            kfc.resolvedBoneIndex = cb.resolved_bone_index;
            seq.controlledBlocks.push_back(kfc);
        }
    }
    if (src.text_keys && src.text_key_count) {
        seq.textKeys.reserve(src.text_key_count);
        for (uint32_t i = 0; i < src.text_key_count; i++) {
            const nif_text_key& tk = src.text_keys[i];
            NIFTextKey key;
            key.time = tk.time;
            key.value = tk.value ? tk.value : "";
            seq.textKeys.push_back(key);
        }
    }
    return seq;
}

}  // namespace

NIFParser::NIFParser() : m_handle(nif_create()) {}

NIFParser::~NIFParser() {
    if (m_handle) {
        nif_destroy(m_handle);
        m_handle = nullptr;
    }
}

bool NIFParser::parseFile(const std::string& filepath) {
    LOGD("=== Starting NIF parse: %s ===", filepath.c_str());

    std::ifstream in(filepath, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        LOGE("Failed to open NIF file: %s", filepath.c_str());
        return false;
    }

    const std::streamoff fileSize = in.tellg();
    if (fileSize <= 0) {
        LOGE("Empty or unreadable NIF file: %s", filepath.c_str());
        in.close();
        return false;
    }

    std::vector<uint8_t> data(static_cast<size_t>(fileSize));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(data.data()), fileSize);
    const bool shortRead = (in.gcount() != fileSize);
    in.close();
    if (shortRead) {
        LOGE("Short read on NIF file: %s", filepath.c_str());
        return false;
    }

    return parseFromData(data);
}

bool NIFParser::parseFromData(const std::vector<uint8_t>& data) {
    LOGD("=== Starting NIF parse from buffer (%zu bytes) ===", data.size());

    // Reset the cached view; the Rust handle replaces its own state on parse.
    nodes.clear();
    rootNodeIndices.clear();
    fileBuffer = data;
    cursor = 0;
    header = NIFHeader();

    if (!m_handle) {
        LOGE("NIF handle is null");
        return false;
    }
    if (fileBuffer.empty()) {
        LOGE("Empty NIF buffer");
        fileBuffer.clear();
        return false;
    }

    const int rc = nif_parse_memory(m_handle, fileBuffer.data(), fileBuffer.size());

    // A parse that stopped early still leaves a partial result to inspect, so
    // rebuild the cache whether or not the parse succeeded.
    const bool synced = syncCache();
    if (rc != NIF_OK) {
        LOGE("Rust NIF parse failed: %d (%s)", rc, nif_last_error(m_handle));
    }
    return rc == NIF_OK && synced;
}

bool NIFParser::syncCache() {
    if (!m_handle) {
        return false;
    }

    header = NIFHeader();
    nodes.clear();
    rootNodeIndices.clear();

    // Magic line and other header strings.
    const std::string magic = copyHeaderText(m_handle, NIF_HEADER_FIELD_MAGIC);
    std::memset(header.magic, 0, sizeof(header.magic));
    if (!magic.empty()) {
        std::strncpy(header.magic, magic.c_str(), sizeof(header.magic) - 1);
    }
    header.creator = copyHeaderText(m_handle, NIF_HEADER_FIELD_CREATOR);
    header.processScript = copyHeaderText(m_handle, NIF_HEADER_FIELD_PROCESS_SCRIPT);
    header.exportScript = copyHeaderText(m_handle, NIF_HEADER_FIELD_EXPORT_SCRIPT);

    nif_header_info hi;
    std::memset(&hi, 0, sizeof(hi));
    if (nif_get_header_info(m_handle, &hi) != NIF_OK) {
        LOGW("nif_get_header_info failed: %s", nif_last_error(m_handle));
        return false;
    }

    header.version = hi.version;
    header.endian = hi.endian;
    header.hasEndianByte = hi.has_endian_byte != 0;
    header.legacyNetImmerse = hi.legacy_net_immerse != 0;
    header.hasBlockTypeTable = hi.has_block_type_table != 0;
    header.userVersion = hi.user_version;
    header.numObjects = hi.num_objects;
    header.unknownField = hi.unknown_field;
    header.numGroups = hi.num_groups;
    header.blockDataOffset = static_cast<size_t>(hi.block_data_offset);

    header.blockTypeNames.clear();
    header.blockTypeNames.reserve(hi.block_type_count);
    for (uint32_t i = 0; i < hi.block_type_count; i++) {
        header.blockTypeNames.push_back(copyHeaderBlockTypeName(m_handle, i));
    }

    header.blockTypeIndices.clear();
    if (hi.block_type_indices && hi.num_objects) {
        header.blockTypeIndices.assign(hi.block_type_indices,
                                       hi.block_type_indices + hi.num_objects);
    }

    // Object array -> NIFNode table. The parent index crosses as the parser
    // leaves it (-1 for every node) and child lists stay empty, matching the
    // original reader which never wired the scene graph.
    const uint32_t blockCount = nif_block_count(m_handle);
    nodes.reserve(blockCount);
    for (uint32_t i = 0; i < blockCount; i++) {
        nif_node_info ni;
        std::memset(&ni, 0, sizeof(ni));
        if (nif_get_node_info(m_handle, i, &ni) != NIF_OK) {
            continue;
        }
        auto node = std::make_shared<NIFNode>();
        node->nodeIndex = ni.node_index;
        node->parentIndex = ni.parent_index;
        node->blockTypeIndex = static_cast<uint16_t>(ni.block_type_index);
        node->hasGeometry = ni.has_geometry != 0;
        node->name = copyNodeText(m_handle, i, NIF_NODE_FIELD_NAME);
        node->blockTypeName = copyNodeText(m_handle, i, NIF_NODE_FIELD_BLOCK_TYPE_NAME);
        nodes.push_back(std::move(node));
    }

    const uint32_t rootCount = nif_root_node_count(m_handle);
    rootNodeIndices.reserve(rootCount);
    for (uint32_t i = 0; i < rootCount; i++) {
        uint32_t idx = 0;
        if (nif_root_node_at(m_handle, i, &idx) == NIF_OK) {
            rootNodeIndices.push_back(static_cast<int32_t>(idx));
        }
    }

    // Decoded meshes: attach each geometry to its owning block node.
    uint32_t geomCount = 0;
    if (nif_geometry_count(m_handle, &geomCount) == NIF_OK && geomCount > 0) {
        std::vector<nif_geometry_info> geoms(geomCount);
        const int total = nif_get_geometries(m_handle, geoms.data(), geomCount);
        if (total > 0) {
            for (uint32_t g = 0; g < geomCount; g++) {
                uint32_t blockIdx = 0;
                if (nif_geometry_block_index(m_handle, g, &blockIdx) != NIF_OK) continue;
                if (blockIdx >= nodes.size() || !nodes[blockIdx]) continue;

                const nif_geometry_info& gi = geoms[g];
                NIFGeometry geom;
                geom.name = copyGeometryText(m_handle, g, NIF_GEOM_FIELD_NAME);
                geom.diffuseTexture = copyGeometryText(m_handle, g, NIF_GEOM_FIELD_DIFFUSE_TEXTURE);
                geom.normalTexture = copyGeometryText(m_handle, g, NIF_GEOM_FIELD_NORMAL_TEXTURE);
                geom.transform = makeTransform(gi.transform);
                geom.materialPropertyIndex = gi.material_property_index;
                geom.texturingPropertyIndex = gi.texturing_property_index;

                if (gi.vertices) {
                    geom.vertices.reserve(gi.vertex_count);
                    for (uint32_t i = 0; i < gi.vertex_count; i++) {
                        geom.vertices.push_back(NIFVector3(gi.vertices[i * 3 + 0],
                                                           gi.vertices[i * 3 + 1],
                                                           gi.vertices[i * 3 + 2]));
                    }
                }
                if (gi.normals) {
                    geom.normals.reserve(gi.normal_count);
                    for (uint32_t i = 0; i < gi.normal_count; i++) {
                        geom.normals.push_back(NIFVector3(gi.normals[i * 3 + 0],
                                                          gi.normals[i * 3 + 1],
                                                          gi.normals[i * 3 + 2]));
                    }
                }
                if (gi.colors) {
                    geom.colors.reserve(gi.color_count);
                    for (uint32_t i = 0; i < gi.color_count; i++) {
                        geom.colors.push_back(NIFVector4(gi.colors[i * 4 + 0],
                                                         gi.colors[i * 4 + 1],
                                                         gi.colors[i * 4 + 2],
                                                         gi.colors[i * 4 + 3]));
                    }
                }
                if (gi.tex_coords) {
                    geom.texCoords.reserve(gi.texcoord_count);
                    for (uint32_t i = 0; i < gi.texcoord_count; i++) {
                        geom.texCoords.push_back(glm::vec2(gi.tex_coords[i * 2 + 0],
                                                           gi.tex_coords[i * 2 + 1]));
                    }
                }
                if (gi.triangles) {
                    geom.triangles.reserve(gi.triangle_count);
                    for (uint32_t i = 0; i < gi.triangle_count; i++) {
                        NIFTriangle tri;
                        tri.v0 = gi.triangles[i * 3 + 0];
                        tri.v1 = gi.triangles[i * 3 + 1];
                        tri.v2 = gi.triangles[i * 3 + 2];
                        geom.triangles.push_back(tri);
                    }
                }

                nodes[blockIdx]->geometry = std::move(geom);
                nodes[blockIdx]->hasGeometry = true;
            }
        }
    }

    LOGD("NIF cache synced: version=0x%08X, userVersion=%u, numBlocks=%u, blockTypes=%zu",
         header.version, header.userVersion, header.numObjects, header.blockTypeNames.size());
    return true;
}

std::shared_ptr<NIFNode> NIFParser::getNodeByName(const std::string& name) const {
    for (const auto& node : nodes) {
        if (node && node->name == name) {
            return node;
        }
    }
    return nullptr;
}

std::vector<NIFGeometry> NIFParser::extractAllGeometry() const {
    std::vector<NIFGeometry> geometries;
    for (const auto& node : nodes) {
        if (node && node->hasGeometry) {
            geometries.push_back(node->geometry);
        }
    }
    return geometries;
}

std::string NIFParser::getBlockTypeName(uint32_t index) const {
    if (index >= header.blockTypeIndices.size()) {
        return std::string();
    }
    const uint16_t typeIndex = header.blockTypeIndices[index];
    if (typeIndex >= header.blockTypeNames.size()) {
        return std::string();
    }
    return header.blockTypeNames[typeIndex];
}

bool NIFParser::hasBlockType(const std::string& typeName) const {
    for (uint32_t i = 0; i < header.blockTypeIndices.size(); i++) {
        if (getBlockTypeName(i) == typeName) {
            return true;
        }
    }
    return false;
}

// ============================================================
// Skinning
// ============================================================

bool NIFParser::parseNiSkinInstance(NIFSkinInstance& skin) {
    nif_skin_instance rec;
    std::memset(&rec, 0, sizeof(rec));
    if (nif_parse_skin_instance(m_handle, &rec) != NIF_OK) {
        return false;
    }
    skin.skinDataIndex = rec.skin_data_index;
    skin.skinPartitionIndex = rec.skin_partition_index;
    skin.skeletonRootIndex = rec.skeleton_root_index;
    skin.boneNodeIndices.clear();
    if (rec.bone_node_indices && rec.bone_count) {
        skin.boneNodeIndices.assign(rec.bone_node_indices,
                                    rec.bone_node_indices + rec.bone_count);
    }
    return true;
}

bool NIFParser::parseNiSkinData(NIFSkinData& skinData) {
    nif_skin_data rec;
    std::memset(&rec, 0, sizeof(rec));
    if (nif_parse_skin_data(m_handle, &rec) != NIF_OK) {
        return false;
    }
    skinData.rootRotation = makeMatrix3x3(rec.root_rotation);
    skinData.rootTranslation = NIFVector3(rec.root_translation[0], rec.root_translation[1],
                                          rec.root_translation[2]);
    skinData.rootScale = rec.root_scale;
    skinData.numBones = rec.num_bones;

    skinData.boneData.clear();
    if (rec.bone_data && rec.num_bones) {
        skinData.boneData.reserve(rec.num_bones);
        for (uint32_t i = 0; i < rec.num_bones; i++) {
            const nif_bone_data& bd = rec.bone_data[i];
            NIFBoneData bone;
            bone.skinTransform = makeTransform(bd.skin_transform);
            if (bd.vertex_weights && bd.vertex_weight_count) {
                bone.vertexWeights.reserve(bd.vertex_weight_count);
                for (uint32_t j = 0; j < bd.vertex_weight_count; j++) {
                    NIFVertexWeight vw;
                    vw.vertexIndex = bd.vertex_weights[j].vertex_index;
                    vw.weight = bd.vertex_weights[j].weight;
                    bone.vertexWeights.push_back(vw);
                }
            }
            skinData.boneData.push_back(std::move(bone));
        }
    }
    return true;
}

bool NIFParser::parseNiSkinPartition(NIFSkinPartition& partition) {
    nif_skin_partition rec;
    std::memset(&rec, 0, sizeof(rec));
    if (nif_parse_skin_partition(m_handle, &rec) != NIF_OK) {
        return false;
    }
    partition.maxBonesPerPartition = rec.max_bones_per_partition;
    partition.maxBonesPerVertex = rec.max_bones_per_vertex;

    partition.partitions.clear();
    if (rec.partitions && rec.partition_count) {
        partition.partitions.reserve(rec.partition_count);
        for (uint32_t i = 0; i < rec.partition_count; i++) {
            const nif_partition& p = rec.partitions[i];
            NIFSkinPartition::Partition part;
            part.numVertices = p.num_vertices;
            part.numTriangles = p.num_triangles;
            if (p.bone_palette && p.bone_count) {
                part.bonePalette.bones.assign(p.bone_palette, p.bone_palette + p.bone_count);
            }
            if (p.packed_weights && p.packed_weight_count) {
                part.packedWeights.reserve(p.packed_weight_count);
                for (uint32_t j = 0; j < p.packed_weight_count; j++) {
                    GPUVertexWeight gw;
                    for (int k = 0; k < 4; k++) {
                        gw.boneIndices[k] = p.packed_weights[j].bone_indices[k];
                        gw.weights[k] = p.packed_weights[j].weights[k];
                    }
                    part.packedWeights.push_back(gw);
                }
            }
            if (p.indices && p.index_count) {
                part.indices.assign(p.indices, p.indices + p.index_count);
                // Triangles are three consecutive indices each.
                for (uint32_t t = 0; t + 2 < p.index_count; t += 3) {
                    NIFTriangle tri;
                    tri.v0 = p.indices[t + 0];
                    tri.v1 = p.indices[t + 1];
                    tri.v2 = p.indices[t + 2];
                    part.triangles.push_back(tri);
                }
            }
            partition.partitions.push_back(std::move(part));
        }
    }
    return true;
}

// ============================================================
// Animation
// ============================================================

bool NIFParser::parseNiControllerManager(NIFControllerManager& manager) {
    nif_controller_manager rec;
    std::memset(&rec, 0, sizeof(rec));
    if (nif_parse_controller_manager(m_handle, &rec) != NIF_OK) {
        return false;
    }
    manager.controllerSequenceCount = rec.controller_sequence_count;
    manager.objectPaletteIndex = rec.object_palette_index;
    manager.lastTime = rec.last_time;

    manager.sequences.clear();
    if (rec.sequences && rec.sequence_count) {
        manager.sequences.reserve(rec.sequence_count);
        for (uint32_t i = 0; i < rec.sequence_count; i++) {
            manager.sequences.push_back(convertSequence(rec.sequences[i]));
        }
    }
    return true;
}

bool NIFParser::parseNiControllerSequence(NIFControllerSequence& sequence) {
    nif_controller_sequence rec;
    std::memset(&rec, 0, sizeof(rec));
    if (nif_parse_controller_sequence(m_handle, &rec) != NIF_OK) {
        return false;
    }
    sequence = convertSequence(rec);
    return true;
}

// The keyframe / transform controller readers have no Rust counterpart yet; the
// original also left them unresolved without a walker. Keep them inert so the
// public API still links and reports "nothing found".
bool NIFParser::parseNiKeyframeController(NIFKeyframeController& /*controller*/) {
    return false;
}

bool NIFParser::parseNiKeyframeData(NIFAnimationClip& /*clip*/) {
    return false;
}

bool NIFParser::parseNiTextKeyExtraData(std::vector<NIFTextKey>& /*textKeys*/) {
    return false;
}

bool NIFParser::parseNiTransformController(NIFTransformController& /*controller*/) {
    return false;
}

bool NIFParser::resolveControllerReferences(
    NIFControllerManager& /*manager*/,
    const std::vector<NIFKeyframeData>& /*keyframeDataArray*/) {
    return false;
}

// ============================================================
// Physics (Havok)
// ============================================================

bool NIFParser::parseBhkCollisionObject(CollisionObject& obj) {
    nif_collision_object rec;
    std::memset(&rec, 0, sizeof(rec));
    if (nif_parse_collision(m_handle, &rec) != NIF_OK) {
        return false;
    }
    obj.nodeIndex = rec.node_index;
    obj.rigidBodyIndex = rec.rigid_body_index;
    obj.bodyInfo = convertBody(rec.body);
    obj.shape = convertShape(rec.shape);
    obj.targetName = copyCollisionText(m_handle, NIF_COLL_FIELD_TARGET_NAME);
    return true;
}

bool NIFParser::parseBhkRigidBody(RigidBodyInfo& info) {
    nif_rigid_body rec;
    std::memset(&rec, 0, sizeof(rec));
    const int rc = nif_parse_rigid_body(m_handle, &rec);
    if (rc != NIF_OK && rc != NIF_ERROR_INCOMPLETE) {
        return false;
    }
    info = convertBody(rec);
    // INCOMPLETE reproduces the original's short-block legacy path: the record
    // is filled with what was read but the reader reports failure.
    return rc == NIF_OK;
}

bool NIFParser::parseBhkShape(CollisionShape& shape) {
    nif_shape rec;
    std::memset(&rec, 0, sizeof(rec));
    if (nif_parse_shape(m_handle, &rec) != NIF_OK) {
        return false;
    }
    shape = convertShape(rec);
    return true;
}

// Per-type shape readers have no separate Rust entry points; nif_parse_shape
// already resolves the first shape block of any concrete type. Keep these inert.
bool NIFParser::parseBhkBoxShape(CollisionShape& /*shape*/) { return false; }
bool NIFParser::parseBhkSphereShape(CollisionShape& /*shape*/) { return false; }
bool NIFParser::parseBhkCapsuleShape(CollisionShape& /*shape*/) { return false; }
bool NIFParser::parseBhkConvexVerticesShape(CollisionShape& /*shape*/) { return false; }
bool NIFParser::parseBhkMeshShape(CollisionShape& /*shape*/) { return false; }
bool NIFParser::parseBhkNiTriStripsShape(CollisionShape& /*shape*/) { return false; }
bool NIFParser::parseBhkMoppBvTreeShape(CollisionShape& /*shape*/) { return false; }
bool NIFParser::parseBhkListShape(CollisionShape& /*shape*/) { return false; }

bool NIFParser::parseBhkCollisionFilter(uint32_t& /*group*/, uint32_t& /*filter*/) {
    return false;
}
