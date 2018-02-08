#include "pch.h"
#include "usdiInternal.h"
#include "usdiAttribute.h"
#include "usdiSchema.h"
#include "usdiContext.h"
#include "usdiUtils.h"

namespace usdi {

RegisterSchemaHandler(Schema)

Schema::Schema(Context *ctx, Schema *parent, Schema *master, const std::string& path, const UsdPrim& p)
    : m_ctx(ctx)
    , m_parent(parent)
    , m_master(master)
    , m_path(path)
    , m_prim(p)
{
    init();
}

Schema::Schema(Context *ctx, Schema *parent, const UsdPrim& p)
    : m_ctx(ctx)
    , m_parent(parent)
    , m_id(ctx->generateID())
    , m_prim(p)
{
    init();
}

Schema::Schema(Context *ctx, Schema *parent, const char *name, const char *type)
    : m_ctx(ctx)
    , m_parent(parent)
    , m_id(ctx->generateID())
{
    m_prim = ctx->getUsdStage()->DefinePrim(SdfPath(makePath(name)), TfToken(type));
    if (ctx->getExportSettings().instanceable_by_default) {
        m_prim.SetInstanceable(true);
    }
    init();
}

void Schema::init()
{
    if (m_parent) { m_parent->addChild(this); }
    if (m_master) { m_master->addInstance(this); }
    if (m_prim && !m_master) {
        m_path = m_prim.GetPath().GetString();
        syncAttributes();
        syncTimeRange();
        syncVariantSets();
    }
}

void Schema::setup()
{
}

void Schema::syncAttributes()
{
    m_attributes.clear();
    auto attrs = m_prim.GetAuthoredAttributes();
    for (auto attr : attrs) {
        if (auto *ret = WrapExistingAttribute(this, attr)) {
            m_attributes.emplace_back(ret);
        }
    }
}

void Schema::syncTimeRange()
{
    double lower = usdiInvalidTime;
    double upper = usdiInvalidTime;
    for (auto& a : m_attributes) {
        double l, u;
        a->getTimeRange(l, u);
        if (!std::isnan(l)) {
            if (std::isnan(lower)) {
                lower = l;
                upper = u;
            }
            else {
                lower = std::min(lower, l);
                upper = std::max(upper, u);
            }
        }
    }
    m_time_start = lower;
    m_time_end = upper;
}

void Schema::syncVariantSets()
{
    std::vector<std::string> names;
    auto vsets = m_prim.GetVariantSets();
    vsets.GetNames(&names);
    m_variant_sets.resize(names.size());
    for (size_t i = 0; i < names.size(); ++i) {
        auto vset = vsets.GetVariantSet(names[i]);
        m_variant_sets[i].name = names[i];
        m_variant_sets[i].variants = vset.GetVariantNames();
    }
}

Schema::~Schema()
{
    m_attributes.clear();
}

Context*    Schema::getContext() const      { return m_ctx; }
int         Schema::getID() const           { return m_id; }
const char* Schema::getPath() const { return m_path.c_str(); }
const char* Schema::getName() const { return m_prim.GetName().GetText(); }
const char* Schema::getUsdTypeName() const { return m_prim.GetTypeName().GetText(); }
UsdPrim Schema::getUsdPrim() const { return m_prim; }

void Schema::getTimeRange(Time& start, Time& end) const
{
    start = m_time_start;
    end = m_time_end;
}

// attribute interface

int Schema::getNumAttributes() const
{
    return (int)m_attributes.size();
}

Attribute* Schema::getAttribute(int i) const
{
    if (i < 0 || i >= m_attributes.size()) {
        usdiLogError("Schema::getAttribute() i < 0 || i >= m_attributes.size()\n");
        return nullptr;
    }
    return m_attributes[i].get();
}

Attribute* Schema::findAttribute(const char *name, AttributeType type) const
{
    for (const auto& a : m_attributes) {
        if (strcmp(a->getName(), name) == 0) {
            if (type == AttributeType::Unknown || a->getType() == type) {
                return a.get();
            }
            else {
                return a->findOrCreateConverter(type);
            }
        }
    }
    return nullptr;
}

Attribute* Schema::createAttribute(const char *name, AttributeType type, AttributeType internal_type)
{
    if (auto *f = findAttribute(name, type)) {
        return f;
    }

    if (internal_type == AttributeType::Unknown) {
        switch (type) {
        // USD doesn't support these types. adding convert layer to emulate.
#define Case(T, InT)\
    case AttributeType::T: internal_type = AttributeType::InT; break;\
    case AttributeType::T##Array: internal_type = AttributeType::InT##Array; break;
        Case(Float2x2, Double2x2);
        Case(Float3x3, Double3x3);
        Case(Float4x4, Double4x4);
#undef Case
        default: internal_type = type; break;
        }
    }

    if (auto *c = CreateAttribute(this, name, internal_type)) {
        m_attributes.emplace_back(c);
        return c->findOrCreateConverter(type);
    }
    return nullptr;
}


// parent & child interface

Schema* Schema::getParent() const       { return m_parent; }
int     Schema::getNumChildren() const  { return (int)m_children.size(); }
Schema* Schema::getChild(int i) const   { return m_children[i]; }
Schema* Schema::findChild(const char * path, bool recursive) const
{
    auto ret = FindSchema(m_children, path);
    if (!ret && recursive) {
        for (auto child : m_children) {
            ret = child->findChild(path, recursive);
            if (ret) { break; }
        }
    }
    return ret;
}


// reference & instance interface

Schema* Schema::getMaster() const       { return m_master; }
int     Schema::getNumInstances() const { return (int)m_instances.size(); }
Schema* Schema::getInstance(int i) const{ return m_instances[i]; }

bool Schema::isEditable() const
{
    return !isInstance() && !isMaster() && !isInMaster();
}
bool    Schema::isInstance() const      { return m_master != nullptr || m_prim.IsInstance(); }
bool    Schema::isInstanceable() const  { return m_prim.IsInstanceable(); }
bool    Schema::isMaster() const        { return m_prim.IsMaster(); }
bool    Schema::isInMaster() const      { return m_prim.IsInMaster(); }
void    Schema::setInstanceable(bool v) { m_prim.SetInstanceable(v); }

bool Schema::addReference(const char *asset_path, const char *prim_path)
{
    if (!asset_path) { asset_path = ""; }
    return m_prim.GetReferences().AddReference(SdfReference(asset_path, SdfPath(prim_path)));
}


// payload interface

bool Schema::hasPayload() const
{
    return m_prim.HasPayload();
}
void Schema::loadPayload()
{
    if (hasPayload()) {
        m_prim.Load();
        m_update_flag_next.payload_loaded = 1;
    }
}
void Schema::unloadPayload()
{
    if (hasPayload()) {
        m_prim.Unload();
        m_update_flag_next.payload_unloaded = 1;
    }
}
bool Schema::setPayload(const char *asset_path, const char *prim_path)
{
    return m_prim.SetPayload(
        SdfPayload(std::string(asset_path), SdfPath(prim_path)));
}


// variant interface

bool Schema::hasVariants() const
{
    return m_prim.HasVariantSets();
}

int Schema::getNumVariantSets() const
{
    return (int)m_variant_sets.size();
}

const char* Schema::getVariantSetName(int iset) const
{
    if (iset < 0 || iset >= m_variant_sets.size()) {
        usdiLogError("Schema::getVariantSetName(): iset < 0 || iset >= m_variant_sets.size()\n");
        return "";
    }
    return m_variant_sets[iset].name.c_str();
}

int Schema::getNumVariants(int iset) const
{
    if (iset < 0 || iset >= m_variant_sets.size()) {
        usdiLogError("Schema::getNumVariants(): iset < 0 || iset >= m_variant_sets.size()\n");
        return 0;
    }
    return (int)m_variant_sets[iset].variants.size();
}

const char* Schema::getVariantName(int iset, int ival) const
{
    if (iset < 0 || iset >= m_variant_sets.size()) {
        usdiLogError("Schema::getVariantName(): iset < 0 || iset >= m_variant_sets.size()\n");
        return "";
    }
    if (ival < 0 || ival >= m_variant_sets[iset].variants.size()) {
        usdiLogError("Schema::getVariantName(): ival < 0 || ival >= m_variant_sets[iset].variants.size()\n");
        return "";
    }
    return m_variant_sets[iset].variants[ival].c_str();
}

int Schema::getVariantSelection(int iset) const
{
    if (iset < 0) { return -1; }
    if (iset >= m_variant_sets.size()) {
        usdiLogError("Schema::getVariantSelection(): iset >= m_variant_sets.size()\n");
        return 0;
    }

    auto valname = m_prim.GetVariantSets().GetVariantSelection(m_variant_sets[iset].name);
    if (valname.empty()) { return -1; }
    return findVariant(iset, valname.c_str());
}

bool Schema::setVariantSelection(int iset, int ival)
{
    if (iset < 0) { return false; }
    if (iset >= m_variant_sets.size()) {
        usdiLogError("Schema::setVariantSelection(): iset >= m_variant_sets.size()\n");
        return false;
    }

    auto& vset = m_variant_sets[iset];
    auto dst = m_prim.GetVariantSet(vset.name);
    auto sel = dst.GetVariantSelection();

    bool ret = false;
    if (ival < 0 || ival >= vset.variants.size()) {
        if (!sel.empty()) {
            ret = dst.ClearVariantSelection();
        }
    }
    else {
        if (sel != vset.variants[ival]) {
            ret = dst.SetVariantSelection(vset.variants[ival]);
        }
    }

    if (ret) {
        m_update_flag_next.variant_set_changed = 1;
    }
    return ret;
}

int Schema::findVariantSet(const char *name) const
{
    for (int i = 0; i < m_variant_sets.size(); ++i) {
        if (m_variant_sets[i].name == name) {
            return i;
        }
    }
    return -1;
}

int Schema::findVariant(int iset, const char *name) const
{
    if (iset < 0) { return -1; }
    if (iset >= m_variant_sets.size()) {
        usdiLogError("Schema::findVariant(): iset >= m_variant_sets.size()\n");
        return -1;
    }
    auto& variants = m_variant_sets[iset].variants;
    for (int i = 0; i < variants.size(); ++i) {
        if (variants[i] == name) {
            return i;
        }
    }
    return -1;
}

bool Schema::beginEditVariant(const char *set, const char *variant)
{
    auto vset = m_prim.GetVariantSets().GetVariantSet(set);
    if (!variant) {
        vset.ClearVariantSelection();
        return false;
    }
    else {
        vset.AddVariant(variant);
        vset.SetVariantSelection(variant);
        syncVariantSets();
        m_ctx->beginEdit(vset.GetVariantEditTarget());
        return true;
    }
}

void Schema::endEditVariant()
{
    m_ctx->endEdit();
}

void Schema::editVariants(const std::function<void()>& body)
{
    if (!isEditable()) {
        body();
        return;
    }

    std::vector<UsdEditTarget> edit_targets;
    std::vector<std::string> vset_names;

    // get current edit targets
    auto vsets = m_prim.GetVariantSets();
    vsets.GetNames(&vset_names);
    for (auto& n : vset_names) {
        auto s = vsets.GetVariantSelection(n);
        if (!s.empty()) {
            auto set = vsets.GetVariantSet(n);
            edit_targets.push_back(set.GetVariantEditTarget());
        }
    }

    // do edit
    for (auto& e : edit_targets) {
        m_ctx->beginEdit(e);
    }
    body();
    for (auto& e : edit_targets) {
        e; // unused
        m_ctx->endEdit();
    }
}

void Schema::notifyForceUpdate()
{
    m_update_flag_next.sample_updated = 1;
}

void Schema::notifyImportConfigChanged()
{
    m_update_flag_next.import_settings_updated = 1;
}

UpdateFlags Schema::getUpdateFlags() const { return m_update_flag; }
UpdateFlags Schema::getUpdateFlagsPrev() const  { return m_update_flag_prev; }

void Schema::updateSample(Time t)
{
    m_update_flag_prev = m_update_flag;
    m_update_flag = m_update_flag_next;
    m_update_flag_next.bits = 0;

    if(m_update_flag.sample_updated == 0) {
        m_update_flag.sample_updated = 1;
        if (!std::isnan(m_time_prev)) {
            if (t == m_time_prev) {
                m_update_flag.sample_updated = 0;
            }
            else if (std::isnan(m_time_start)) {
                m_update_flag.sample_updated = 0;
            }
            else if ((t <= m_time_start && m_time_prev <= m_time_start) || (t >= m_time_end && m_time_prev >= m_time_end)) {
                m_update_flag.sample_updated = 0;
            }
        }
    }

    //if (m_update_flag.variant_set_changed) {
    //    syncAttributes();
    //    syncTimeRange();
    //}

    m_time_prev = t;
}

void Schema::setOverrideImportSettings(bool v)
{
    if (m_master) {
        m_master->setOverrideImportSettings(v);
    }
    else {
        if (m_isettings_override != v) {
            m_isettings_override = v;
            m_update_flag_next.import_settings_updated = 1;
        }
    }
}
bool Schema::isImportSettingsOverridden() const
{ 
    if (m_master) {
        return m_master->isImportSettingsOverridden();
    }
    else {
        return m_isettings_override;
    }
}
const ImportSettings& Schema::getImportSettings() const
{
    if (m_master) {
        return m_master->getImportSettings();
    }
    else {
        return m_isettings_override ? m_isettings : m_ctx->getImportSettings();
    }
}
void Schema::setImportSettings(const ImportSettings& v)
{
    if (m_master) {
        m_master->setImportSettings(v);
    }
    else {
        if (m_isettings != v) {
            m_isettings = v;
            if (m_isettings_override) {
                m_update_flag_next.import_settings_updated = 1;
            }
        }
    }
}

void Schema::setOverrideExportSettings(bool v)
{
    if (m_master) {
        m_master->setOverrideExportSettings(v);
    }
    else {
        m_esettings_override = v;
    }
}
bool Schema::isExportSettingsOverridden() const
{
    if (m_master) {
        return m_master->isExportSettingsOverridden();
    }
    else {
        return m_esettings_override;
    }
}
const ExportSettings& Schema::getExportSettings() const
{
    if (m_master) {
        return m_master->getExportSettings();
    }
    else {
        return m_esettings_override ? m_esettings : m_ctx->getExportSettings();
    }
}
void Schema::setExportSettings(const ExportSettings& v)
{
    if (m_master) {
        m_master->setExportSettings(v);
    }
    else {
        m_esettings = v;
    }
}


void* Schema::getUserData() const { return m_userdata; }
void Schema::setUserData(void *v) { m_userdata = v; }



void Schema::addChild(Schema *child)
{
    m_children.push_back(child);
}

void Schema::addInstance(Schema *instance)
{
    m_instances.push_back(instance);
}

std::string Schema::makePath(const char *name_)
{
    // sanitize
    std::string name = name_;
    for (auto& c : name) {
        if (!std::isalnum(c)) {
            c = '_';
        }
    }

    std::string path;
    if (m_parent) {
        path += m_parent->getPath();
    }
    if (path.empty() || path.back() != '/') {
        path += "/";
    }
    path += name;

    usdiLogTrace("Schema::makePath(): %s\n", path.c_str());
    return path;
}



static std::vector<ISchemaHandler*>& GetSchemaHandlers()
{
    static std::vector<ISchemaHandler*> s_handlers;
    return s_handlers;
}

void RegisterSchemaHandlerImpl(ISchemaHandler& handler)
{
    auto& handlers = GetSchemaHandlers();
    handlers.push_back(&handler);
    std::sort(handlers.begin(), handlers.end(),
        [](ISchemaHandler* a, ISchemaHandler* b) -> bool { return a->getInheritDepth() > b->getInheritDepth(); });
}

Schema* CreateSchema(Context *ctx, Schema *parent, const UsdPrim& p)
{
    auto& handlers = GetSchemaHandlers();
    for (auto *handler : handlers) {
        if (handler->isCompatible(p)) {
            return handler->create(ctx, parent, p);
        }
    }
    return nullptr;
}

ISchemaHandler::~ISchemaHandler() {}


} // namespace usdi
