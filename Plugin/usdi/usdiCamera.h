#pragma once

namespace usdi {

class Camera : public Xform
{
typedef Xform super;
public:
    DefSchemaTraits(UsdGeomCamera, "Camera");

    Camera(Context *ctx, Schema *parent, const UsdPrim& prim);
    Camera(Context *ctx, Schema *parent, const char *name, const char *type = _getUsdTypeName());
    ~Camera() override;

    void                updateSample(Time t) override;

    const CameraSummary& getSummary() const;
    bool                readSample(CameraData& dst);
    bool                writeSample(const CameraData& src, Time t);

    using SampleCallback = std::function<void(const CameraData& data, Time t)>;
    int eachSample(const SampleCallback& cb);

private:
    UsdGeomCamera       m_cam;
    CameraData          m_sample;

    mutable bool          m_summary_needs_update = true;
    mutable CameraSummary m_summary;
};

} // namespace usdi
