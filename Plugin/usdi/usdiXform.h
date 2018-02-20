#pragma once

namespace usdi {

class Xform : public Schema
{
typedef Schema super;
public:
    DefSchemaTraits(UsdGeomXformable, "Xform");

    Xform(Context *ctx, Schema *parent, const UsdPrim& prim);
    Xform(Context *ctx, Schema *parent, const char *name, const char *type = _getUsdTypeName());
    ~Xform() override;

    void                updateSample(Time t) override;

    const XformSummary& getSummary() const;
    bool                readSample(XformData& dst);
    bool                writeSample(const XformData& src, Time t);

    using SampleCallback = std::function<void(const XformData& data, Time t)>;
    int eachSample(const SampleCallback& cb);

private:
    typedef std::vector<UsdGeomXformOp> UsdGeomXformOps;

    void interpretXformOps();

    UsdGeomXformable    m_xf;
    UsdGeomXformOps     m_read_ops;
    UsdGeomXformOps     m_write_ops;

    XformData            m_sample;
    mutable bool         m_summary_needs_update = true;
    mutable XformSummary m_summary;
};

} // namespace usdi
