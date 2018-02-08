#include "pch.h"
#include "usdiInternal.h"
#include "usdiAttribute.h"
#include "usdiSchema.h"
#include "usdiXform.h"
#include "usdiCamera.h"
#include "usdiMesh.h"
#include "usdiPoints.h"
#include "usdiContext.h"


#ifdef _WIN32
    #pragma comment(lib, "Shlwapi.lib")
    #pragma comment(lib, "Ws2_32.lib")
    #pragma comment(lib, "half.lib")
    #pragma comment(lib, "usd_ms.lib")
    #ifdef usdiDbgVTune
        #pragma comment(lib, "libittnotify.lib")
    #endif
#endif
PXR_NAMESPACE_USING_DIRECTIVE

namespace usdi {
    extern int g_debug_level;
} // namespace usdi

extern "C" {

usdiAPI void usdiSetDebugLevel(int l)
{
    usdi::g_debug_level = l;
}

usdiAPI usdi::Time usdiDefaultTime()
{
    return std::numeric_limits<double>::quiet_NaN();
}


usdiAPI void usdiInitialize()
{
}

usdiAPI void usdiFinalize()
{
}

usdiAPI void* usdiAlignedMalloc(size_t size, size_t al) { return AlignedMalloc(size, al); }
usdiAPI void  usdiAlignedFree(void* addr)               { AlignedFree(addr); }

// Context interface


usdiAPI void usdiAddAssetSearchPath(const char *path)
{
    usdiTraceFunc();
    usdi::Context::addAssetSearchPath(path);
}
usdiAPI void usdiClearAssetSearchPath()
{
    usdiTraceFunc();
    usdi::Context::clearAssetSearchPath();
}

usdiAPI usdi::Context* usdiCreateContext()
{
    usdiTraceFunc();
    return new usdi::Context();
}

usdiAPI void usdiDestroyContext(usdi::Context *ctx)
{
    usdiTraceFunc();
    delete ctx;
}

usdiAPI bool usdiOpen(usdi::Context *ctx, const char *path)
{
    usdiTraceFunc();
    if (!ctx || !path) return false;
    return ctx->open(path);
}

usdiAPI bool usdiCreateStage(usdi::Context *ctx, const char *path)
{
    usdiTraceFunc();
    if (!ctx) return false;
    return ctx->createStage(path);
}

usdiAPI bool usdiSave(usdi::Context *ctx)
{
    usdiTraceFunc();
    if (!ctx) return false;
    return ctx->save();
}

usdiAPI bool usdiSaveAs(usdi::Context *ctx, const char *path)
{
    usdiTraceFunc();
    if (!ctx || !path) return false;
    return ctx->saveAs(path);
}

usdiAPI void usdiSetImportSettings(usdi::Context *ctx, const usdi::ImportSettings *v)
{
    usdiTraceFunc();
    if (!ctx || !v) return;
    ctx->setImportSettings(*v);
}
usdiAPI void usdiGetImportSettings(usdi::Context *ctx, usdi::ImportSettings *v)
{
    usdiTraceFunc();
    if (!ctx || !v) return;
    *v = ctx->getImportSettings();
}
usdiAPI void usdiSetExportSettings(usdi::Context *ctx, const usdi::ExportSettings *v)
{
    usdiTraceFunc();
    if (!ctx || !v) return;
    ctx->setExportSettings(*v);
}
usdiAPI void usdiGetExportSettings(usdi::Context *ctx, usdi::ExportSettings *v)
{
    usdiTraceFunc();
    if (!ctx || !v) return;
    *v = ctx->getExportSettings();
}

usdiAPI usdi::Schema* usdiGetRoot(usdi::Context *ctx)
{
    usdiTraceFunc();
    if (!ctx) return nullptr;
    return ctx->getRoot();
}
usdiAPI int usdiGetNumSchemas(usdi::Context *ctx)
{
    usdiTraceFunc();
    if (!ctx) return 0;
    return ctx->getNumSchemas();
}
usdiAPI usdi::Schema* usdiGetSchema(usdi::Context *ctx, int i)
{
    usdiTraceFunc();
    if (!ctx) return nullptr;
    return ctx->getSchema(i);
}
usdiAPI int usdiGetNumMasters(usdi::Context *ctx)
{
    usdiTraceFunc();
    if (!ctx) return 0;
    return ctx->getNumMasters();
}
usdiAPI usdi::Schema* usdiGetMaster(usdi::Context *ctx, int i)
{
    usdiTraceFunc();
    if (!ctx) return nullptr;
    return ctx->getMaster(i);
}
usdiAPI usdi::Schema* usdiFindSchema(usdi::Context *ctx, const char *path_or_name)
{
    usdiTraceFunc();
    if (!ctx) return nullptr;
    return ctx->findSchema(path_or_name);
}

usdiAPI usdi::Schema* usdiCreateOverride(usdi::Context *ctx, const char *prim_path)
{
    usdiTraceFunc();
    if (!ctx) { usdiLogError("usdiCreateOverride(): ctx is null\n"); return nullptr; }
    return ctx->createOverride(prim_path);
}
usdiAPI usdi::Xform* usdiCreateXform(usdi::Context *ctx, usdi::Schema *parent, const char *name)
{
    usdiTraceFunc();
    if (!ctx) { usdiLogError("usdiCreateXform(): ctx is null\n"); return nullptr; }
    return ctx->createSchema<usdi::Xform>(parent, name);
}
usdiAPI usdi::Camera* usdiCreateCamera(usdi::Context *ctx, usdi::Schema *parent, const char *name)
{
    usdiTraceFunc();
    if (!ctx) { usdiLogError("usdiCreateCamera(): ctx is null\n"); return nullptr; }
    return ctx->createSchema<usdi::Camera>(parent, name);
}
usdiAPI usdi::Mesh* usdiCreateMesh(usdi::Context *ctx, usdi::Schema *parent, const char *name)
{
    usdiTraceFunc();
    if (!ctx) { usdiLogError("usdiCreateMesh(): ctx is null\n"); return nullptr; }
    return ctx->createSchema<usdi::Mesh>(parent, name);
}
usdiAPI usdi::Points* usdiCreatePoints(usdi::Context *ctx, usdi::Schema *parent, const char *name)
{
    usdiTraceFunc();
    if (!ctx) { usdiLogError("usdiCreatePoints(): ctx is null\n"); return nullptr; }
    return ctx->createSchema<usdi::Points>(parent, name);
}

usdiAPI void usdiFlatten(usdi::Context *ctx)
{
    usdiTraceFunc();
    if (!ctx) return;
    ctx->flatten();
}

usdiAPI void usdiNotifyForceUpdate(usdi::Context *ctx)
{
    usdiTraceFunc();
    if (!ctx) return;
    ctx->notifyForceUpdate();
}

usdiAPI void usdiUpdateAllSamples(usdi::Context *ctx, usdi::Time t)
{
    usdiTraceFunc();
    if (!ctx) return;

    usdiVTuneScope("usdiUpdateAllSamples");
    ctx->updateAllSamples(t);
}
usdiAPI void usdiRebuildSchemaTree(usdi::Context *ctx)
{
    usdiTraceFunc();
    if (!ctx) return;
    ctx->rebuildSchemaTree();
}

usdiAPI int usdiEachTimeSample(usdi::Context * ctx, usdiTimeSampleCallback cb)
{
    usdiTraceFunc();
    if (!ctx) return 0;
    return ctx->eachTimeSample([cb](usdi::Time t) { cb(t); });
}


// Schema interface

usdiAPI int usdiPrimGetID(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return 0; }
    return schema->getID();
}
usdiAPI const char* usdiPrimGetPath(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return ""; }
    return schema->getPath();
}
usdiAPI const char* usdiPrimGetName(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return ""; }
    return schema->getName();
}
usdiAPI const char* usdiPrimGetUsdTypeName(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return ""; }
    return schema->getUsdTypeName();
}

// import / export settings
usdiAPI bool usdiPrimIsImportSettingsOverriden(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->isImportSettingsOverridden();
}
usdiAPI void usdiPrimSetOverrideImportSettings(usdi::Schema *schema, bool v)
{
    usdiTraceFunc();
    if (!schema) { return; }
    schema->setOverrideImportSettings(v);
}
usdiAPI void usdiPrimGetImportSettings(usdi::Schema *schema, usdi::ImportSettings *dst)
{
    usdiTraceFunc();
    if (!schema || !dst) { return; }
    *dst = schema->getImportSettings();
}
usdiAPI void usdiPrimSetImportSettings(usdi::Schema *schema, const usdi::ImportSettings *v)
{
    usdiTraceFunc();
    if (!schema || !v) { return; }
    schema->setImportSettings(*v);
}
usdiAPI bool usdiPrimIsExportSettingsOverriden(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->isExportSettingsOverridden();
}
usdiAPI void usdiPrimSetOverrideExportSettings(usdi::Schema *schema, bool v)
{
    usdiTraceFunc();
    if (!schema) { return; }
    schema->setOverrideExportSettings(v);
}
usdiAPI void usdiPrimGetExportSettings(usdi::Schema *schema, usdi::ExportSettings *dst)
{
    usdiTraceFunc();
    if (!schema || !dst) { return; }
    *dst = schema->getExportSettings();
}
usdiAPI void usdiPrimSetExportSettings(usdi::Schema *schema, const usdi::ExportSettings *v)
{
    usdiTraceFunc();
    if (!schema || !v) { return; }
    schema->setExportSettings(*v);
}

// master / instance
usdiAPI usdi::Schema* usdiPrimGetMaster(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return nullptr; }
    return schema->getMaster();
}
usdiAPI int usdiPrimGetNumInstances(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return 0; }
    return schema->getNumInstances();
}
usdiAPI usdi::Schema* usdiPrimGetInstance(usdi::Schema *schema, int i)
{
    usdiTraceFunc();
    if (!schema) { return nullptr; }
    return schema->getInstance(i);
}

usdiAPI bool usdiPrimIsEditable(usdi::Schema * schema)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->isEditable();
}
usdiAPI bool usdiPrimIsInstance(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->isInstance();
}
usdiAPI bool usdiPrimIsInstanceable(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->isInstanceable();
}
usdiAPI bool usdiPrimIsMaster(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->isMaster();
}
usdiAPI bool usdiPrimIsInMaster(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->isInMaster();
}
usdiAPI void usdiPrimSetInstanceable(usdi::Schema *schema, bool v)
{
    usdiTraceFunc();
    if (!schema) { return; }
    schema->setInstanceable(v);
}

// reference / payload
usdiAPI bool usdiPrimAddReference(usdi::Schema *schema, const char *asset_path, const char *prim_path)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->addReference(asset_path, prim_path);
}
usdiAPI bool usdiPrimHasPayload(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->hasPayload();
}
usdiAPI void usdiPrimLoadPayload(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return; }
    schema->loadPayload();
}
usdiAPI void usdiPrimUnloadPayload(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return; }
    schema->unloadPayload();
}
usdiAPI bool usdiPrimSetPayload(usdi::Schema *schema, const char *asset_path, const char *prim_path)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->setPayload(asset_path, prim_path);
}

// parent / chind
usdiAPI usdi::Schema* usdiPrimGetParent(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return nullptr; }
    return schema->getParent();
}
usdiAPI int usdiPrimGetNumChildren(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) return 0;
    return (int)schema->getNumChildren();
}
usdiAPI usdi::Schema* usdiPrimGetChild(usdi::Schema *schema, int i)
{
    usdiTraceFunc();
    if (!schema) return nullptr;
    return schema->getChild(i);

}
usdiAPI usdi::Schema* usdiPrimFindChild(usdi::Schema * schema, const char *path_or_name, bool recursive)
{
    usdiTraceFunc();
    if (!schema) return nullptr;
    return schema->findChild(path_or_name, recursive);
}
usdiAPI int usdiPrimGetNumAttributes(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return 0; }
    return (int)schema->getNumAttributes();
}
usdiAPI usdi::Attribute* usdiPrimGetAttribute(usdi::Schema *schema, int i)
{
    usdiTraceFunc();
    if (!schema) { return nullptr; }
    return schema->getAttribute(i);
}
usdiAPI usdi::Attribute* usdiPrimFindAttribute(usdi::Schema *schema, const char *name, usdi::AttributeType type)
{
    usdiTraceFunc();
    if (!schema) { return nullptr; }
    return schema->findAttribute(name, type);
}
usdiAPI usdi::Attribute* usdiPrimCreateAttribute(usdi::Schema *schema, const char *name, usdi::AttributeType type, usdi::AttributeType internal_type)
{
    usdiTraceFunc();
    if (!schema) { return nullptr; }
    return schema->createAttribute(name, type, internal_type);
}

usdiAPI int usdiPrimGetNumVariantSets(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return 0; }
    return schema->getNumVariantSets();
}
usdiAPI const char* usdiPrimGetVariantSetName(usdi::Schema *schema, int iset)
{
    usdiTraceFunc();
    if (!schema) { return ""; }
    return schema->getVariantSetName(iset);
}
usdiAPI int usdiPrimGetNumVariants(usdi::Schema *schema, int iset)
{
    usdiTraceFunc();
    if (!schema) { return 0; }
    return schema->getNumVariants(iset);
}
usdiAPI const char* usdiPrimGetVariantName(usdi::Schema *schema, int iset, int ival)
{
    usdiTraceFunc();
    if (!schema) { return ""; }
    return schema->getVariantName(iset, ival);
}
usdiAPI int usdiPrimGetVariantSelection(usdi::Schema *schema, int iset)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->getVariantSelection(iset);
}
usdiAPI bool usdiPrimSetVariantSelection(usdi::Schema *schema, int iset, int ival)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->setVariantSelection(iset, ival);
}
usdiAPI int usdiPrimFindVariantSet(usdi::Schema *schema, const char *name)
{
    usdiTraceFunc();
    if (!schema) { return -1; }
    return schema->findVariantSet(name);
}
usdiAPI int usdiPrimFindVariant(usdi::Schema *schema, int iset, const char *name)
{
    usdiTraceFunc();
    if (!schema) { return -1; }
    return schema->findVariant(iset, name);
}
usdiAPI bool usdiPrimBeginEditVariant(usdi::Schema *schema, const char *set, const char *variant)
{
    usdiTraceFunc();
    if (!schema) { return false; }
    return schema->beginEditVariant(set, variant);
}
usdiAPI void usdiPrimEndEditVariant(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return; }
    return schema->endEditVariant();
}

usdiAPI usdi::UpdateFlags usdiPrimGetUpdateFlags(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return{ {0} }; }
    return schema->getUpdateFlags();
}
usdiAPI usdi::UpdateFlags usdiPrimGetUpdateFlagsPrev(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return{ {0} }; }
    return schema->getUpdateFlagsPrev();
}

usdiAPI void usdiPrimUpdateSample(usdi::Schema *schema, usdi::Time t)
{
    usdiTraceFunc();
    if (!schema) { return; }
    return schema->updateSample(t);
}
usdiAPI void* usdiPrimGetUserData(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) { return nullptr; }
    return schema->getUserData();
}
usdiAPI void usdiPrimSetUserData(usdi::Schema *schema, void *data)
{
    usdiTraceFunc();
    if (!schema) { return; }
    return schema->setUserData(data);
}


// Xform interface

usdiAPI usdi::Xform* usdiAsXform(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) return nullptr;
    return schema->as<usdi::Xform*>();
}

usdiAPI void usdiXformGetSummary(usdi::Xform *xf, usdi::XformSummary *dst)
{
    usdiTraceFunc();
    if (!xf || !dst) return;
    *dst = xf->getSummary();
}
usdiAPI bool usdiXformReadSample(usdi::Xform *xf, usdi::XformData *dst, usdi::Time t)
{
    usdiTraceFunc();
    if (!xf || !dst) return false;
    usdiVTuneScope("usdiXformReadSample");
    return xf->readSample(*dst, t);
}
usdiAPI bool usdiXformWriteSample(usdi::Xform *xf, const usdi::XformData *src, usdi::Time t)
{
    usdiTraceFunc();
    if (!xf || !src) return false;
    usdiVTuneScope("usdiXformWriteSample");
    return xf->writeSample(*src, t);
}

usdiAPI int usdiXformEachSample(usdi::Xform * xf, usdiXformSampleCallback cb)
{
    usdiTraceFunc();
    if (!xf || !cb) return 0;
    return xf->eachSample([cb](const usdi::XformData& data, usdi::Time t) { cb(&data, t); });
}


// Camera interface

usdiAPI usdi::Camera* usdiAsCamera(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) return nullptr;
    return schema->as<usdi::Camera*>();
}
usdiAPI void usdiCameraGetSummary(usdi::Camera *cam, usdi::CameraSummary *dst)
{
    usdiTraceFunc();
    if (!cam || !dst) return;
    *dst = cam->getSummary();
}
usdiAPI bool usdiCameraReadSample(usdi::Camera *cam, usdi::CameraData *dst, usdi::Time t)
{
    usdiTraceFunc();
    if (!cam || !dst) return false;
    usdiVTuneScope("usdiCameraReadSample");
    return cam->readSample(*dst, t);
}
usdiAPI bool usdiCameraWriteSample(usdi::Camera *cam, const usdi::CameraData *src, usdi::Time t)
{
    usdiTraceFunc();
    if (!cam || !src) return false;
    usdiVTuneScope("usdiCameraWriteSample");
    return cam->writeSample(*src, t);
}

usdiAPI int usdiCameraEachSample(usdi::Camera *cam, usdiCameraSampleCallback cb)
{
    usdiTraceFunc();
    if (!cam || !cb) return 0;
    return cam->eachSample([cb](const usdi::CameraData& data, usdi::Time t) { cb(&data, t); });
}


// Mesh interface

usdiAPI usdi::Mesh* usdiAsMesh(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) return nullptr;
    return schema->as<usdi::Mesh*>();
}


usdiAPI void usdiMeshGetSummary(usdi::Mesh *mesh, usdi::MeshSummary *dst)
{
    usdiTraceFunc();
    if (!mesh || !dst) return;
    *dst = mesh->getSummary();
}

usdiAPI bool usdiMeshReadSample(usdi::Mesh *mesh, usdi::MeshData *dst, usdi::Time t, bool copy)
{
    usdiTraceFunc();
    if (!mesh || !dst) return false;
    usdiVTuneScope("usdiMeshReadSample");
    return mesh->readSample(*dst, t, copy);
}

usdiAPI bool usdiMeshWriteSample(usdi::Mesh *mesh, const usdi::MeshData *src, usdi::Time t)
{
    usdiTraceFunc();
    if (!mesh || !src) return false;
    usdiVTuneScope("usdiMeshWriteSample");
    return mesh->writeSample(*src, t);
}

usdiAPI int usdiMeshEachSample(usdi::Mesh *mesh, usdiMeshSampleCallback cb)
{
    usdiTraceFunc();
    if (!mesh || !cb) return 0;
    return mesh->eachSample([cb](const usdi::MeshData& data, usdi::Time t) { cb(&data, t); });
}


// Points interface

usdiAPI usdi::Points* usdiAsPoints(usdi::Schema *schema)
{
    usdiTraceFunc();
    if (!schema) return nullptr;
    return schema->as<usdi::Points*>();
}

usdiAPI void usdiPointsGetSummary(usdi::Points *points, usdi::PointsSummary *dst)
{
    usdiTraceFunc();
    if (!points || !dst) return;
    *dst = points->getSummary();
}

usdiAPI bool usdiPointsReadSample(usdi::Points *points, usdi::PointsData *dst, usdi::Time t, bool copy)
{
    usdiTraceFunc();
    if (!points || !dst) return false;
    usdiVTuneScope("usdiPointsReadSample");
    return points->readSample(*dst, t, copy);
}

usdiAPI bool usdiPointsWriteSample(usdi::Points *points, const usdi::PointsData *src, usdi::Time t)
{
    usdiTraceFunc();
    if (!points || !src) return false;
    usdiVTuneScope("usdiPointsWriteSample");
    return points->writeSample(*src, t);
}

usdiAPI int usdiPointsEachSample(usdi::Points *points, usdiPointsSampleCallback cb)
{
    usdiTraceFunc();
    if (!points || !cb) return 0;
    return points->eachSample([cb](const usdi::PointsData& data, usdi::Time t) { cb(&data, t); });
}


// Attribute interface

usdiAPI usdi::Schema* usdiAttrGetParent(usdi::Attribute *attr)
{
    usdiTraceFunc();
    if (!attr) { return nullptr; }
    return attr->getParent();
}

usdiAPI const char* usdiAttrGetName(usdi::Attribute *attr)
{
    usdiTraceFunc();
    if (!attr) { return ""; }
    return attr->getName();
}

usdiAPI const char* usdiAttrGetTypeName(usdi::Attribute *attr)
{
    usdiTraceFunc();
    if (!attr) { return ""; }
    return attr->getTypeName();
}

usdiAPI void usdiAttrGetSummary(usdi::Attribute *attr, usdi::AttributeSummary *dst)
{
    usdiTraceFunc();
    if (!attr) { return; }
    *dst = attr->getSummary();
}

usdiAPI bool usdiAttrReadSample(usdi::Attribute *attr, usdi::AttributeData *dst, usdi::Time t, bool copy)
{
    usdiTraceFunc();
    if (!attr || !dst) { return false; }
    return attr->readSample(*dst, t, copy);
}

usdiAPI bool usdiAttrWriteSample(usdi::Attribute *attr, const usdi::AttributeData *src, usdi::Time t)
{
    usdiTraceFunc();
    if (!attr || !src) { return false; }
    return attr->writeSample(*src, t);
}

usdiAPI bool usdiConvertUSDToAlembic(const char *src_usd, const char *dst_abc)
{
    return usdi::Context::convertUSDToAlembic(src_usd, dst_abc);
}

} // extern "C"
